#include "lexer.h"

#include <fstream>
#include <cstring>

#include "util.h"

const char * Token::TypeStr[] = {
  "IDENT", "NUMB", "PLUS", "MINUS", "TIMES", "kwDIV", "kwMOD",
  "EQ", "NEQ", "LT", "GT", "LTE", "GTE", "LPAR", "RPAR", "ASSIGN",
  "COMMA", "SEMICOLON", "DOT", "COLON",
  "LBR", "RBR",
  "APOSTROPHE",
  "kwVAR", "kwCONST", "kwBEGIN", "kwEND",
  "kwIF", "kwTHEN", "kwELSE", "kwAND", "kwOR", "kwNOT",
  "kwBREAK", "kwWHILE", "kwDO",
  "kwFOR", "kwTO", "kwDOWNTO",
  "kwOF", "kwPROGRAM",
  "kwFUNCTION", "kwPROCEDURE", "kwFORWARD",
  "EOI" };

const struct {const char* word; Token::Type type;} keywordTable[] = {
   {"var", Token::kwVAR},
   {"const", Token::kwCONST},
   {"begin", Token::kwBEGIN},
   {"end", Token::kwEND},
   {"div", Token::kwDIV},
   {"mod", Token::kwMOD},
   {"if", Token::kwIF},
   {"then", Token::kwTHEN},
   {"else", Token::kwELSE},
   {"and", Token::kwAND},
   {"or", Token::kwOR},
   {"not", Token::kwNOT},
   {"break", Token::kwBREAK},
   {"while", Token::kwWHILE},
   {"do", Token::kwDO},
   {"for", Token::kwFOR},
   {"to", Token::kwTO},
   {"downto", Token::kwDOWNTO},
   {"of", Token::kwOF},
   {"program", Token::kwPROGRAM},
   {"function", Token::kwFUNCTION},
   {"procedure", Token::kwPROCEDURE},
   {"forward", Token::kwFORWARD},
   {NULL, (Token::Type) 0}
};

Token::Type Token::keyword(const char * id)
{
  for ( int i = 0 ; keywordTable[i].word ; ++i )
    if ( strcmp(id,keywordTable[i].word) == 0 )
      return keywordTable[i].type;
  return IDENT;
}

Lexer::Lexer(std::istream &input)
  : mInput(input),
    reddit(false)
{
  util_init(&mInput);
}

