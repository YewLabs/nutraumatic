#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include <vector>
#include <limits.h>
const char *ParseOuter(const char *, fst::StdMutableFst *out);
const char *ParseExpr(const char *, fst::StdMutableFst* out, char xMeaning);
const char *ParseBranch(const char *, fst::StdMutableFst* out, char xMeaning);
const char *ParseFactor(const char *, fst::StdMutableFst *out, char xMeaning);
const char *ParsePossiblyInvertedPiece(const char *, fst::StdMutableFst *out, char xMeaning);
const char *ParsePiece(const char *, fst::StdMutableFst *out, char xMeaning);
const char *ParseAtom(const char *, fst::StdMutableFst* out, char xMeaning);
const char *ParseCharClass(const char *, std::vector<char>* out);

const char *ParsePalindrome(const char *, fst::StdMutableFst* out, char xMeaning);

void OptimizeExpr(fst::StdFst const& in, fst::StdMutableFst* out);

void IntersectExprs(
    std::vector<fst::StdVectorFst> const& in,
    fst::StdMutableFst* out);

class ExprFilter: public SearchFilter {
 public:
  ExprFilter(fst::StdFst const& parsed_expr);

  State start() const { return start_state; }

  bool is_accepting(State state) const {
    assert(state >= 0 && state < accepting.size());
    return accepting[state];
  }

  bool has_transition(State from, char ch, State *to) const {
    assert(from >= 0 && from < accepting.size());
    *to = next[(unsigned char) ch][from];
    return *to >= 0;
  }

 private:
  State start_state;
  std::vector<bool> accepting;
  std::vector<State> next[UCHAR_MAX + 1];
};
