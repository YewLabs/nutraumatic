#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/concat.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

using namespace fst;
using std::string;

bool isWord(char* w);

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 4 || strlen(argv[2]) == 0) {
    fprintf(stderr, "usage: %s input.index expression [--debug]\n", argv[0]);
    return 2;
  }
  bool mustbeword = true;
  if (argc == 4 && strcmp(argv[3], "--debug") == 0) {
    mustbeword = false;
    fprintf(stderr, "Running in debug mode.\n");
  }
  if (mustbeword && !isWord(argv[2])) {
    fprintf(stderr, "Error: \"%s\" does not seem to be a word.\n", argv[2]);
    return 3;
  }

  SymbolTable *chars = new SymbolTable("chars");
  chars->AddSymbol("epsilon", 0);
  chars->AddSymbol("space", ' ');
  for (int i = 33; i <= 127; ++i)
    chars->AddSymbol(string(1, i), i);

  StdVectorFst parsed;
  parsed.SetInputSymbols(chars);
  parsed.SetOutputSymbols(chars);
  
  if (strlen(argv[2]) >= 1 && argv[2][0] == 'Z') {
    fprintf(stderr, "error: can't parse \"%s\"\n", argv[2]);
    return 2;
  }

  const char *p = ParseOuter(argv[2], &parsed);
  if (p == NULL || *p != '\0') {
    fprintf(stderr, "error: can't parse \"%s\"\n", p ? p : argv[2]);
    return 2;
  }

  // Require a space at the end, so the matches must be complete words.
  StdVectorFst space;
  ParseExpr(" ", &space, 'a');
  Concat(&parsed, space);

  FILE *fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[1]);
    return 1;
  }

  ExprFilter filter(parsed);
  IndexReader reader(fp);
  SearchDriver driver(&reader, &filter, filter.start(), 1e-6);
  if (!strchr(argv[2], 'J')) {
    PrintAll(&driver);
  }
  return 0;
}


bool isWord(char* w) {
  auto l = strlen(w);
  std::ifstream is("/usr/share/dict/words");
  string str;
  while (std::getline(is, str)) {
    if (l == str.length() && strncasecmp(w,str.c_str(), str.length()) == 0) {
      fprintf(stderr, "It's a word: \"%s\"\n", str.c_str());
      return true;
    }
  }
  return false;
}