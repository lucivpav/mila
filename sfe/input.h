#ifndef INPUT_H
#define INPUT_H

#include <iosfwd>

class Input
{
public:
  struct Symbol {
    enum Type { LETTER, NUMBER, WHITE_SPACE, END, NO_TYPE };

    Type type;
    char symbol;
  };

  Input(std::istream & input);
  void readSymbol();
  const Symbol & curSymbol();

  int curLineNumber() const;
  const char * curLine() const;
private:
  char getChar();

  static const int MAX_LINE_LEN = 258; // (256) \n \0
  char line[MAX_LINE_LEN];
  bool extendedLine;
  int lineNumber;
  char * currentChar;
  std::istream & input;

  Symbol mSymbol;
};

#endif // INPUT_H
