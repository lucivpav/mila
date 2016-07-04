#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <cassert>

#include "input.h"
using namespace std;

void util_init(Input * input);
void error(const string & text, bool printLine = true);

#endif // UTIL_H
