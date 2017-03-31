#include "input.h"

#include <fstream>
#include <cstring>

#include "util.h"

Input::Input(std::istream &input)
  : extendedLine(false),
    lineNumber(0),
    currentChar(line),
    input(input)
{
  line[0] = 0;
}

void Input::readSymbol()
{
  char c = getChar();
  Symbol s;
  if ( ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) || ( c == '_' ) )
    s.type = Symbol::LETTER;
  else if ( c >= '0' && c <= '9' )
    s.type = Symbol::NUMBER;
  else if ( c == EOF )
    s.type = Symbol::END;
  else if ( c <= ' ' )
    s.type = Symbol::WHITE_SPACE;
  else
    s.type = Symbol::NO_TYPE;

  s.symbol = c;
  mSymbol = s;
}

const Input::Symbol & Input::curSymbol()
{
  return mSymbol;
}

int Input::curLineNumber() const
{
  return lineNumber;
}

const char * Input::curLine() const
{
  return line;
}

char Input::getChar()
{
  if ( *currentChar ) return *currentChar++;
  input.getline(line, MAX_LINE_LEN-1);
  if ( !input && !input.eof() ) error("Cannot read from input file");

  int size = strlen(line);
  line[size] = input.eof() ? EOF : '\n';
  line[size+1] = 0;

  currentChar = line;
  if ( !extendedLine ) lineNumber++;

  extendedLine = input.fail();
  return *currentChar++;
}
