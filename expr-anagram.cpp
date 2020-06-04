#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/arcsort.h"
#include "fst/closure.h"
#include "fst/concat.h"
#include "fst/equivalent.h"
#include "fst/intersect.h"
#include "fst/union.h"

#include <stdio.h>

#include <vector>
#include <utility>

using namespace fst;


static void GetPal(StdMutableFst* out) {
  static StdVectorFst res;
  static bool computed = false;
  if (computed) {
    *out = res;
    return;
  }
  // fprintf(stderr,"h1\n");

  StdVectorFst onechar;
  StdVectorFst singles[26];
  ParseAtom("A", &onechar, 'a');
  for (char ch = 'a'; ch <= 'z'; ++ch) {
    ParseAtom(&ch, &singles[ch-'a'],'a');
  }
  // fprintf(stderr, "h2\n");

  for (int len = 2; len < 7; ++len)
  {
    vector<StdVectorFst> to_intersect;
    // fprintf(stderr, "h3 %d\n", len);

    for (int i = 0; i * 2 + 1 < len; i++)
    {
      StdVectorFst column;
      for (int ch = 0;ch<26; ++ch)
      {
        StdVectorFst poss;
        poss.SetStart(poss.AddState());
        poss.SetFinal(poss.Start(), StdArc::Weight::One());

        for (int j = 0; j < i; ++j)
          Concat(&poss, onechar);
        Concat(&poss, singles[ch]);
        for (int j = i + 1; j < len - i - 1; ++j)
          Concat(&poss, onechar);
        Concat(&poss, singles[ch]);
        for (int j = len-i; j < len; ++j)
          Concat(&poss, onechar);

        Union(&column, poss);
      }
      to_intersect.push_back(column);
    }
    StdVectorFst v = to_intersect[0];
    for (auto it = to_intersect.begin();it != to_intersect.end(); ++it) {}
    IntersectExprs(to_intersect, &v);
    Union(&res, v);
  }

  Union(&res, onechar);

  *out = res;
}

static void MakeExpr(vector<StdVectorFst> const& parts, StdMutableFst* out) {
  StdVectorFst pal_parts, all_pals;
  pal_parts.SetStart(pal_parts.AddState());
  pal_parts.SetFinal(pal_parts.Start(), StdArc::Weight::One());

  for (size_t i = 0; i < parts.size(); ++i)
  {
    Concat(&pal_parts, parts[i]);
    if (getenv("DEBUG_FST") != NULL)
    {
      fprintf(stderr, "part %ld ", i);
    }
  }
  if (parts.size() > 1) {
    for (size_t i = parts.size()-1; i-- > 0;)
    {
      StdVectorFst reversed;
      Reverse(parts[i], &reversed);
      Concat(&pal_parts, reversed);
      if (getenv("DEBUG_FST") != NULL)
        fprintf(stderr, "part %ld ", i);
    }
  }
  if (getenv("DEBUG_FST") != NULL)
    fprintf(stderr, "\n");

  GetPal(&all_pals);
  vector<StdVectorFst> to_intersect;
  to_intersect.push_back(all_pals);
  to_intersect.push_back(pal_parts);
  // Union(out, pal_parts);
  IntersectExprs(to_intersect, out);
}

const char *ParsePalindrome(const char *p, StdMutableFst* out, char xMeaning) {
  if (p == NULL) return NULL;

  vector<StdVectorFst> parts;
  while (*p != 'Q') {
    StdVectorFst expr;
    p = ParsePossiblyInvertedPiece(p, &expr, xMeaning);
    if (p == NULL) return NULL;
    parts.push_back(expr);
  }

  if (getenv("DEBUG_FST") != NULL) {
    fprintf(stderr, "anagram: %zd unique parts\n", parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
      fprintf(stderr, "  #%zu: %d states\n", i,
          parts[i].NumStates());
    }
  }

  MakeExpr(parts, out);
  return p;
}
