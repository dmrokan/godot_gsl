#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define ARGV_BOUND_DATA_COUNT 4

typedef enum GGSL_PARSER_STATE {
    S_IDLE,
    S_NUMBER,
    S_SEP,
} GGSL_PARSER_STATE;

typedef struct GGSL_BOUNDS {
    size_t r1, r2, c1, c2;
} GGSL_BOUNDS;

GGSL_BOUNDS argv_bounds_parse(const char *vn, size_t *size)
{
    GGSL_PARSER_STATE state = S_IDLE;
    size_t bounds[ARGV_BOUND_DATA_COUNT];
    bounds[0] = bounds[2] = 0;
    bounds[1] = size[0];
    bounds[3] = size[1];

    size_t len = strlen(vn);
    int l = 0;
    int l2 = 0;
    char *sub = (char*) calloc(len * sizeof(char), 0);
    for (int k = 0; k < len; k++)
    {
        char c = vn[k];

        if (state == S_IDLE)
        {
            if (c == '[')
            {
                state = S_NUMBER;
                continue;
            }
        }
        else if (state == S_NUMBER)
        {
            if (c >= '0' && c <= '9')
            {
                sub[l2++] = c;
                continue;
            }
            else if (c == ':' || c == ',')
            {
                if (strlen(sub) > 0)
                    bounds[l] = atoi(sub);
                l++;
                state = S_SEP;
                continue;
            }
            else if (c == ']')
            {
                if (strlen(sub) > 0)
                    bounds[l] = atoi(sub);
                state = S_IDLE;
                break;
            }
        }
        else if (state == S_SEP)
        {
            memset(sub, 0, len);
            l2 = 0;
            k--;
            state = S_NUMBER;
            continue;
        }
    }

    GGSL_BOUNDS _bounds;
    _bounds.r1 = bounds[0];
    _bounds.r2 = bounds[1];
    _bounds.c1 = bounds[2];
    _bounds.c2 = bounds[3];
    return _bounds;
}

int main(int argc, char **argv)
{
    size_t lala[] = { 100, 100, 100, 100 };
    GGSL_BOUNDS bounds = argv_bounds_parse(argv[1], &lala[0]);
    printf("bounds: %d %d %d %d\n", bounds.r1, bounds.r2, bounds.c1, bounds.c2);
    return 0;
}
