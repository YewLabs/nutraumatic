#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/closure.h"
#include "fst/concat.h"
#include "fst/difference.h"
#include "fst/project.h"
#include "fst/union.h"
#include "fst/vector-fst.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <vector>

using namespace fst;
using std::vector;

typedef StdArc::StateId State;
typedef StdArc::Weight Weight;

const char *countries = "alakazarcacoctdeflgahiidiliniakskylamemdmamimnmsmomtnenvnhnjnmnyncndohokorpariscsdtntxutvtvawawvwiwy";

// consider all the letters X could be
const char* ParseOuter(const char *p, StdMutableFst *fst) {
  const char* p3 = p;
  if (strchr(p, 'X')) {
    for (char x = 'a'; x <= 'z'; ++x) {
      p3 = p;
      StdVectorFst res;
      p3= ParseExpr(p3, &res, x);
      if (p3 == NULL || *p3 != '\0') {
        return p3;
      }
      Union(fst, res);
    }
  } else {
    p3 = ParseExpr(p, fst, 'a');
  }
  return p3;
}


// an Expr is a bunch of branches separated by |s
const char *
ParseExpr(const char *p, StdMutableFst *fst, char xMeaning)
{
  p = ParseBranch(p, fst, xMeaning);
  while (p != NULL && *p == 'F') {
    StdVectorFst branch;
    p = ParseBranch(p + 1, &branch, xMeaning);
    Union(fst, branch);
  }
  return p;
}

// a branch is a bunch of factors separated by &s
const char *ParseBranch(const char *p, StdMutableFst* fst, char xMeaning) {
  vector<StdVectorFst> to_intersect;
  StdVectorFst first;
  p = ParseFactor(p, &first, xMeaning);
  to_intersect.push_back(first);
  while (p != NULL && *p == 'N') {
    StdVectorFst next;
    p = ParseFactor(p + 1, &next, xMeaning);
    to_intersect.push_back(next);
  }
  IntersectExprs(to_intersect, fst);
  return p;
}

// a factor is a series of pieces concatenated
const char *ParseFactor(const char *p, StdMutableFst* fst, char xMeaning) {
  fst->SetStart(fst->AddState());
  fst->SetFinal(fst->Start(), Weight::One());
  for (;;) {
    StdVectorFst piece;
    const char *n = ParsePossiblyInvertedPiece(p, &piece, xMeaning);
    if (n == NULL) return p;
    Concat(fst, piece);
    p = n;
  }
}

const char *ParsePossiblyInvertedPiece(const char *p, StdMutableFst* fst, char xMeaning) {
  StdVectorFst one;
  p = ParsePiece(p, &one, xMeaning);
  if (p == NULL)
    return NULL;
  
  
  int inversions = 0;
  while (*p == 'L') {
    inversions++;
    p++;
  }

  if (inversions%2==1)
  {
    StdVectorFst anything, oneOptimized;
    ArcSort(&one, StdOLabelCompare());
    ArcMap(&one, RmWeightMapper<StdArc>());
    Project(&one, PROJECT_OUTPUT);
    // RmEpsilon(&one);
    OptimizeExpr(one, &oneOptimized);
    ParsePiece("AG", &anything, xMeaning);
    Difference(anything, oneOptimized, fst);
  } else {
    Union(fst, one);
  }
  return p;
}

