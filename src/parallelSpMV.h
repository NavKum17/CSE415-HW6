#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#include <sys/time.h> // for clock_gettime()

typedef struct parblock
{
    int nnz;
    int roffset, coffset;
    unsigned short int *rloc, *cloc;
    float *val;
}parblock;

extern int block_width, nrowblks, ncolblks;
extern parblock *parMatrixBlock;

extern int numrows, numcols, nnonzero;
extern int *colptrs, *irem;
extern float *xrem;

void parallelMatrixConversion();

void parallelCSC_SpMV(float *, float *);

void parallelCSB_SpMV(float *, float *);
