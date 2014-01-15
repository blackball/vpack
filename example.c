/**
 * Unit test for vpack.
 *
 * @blackball
 */

#include "vpack.h"
#include <stdio.h>

const char *fmt_str = "cisfdc#";
struct model_t {
        char c;
        int i;
        short s;
        float f;
        double d;
        char name[10];
};
  
int main(int argc, char *argv[]) {
        struct model_t model = {'a', 23, 21, 1.4, 2.3, {'h', 'e', '\0'}};

        /* fill model */

        if (vpack_save("model.b", fmt_str, &(model.c), &(model.i), &(model.s), &(model.f), &(model.d), model.name, 10)) {
                fprintf(stderr, "Error: some error ocurs while saving model.\n");
                return -1;
        }

        /* when you need use model above */

        if (vpack_load("model.b", fmt_str, &(model.c), &(model.i), &(model.s), &(model.f), &(model.d), model.name, 10)) {
                fprintf(stderr, "Error: some error ocurs while reading model.\n");
                return -1;
        }

        /* use model */
        return 0;
}
