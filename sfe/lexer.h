#ifndef LEXER_H
#define LEXER_H

#include <iosfwd>
#include <vector>

#include "input.h"

struct Token {
  enum Type { IDENT, NUMB, PLUS, MINUS, TIMES, kwDIV, kwMOD,
              EQ, NEQ, LT, GT, LTE, GTE, LPAR, RPAR, ASSIGN,
              COMMA, SEMICOLON, DOT, COLON,
              LBR, RBR, // [, ]
              APOSTROPHE,
              kwVAR, kwCONST, kwBEGIN, kwEND,
              kwIF, kwTHEN, kwELSE, kwAND, kwOR, kwNOT,
              kwBREAK, kwWHILE, kwDO,
              kwFOR, kwTO, kwDOWNTO,
              kwOF, kwPROGRAM,
              kwFUNCTION, kwPROCEDURE, kwFORWARD,
              EOI };
  static const int MAX_IDENT_LEN = 32;

  static const char * TypeStr[];

  Type type;
  char ident[MAX_IDENT_LEN]; /* IDENT attribute */
  int number; /* NUMB attribute */

  static Type keyword(const char * id);
};

class Lexer {
private:
  Input mInput;
  bool reddit; // whether one symbol in advance has been read

  /* if any tokens have been put back, nextToken() will return
   * tokens from this buffer first */
  std::vector<Token> bufferedTokens;
public:
  Lexer(std::istream & input);
  Token nextToken();
  void returnToken(Token t); // allow for LL(k) analysis

  // reads all characters read till '
  void readString(std::string &str);
};

#endif // LEXER_H
