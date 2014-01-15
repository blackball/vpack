#include "vpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

EXTERN_BEGIN

/**
 *  length of format item,
 *  means c,i,f,d,c#,i#... means one item
 */
#define V_VAR_LEN 64

/**
 * Since I just support CHAR_BIT == 8,
 * I could gain more confidence to have a
 * check flag to check if the loaded file
 * is suitable on new system.
 *
 * we should handle:
 * 0. endianess
 * 1. bit length of int,short,long.(long long is not supported)
 */

static char
vpack_getendian() {
        int i = 1;
        return ( (*(char*)&i) == 0 ) ? 0xff : 0x00;
}

/**
 * guard to protect the consistant of IO in different places
 * 0. sizeof type is equal ?
 * 1. endianess is the same?
 * 2. used or not ?
 */

/*
    char vguard[8] = {
    sizeof(char),
    sizeof(short),
    sizeof(int),
    sizeof(long),
    sizeof(float),
    sizeof(double),
    vgetendian,
    0, // ending flag.
    };
*/

struct _vguard {
        char flags[8];
};


static int
vpack_checkguard(struct _vguard *file, struct _vguard *local) {
        int i;
        /* 0. endianess */
        if (file->flags[6] != local->flags[6])
                return -1;

        /* type used, if used but not equal in size, false */
        for (i = 0; i < 6; ++i)
                if (file->flags[i] != 0 &&
                    file->flags[i] != local->flags[i])
                        return -1;

        return 0;
}


static int
vpack_getfuncidx(const char c) {
        switch(c) {
        case 'c': return 0;
        case 's': return 1;
        case 'i': return 2;
        case 'l': return 3;
        case 'f': return 4;
        case 'd': return 5;
        }
        // error
        return -1;
}

/**
 * copy data.
 *
 * @param dst destination mem block
 * @param src data source
 * @param n length of src, >= 1
 * @param incrwho 0->dst 1->src
 */

#define VPACK_MAKECPFUNC(suffix, type)                                      \
        static void vpack_cp##suffix(void **dst, void **src, int n, int icrwho) { \
                type *pd = (type*)(*dst);                               \
                type *ps = (type*)(*src);                               \
                memcpy(*dst, *src, sizeof(type)*n);                     \
                if (icrwho) /* == 1*/                                   \
                        *src = (void*)(ps + n);                         \
                else                                                    \
                        *dst = (void*)(pd + n);                         \
        }

VPACK_MAKECPFUNC(char,   char)
VPACK_MAKECPFUNC(short,  short)
VPACK_MAKECPFUNC(int,    int)
VPACK_MAKECPFUNC(long,   long)
VPACK_MAKECPFUNC(float,  float)
VPACK_MAKECPFUNC(double, double)

#undef VPACK_MAKECPFUNC

typedef void (* vpack_cpfunc)(void **, void **, int, int);

/*
  #define _vpack_cpfunc(name, id) &vpack_cp##name
  static vpack_cpfunc vfunctab[] =
  {
  _vpack_cpfunc(char,   0),
  _vpack_cpfunc(short,  1),
  _vpack_cpfunc(int,    2),
  _vpack_cpfunc(long,   3),
  _vpack_cpfunc(float,  4),
  _vpack_cpfunc(double, 5),
  };
  #undef _vpack_cpfunc
*/

/* get last non-blank char*/
#define VPACK_SKIP(c)                          \
        while(1){                               \
                switch(*c) {                    \
                case ' ':                       \
                case '\n':                      \
                case '\t':                      \
                case '\v':                      \
                case '\f':                      \
                case '\r': ++c; continue;       \
                }                               \
                break;                          \
        }

/**
 * [func][addr][len]
 */
struct _vprocitem {
        vpack_cpfunc func;
        void *pdata;
        int n;
};

struct _vproctab {
        int cnt;
        int memsz;
        struct _vguard guard;
        struct _vprocitem items[V_VAR_LEN];
};


/**
 * Add one item into proccess tab.
 *
 * @param tab pointer to proctab.
 * @param c type flag
 * @param data pointer to data
 * @param len of data, >= 1
 * @return 0 if OK, else -1
 */
static int
vpack_additem(struct _vproctab *tab, const char c, void *data, int n) {
        int icnt;
        int sz = -1;
        int gidx = -1;
        vpack_cpfunc func = NULL;

        switch(c) {
        case 'c': gidx = 0; sz = sizeof(char);   func = &vpack_cpchar;   break;
        case 's': gidx = 1; sz = sizeof(short);  func = &vpack_cpshort;  break;
        case 'i': gidx = 2; sz = sizeof(int);    func = &vpack_cpint;    break;
        case 'l': gidx = 3; sz = sizeof(long);   func = &vpack_cplong;   break;
        case 'f': gidx = 4; sz = sizeof(float);  func = &vpack_cpfloat;  break;
        case 'd': gidx = 5; sz = sizeof(double); func = &vpack_cpdouble; break;
        default: return -1; /* error */
        }

        icnt = tab->cnt;
        assert(icnt+1 < V_VAR_LEN);

        tab->memsz += sz*(n);
        tab->guard.flags[gidx] |= sz;

        tab->items[icnt].func = func;
        tab->items[icnt].pdata = data;
        tab->items[icnt].n = n;

        ++ tab->cnt;

        return 0;
}

