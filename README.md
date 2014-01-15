vpack 

vpack is a simple plain data (un)serilization lib.

```c
struct model_t {
       int i;
       double x[10];
};

static const char *fmtstr = "id#";

struct model_t model = {0};

/* fulfill model */

/* save model into binary file */
vpack_save("model.db", fmtstr, &(model.i), model.x, 10);

/* load model.db into model */
vpack_load("model.db", fmtstr, &(model.i), model.x, 10);

```

The vpack.c file was documented with comments.
