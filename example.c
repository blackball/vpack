/**
 * Unit test for vpack.
 *
 * @blackball
 */

#include "vpack.h"
#include <stdio.h>

struct Model
{
  char c;
  int pts[4];
  float vec[1024];
};
  
int main(int argc, char *argv[])
{
  struct Model model = {0};

  /* fill model */

  if (vpack("model.b", "ci#f#", &(model.c), model->pts, 4, model->vec, 1024))
  {
    fprintf(stderr, "Error: some error ocurs while saving model.\n");
    return -1;
  }

  /* when you need use model above */

  if (vget("model.b", "ci#f#", &(model.c), model->pts, 4, model->vec, 1024))
  {
    fprintf(stderr, "Error: some error ocurs while reading model.\n");
    return -1;
  }

  /* use model */
  
  return 0;
}
