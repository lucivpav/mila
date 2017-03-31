#include "util.h"

#include <iostream>
#include <cstdlib>

static Input * mInput = nullptr;

void util_init(Input * input)
{
  mInput = input;
}

void error(const string & text, bool printLine)
{
  cout << "Error";
  if ( printLine ) {
    assert ( mInput );
    cout << " on line " << mInput->curLineNumber();
  }
  cout << ": " << text << endl;
  cout << mInput->curLine() << endl;
  exit(1);
}