// a piece is a letter or glob
const char *ParsePiece(const char *p, StdMutableFst* fst, char xMeaning) {
  StdVectorFst one;
  p = ParseAtom(p, &one, xMeaning);
  if (p == NULL) return NULL;

  int min, max;
  if (*p == 'G') { // *
    min = 0;
    max = INT_MAX;
    ++p;
  } else if (*p == 'T') { // +
    min = 1;
    max = INT_MAX;
    ++p;
  } else if (*p == 'H') { // ?
    min = 0;
    max = 1;
    ++p;
  } /*else if (*p == '{') {
    min = strtoul(p + 1, (char**) &p, 10);
    if (*p == ',' && *(p + 1) == '}') {
      max = INT_MAX;
      ++p;
    } else if (*p == ',') {
      max = strtoul(p + 1, (char**) &p, 10);
    } else {
      max = min;
    }
    if (*p != '}' || max < min || (max > 255 && max < INT_MAX)) return NULL;
    ++p;
  } */else {
    min = max = 1;
  }

  StdVectorFst many;
  many.SetStart(many.AddState());
  many.SetFinal(many.Start(), Weight::One());

  assert(max >= min && min >= 0);
  for (int i = 0; i <= min || (i <= max && max < INT_MAX); ++i) {
    if (i >= min) Union(fst, many);
    Concat(&many, one);
  }

  if (max >= INT_MAX) {
    Closure(&one, CLOSURE_STAR);
    Concat(&many, one);
    Union(fst, many);
  }

  return p;
}

const void GetIncr(StdMutableFst *out)
{
  static StdVectorFst fst;
  static bool made;
  if (made)
  {
    Union(out, fst);
    return;
  }
  State states[26];
  for (size_t i=0;i<26;i++) {
    states[i] = fst.AddState();
  }
  State start = fst.AddState();
  State final = fst.AddState();
  fst.SetStart(start);
  fst.SetFinal(final, Weight::One());
  for (char i=0;i<26;i++) {
    for (char j=i+1;j<26;j++) {
      fst.AddArc(states[size_t(i)], StdArc('a' + j, 'a' + j, Weight::One(), states[size_t(j)]));
    }
    fst.AddArc(states[size_t(i)], StdArc(0, 0, Weight::One(), final));
    fst.AddArc(start, StdArc('a' + i, 'a' + i, Weight::One(), states[size_t(i)]));
  }
  Union(out, fst);
  made = true;
}

const void GetSum42(StdMutableFst *out)
{
  static StdVectorFst fst;
  static bool made;
  if (made)
  {
    Union(out, fst);
    return;
  }
  State states[43];
  for (size_t i = 0; i < 43; i++)
  {
    states[i] = fst.AddState();
  }
  fst.SetStart(states[0]);
  fst.SetFinal(states[42], Weight::One());
  for (char i = 0; i < 42; i++)
  {
    for (char j = i+1; j <= i+26 && j<=42; j++)
    {
      fst.AddArc(states[size_t(i)], StdArc('a' + j-i-1, 'a' + j-i-1, Weight::One(), states[size_t(j)]));
    }
  }
  Union(out, fst);
  made = true;
}

const void GetDbl(StdMutableFst *out)
{
  static StdVectorFst fst;
  static bool made;
  if (made)
  {
    Union(out, fst);
    return;
  }
  State states[26];
  for (size_t i = 0; i < 26; i++)
  {
    states[i] = fst.AddState();
  }
  State start = fst.AddState();
  State final = fst.AddState();
  fst.SetStart(start);
  fst.SetFinal(final, Weight::One());
  for (char i = 0; i < 26; i++)
  {

    fst.AddArc(states[size_t(i)], StdArc('a' + i, 'a' + i, Weight::One(), final));
    fst.AddArc(start, StdArc('a' + i, 'a' + i, Weight::One(), states[size_t(i)]));
  }
  Union(out, fst);
  made = true;
}
const void GetCountries(StdMutableFst *out)
{
  static StdVectorFst fst;
  static bool made;
  if (made)
  {
    Union(out, fst);
    return;
  }
  State states[26];
  for (size_t i = 0; i < 26; i++)
  {
    states[i] = fst.AddState();
  }
  State start = fst.AddState();
  State final = fst.AddState();
  fst.SetStart(start);
  fst.SetFinal(final, Weight::One());
  for (size_t i = 0; i<strlen(countries);i+=2)
  {
    char c1 = countries[i];
    char c2 = countries[i+1];
    fst.AddArc(states[size_t(c1 - 'a')], StdArc(c2, c2, Weight::One(), final));
  }
  for (size_t i = 0;i<26;i++) {
    fst.AddArc(start, StdArc(char(i+'a'), char(i+'a'), Weight::One(), states[i]));
  }
  Union(out, fst);
  made = true;
}