/**
 * pre-parse the format string from head to tail
 * to get needed information. this will skip all
 * spaces including:
 * ' ', '\n', '\t', '\v', '\f', '\r'
 *
 * @param fmt format string
 * @param ap va_list pointing to the first var.
 *
 * @return 0 if fmt is valid, else -1
 */
static int
vpack_preparse(struct _vproctab *tab, const char *fmt, va_list ap) {
        char c;
        void *data;
        int len = 0;

        const char *pc = fmt;

        /* endianess, @TODO 6 is not a good way to make a extend */
        tab->guard.flags[6] = vpack_getendian();

        while (*pc != '\0')
        {
                VPACK_SKIP(pc);
                c = *pc;

                switch(c)
                {
                case 'c':
                case 's':
                case 'i':
                case 'l':
                case 'f':
                case 'd':
                        data = va_arg(ap, void*);
                        len = (*(pc+1) == '#' ? pc ++, va_arg(ap,int): 1);
                        vpack_additem(tab, c, data, len);
                        break;

                default:
                        /* error, @TODO give more msg */
                        assert(0);
                        break;
                }

                ++ pc;
        }
        return 0;
}

#undef VPACK_SKIP

/**
 * fmt string is very flexible.
 *
 * First, parse the format string, gain information to
 * allocate whole memory, and copy data into memory.
 * we support char, short, int, long, float, double;
 * the abbreviations is: c,s,i,l,f,d.
 * and Array is supported,using "type#", means,
 * an type type array, which has len elements.nested structs
 * is not supported.
 *
 * @param fn filename that will create to store data
 * @param fmt format string
 * @return 0 if OK, else -1
 */

int
vpack_save(const char *fn, const char* fmt, ...) {
        FILE *f = NULL;
        void *mem = NULL, *p = NULL, *tp = NULL;
        struct _vproctab vproctab = {0};
        int i;

        va_list ap;
        va_start(ap, fmt);
        vpack_preparse(&vproctab, fmt, ap);
        va_end(ap);

        /* pack header */
        mem = malloc(vproctab.memsz + sizeof(char)*8);
        assert(mem != NULL);
        p = mem;

        tp = (vproctab.guard.flags);

        vpack_cpchar(&p, &tp, 8, 0);

        /* pack data */
        for (i = 0; i < vproctab.cnt; ++i)
                vproctab.items[i].func(&p,
                                       (void**)(&(vproctab.items[i].pdata)),
                                       vproctab.items[i].n, 0);

        f = fopen(fn, "wb");
        if (f==NULL)
        {
                /* @TODO give more msg */
                free(mem);
                return -1;
        }

        fwrite(mem, vproctab.memsz + sizeof(char)*8, 1, f);

        fclose(f);
        free(mem);

        return 0;
}

/**
 * Give the filename, and the format string used
 * when pack the file, we could load the file correctly,
 * if the platform is different, and we that won't hurt,
 * then warning will give, if it really hurts, it will
 * failed and will raise a runtime error and information.
 */

int
vpack_load(const char *fn, const char *fmt, ...) {
        int i;
        FILE *f;
        void *mem = NULL,
                *p = NULL;
        va_list ap;

        struct _vproctab vproctab = {0}; /* parse from format string */
        struct _vguard fileguard = {{0}};  /* read from file */
        struct _vguard local ={
                {
                        sizeof(char),
                        sizeof(short),
                        sizeof(int),
                        sizeof(long),
                        sizeof(float),
                        sizeof(double),
                        vpack_getendian(),
                        0,
                }
        };

        /* read data */
        va_start(ap, fmt);
        vpack_preparse(&vproctab, fmt, ap);
        va_end(ap);

        /* read & check  header */
        f = fopen(fn, "rb");
        assert(f != NULL);
        fread(fileguard.flags, sizeof(char), 8, f);

        if (vpack_checkguard(&(vproctab.guard), &local) &&
            vpack_checkguard(&(vproctab.guard), &fileguard))
        {
                /* platform incompatible ? */
                /* @TODO give more msg */
                fclose(f);
                return -1;
        }

        mem = malloc(vproctab.memsz);
        assert(mem != NULL);
        p = mem;

        fread(p, vproctab.memsz, 1, f);
        //p = (void*)(((char*)p) + 8);

        for (i = 0; i < vproctab.cnt; ++i)
                vproctab.items[i].func((void**)(&(vproctab.items[i].pdata)),
                                       &p,
                                       vproctab.items[i].n, 1);

        fclose(f);
        free(mem);

        return 0;
}

EXTERN_END
