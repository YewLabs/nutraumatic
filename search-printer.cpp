#include "index.h"
#include "search.h"

#include <stdio.h>
#include <string.h>

void PrintAll(SearchDriver* d) {
  int count = 0;
  for (;;) {
    if (!(++count % 100000)) {
      printf("# %d\n", count);
      fflush(stdout);
    }
    if (d->step()) {
      if (d->text == NULL) break;
      if (d->score < 1e-4)
      {
        return;
      }
      int len = strlen(d->text);
      while (len > 0 && d->text[len - 1] == ' ') --len;
      printf("%.8g %.*s\n", d->score, len, d->text);
    }
  }
}