Token Lexer::nextToken()
{
  if ( bufferedTokens.size() ) {
    Token t = bufferedTokens.back();
    bufferedTokens.pop_back();
    return t;
  }

  typedef Input::Symbol Symbol;
  Token token;
  int identSize;
q0: // start
  if ( !reddit ) mInput.readSymbol();
  else reddit = false;
  const Symbol & s = mInput.curSymbol();
  switch( s.symbol ) {
  case '{':
    goto q1;
  case '+':
    token.type = Token::PLUS;
    return token;
  case '-':
    token.type = Token::MINUS;
    return token;
  case '*':
    token.type = Token::TIMES;
    return token;
  case '(':
    token.type = Token::LPAR;
    return token;
  case ')':
    token.type = Token::RPAR;
    return token;
  case '=':
    token.type = Token::EQ;
    return token;
  case '<':
    goto q7;
  case '>':
    goto q8;
  case ':':
    goto q9;
  case '$': // hexa
    token.number = 0;
    goto q6;
  case '&': // octa
    token.number = 0;
    goto q5;
  case ',':
    token.type = Token::COMMA;
    return token;
  case ';':
    token.type = Token::SEMICOLON;
    return token;
  case '.':
    token.type = Token::DOT;
    return token;
  case '[':
    token.type = Token::LBR;
    return token;
  case ']':
    token.type = Token::RBR;
    return token;
  case '\'':
    token.type = Token::APOSTROPHE;
    return token;
  default:;
  }
  switch ( s.type ) {
  case Symbol::WHITE_SPACE:
    goto q0;
  case Symbol::END:
    token.type = Token::EOI;
    return token;
  case Symbol::LETTER:
    identSize = 0;
    goto q2;
  case Symbol::NUMBER:
    token.number = s.symbol - '0';
    token.type = Token::NUMB;
    if ( s.symbol == '0' ) {
      goto q4;
    }
    goto q3;
  default:
    error("Invalid symbol");
  }
q1: // {
  mInput.readSymbol();
  switch ( s.symbol ) {
  case '}':
    goto q0;
  default:;
  }
  switch ( s.type ) {
  case Symbol::END:
    error("Unterminated comment");
  default:
    goto q1;
  }
q2: // ident or kw
  switch ( s.type ) {
  case Symbol::LETTER:
  case Symbol::NUMBER:
    token.ident[identSize++] = s.symbol;
    mInput.readSymbol();
    goto q2;
  default:
    token.ident[identSize++] = 0;
    token.type = Token::keyword(token.ident);
    reddit = true;
    return token;
  }

q3: // number
  mInput.readSymbol();
  switch(s.type) {
    case Symbol::NUMBER:
      token.number = 10*token.number + (s.symbol - '0');
      goto q3;
    default:
      reddit = true;
      return token;
  }
q4: // zero, hex or octa
  mInput.readSymbol();
  switch(s.type) {
    case Symbol::NUMBER: // octa
      if ( s.symbol < '0' || s.symbol > '7' )
        error("Invalid octal number");
      token.number = s.symbol - '0';
      goto q5;
    case Symbol::LETTER: // hexa
      if ( s.symbol != 'x' &&
           s.symbol != 'X' )
        error("Invalid hexadecimal number");
      goto q6;
    default:
      reddit = true;
      return token; // (possibly) zero
  }

q5: // octal number
  token.type = Token::NUMB;
  mInput.readSymbol();
  switch ( s.type ) {
    case Symbol::NUMBER:
      if ( s.symbol < '0' || s.symbol > '7' )
        error("Invalid octal number");
      token.number = 8*token.number + (s.symbol - '0');
      goto q5;
    case Symbol::LETTER:
        error("Invalid octal number");
  default:
    reddit = true;
    return token;
  }

q6: // hexa number
  token.type = Token::NUMB;
  mInput.readSymbol();
  switch ( s.type ) {
    case Symbol::NUMBER:
      token.number = 16*token.number + (s.symbol - '0');
      goto q6;
    case Symbol::LETTER:
      if ( s.symbol < 'A' && s.symbol > 'F' &&
           s.symbol < 'a' && s.symbol > 'f' )
        error("Invalid hexadecimal number");

      int x;
      if ( s.symbol >= 'A' && s.symbol <= 'F' )
        x = s.symbol - 'A' + 10;
      else
        x = s.symbol - 'a' + 10;

      token.number = 16*token.number + x;
      goto q6;
  default:
    reddit = true;
    return token;
  }
q7: // <
  mInput.readSymbol();
  switch ( s.symbol ) {
  case '=':
    token.type = Token::LTE;
    return token;
  case '>':
    token.type = Token::NEQ;
    return token;
  default:
    reddit = true;
  }
  switch ( s.type ) {
  default:
    token.type = Token::LT;
    return token;
  }
q8: // >
  mInput.readSymbol();
  switch ( s.symbol ) {
  case '=':
    token.type = Token::GTE;
    return token;
  default:;
  }
  switch ( s.type ) {
  default:
    token.type = Token::GT;
    return token;
  }
q9: // :
  mInput.readSymbol();
  switch ( s.symbol ) {
  case '=':
    token.type = Token::ASSIGN;
    return token;
  default:;
    token.type = Token::COLON;
    reddit = true;
    return token;
  }
  switch ( s.type ) {
  default:
    error("Invalid assignment or colon");
  }
  assert ( false );
}

void Lexer::returnToken(Token t)
{
  bufferedTokens.push_back(t);
}

void Lexer::readString(std::string & str)
{
  // todo: handle bufferedTokens properly
  if ( !reddit ) mInput.readSymbol();
  while ( 1 ) {
    const Input::Symbol & s = mInput.curSymbol();
    if ( s.symbol == '\'' ) break;
    if ( s.type == Input::Symbol::END ) error("string not terminated");
    str += s.symbol;
    mInput.readSymbol();
  }
  reddit = true;
}