// parses stuff surrounded by stuff
const char *ParseAtom(const char *p, StdMutableFst* fst, char xMeaning) {
  if (p == NULL) return NULL;

  vector<char> chars;

  if (*p == 'P') {
    p = ParsePalindrome(p + 1, fst, xMeaning);
    if (p == NULL || *p != 'Q') return NULL;
    return p + 1;
  } else if (*p == 'B') {
    p = ParseExpr(p + 1, fst, xMeaning);
    if (p == NULL || *p != 'Y') return NULL;
    return p + 1;
  } else if (*p == 'Z') {
    int prev = *(p-1);
    if (prev >= 'A' && prev <= 'Z') {
      chars.push_back(prev + 'a' - 'A');
    }
    else {
      return NULL;
    }
  }
  else if (*p == 'O')
  {
    GetIncr(fst);
    return p + 1;
  }
  else if (*p == 'D')
  {
    GetSum42(fst);
    return p + 1;
  }
  else if (*p == 'R')
  {
    GetDbl(fst);
    return p + 1;
  }
  else if (*p == 'I')
  {
    GetCountries(fst);
    return p + 1;
  } else if (*p == 'X') {
    chars.push_back(xMeaning);
  }

    p = ParseCharClass(p, &chars);
    if (p == NULL) return NULL;

  State start = fst->AddState(), final = fst->AddState();
  fst->SetStart(start);
  fst->SetFinal(final, Weight::One());
  
    for (int i = 0; i < chars.size(); ++i)
      fst->AddArc(start, StdArc(chars[i], chars[i], Weight::One(), final));
  

    fst->AddArc(start, StdArc(' ', ' ', Weight::One(), start));
    fst->AddArc(final, StdArc(' ', ' ', Weight::One(), final));
  

  return p;
}

const char *ParseCharClass(const char *p, vector<char>* out) {
  if (p == NULL) return NULL;
  if ((*p >= 'a' && *p <= 'z')) {
    out->push_back(*p);
  } /*else if (*p == '-') {
    out->push_back(0);
    out->push_back(' ');
  } else if (*p == '.') {
    for (int ch = '0'; ch <= '9'; ++ch) out->push_back(ch);
    for (int ch = 'a'; ch <= 'z'; ++ch) out->push_back(ch);
    out->push_back(' ');
  } else if (*p == '_') {
    for (int ch = '0'; ch <= '9'; ++ch) out->push_back(ch);
    for (int ch = 'a'; ch <= 'z'; ++ch) out->push_back(ch);
  } else if (*p == '#') {
    for (int ch = '0'; ch <= '9'; ++ch) out->push_back(ch);
  }*/ else if (*p == 'A') {
    for (int ch = 'a'; ch <= 'z'; ++ch) out->push_back(ch);
  } else if (*p == 'C') {
    for (int ch = 'a'; ch <= 'z'; ++ch)
      if (!strchr("aeiou", ch)) out->push_back(ch);
  } else if (*p == 'V') {
    for (int ch = 'a'; ch <= 'z'; ++ch)
      if (strchr("aeiou", ch)) out->push_back(ch);
  }
  else if (*p == 'K')
  {
    for (int ch = 'a'; ch <= 'z'; ++ch)
      if (strchr("jkxqz", ch))
        out->push_back(ch);
  }
  else if (*p == 'U')
  {
    for (int ch = 'a'; ch <= 'z'; ++ch)
      if (strchr("nutrimatic", ch))
        out->push_back(ch);
  }
  else if (*p == 'M')
  {
    out->push_back('w');
  }
  else if (*p == 'W')
  {
    out->push_back('m');
  }
  else if (*p == 'S' || *p == 'J') {
    out->push_back(0);
  } else if (*p == 'E') {
    for (int ch = 'a'; ch <= 'z'; ++ch) {
      if (ch != 'e') {
        out->push_back(ch);
      }
    }
  }
  else if(*p != 'Z' && *p != 'X')
  {
    return NULL;
  }
  return p + 1;
}
