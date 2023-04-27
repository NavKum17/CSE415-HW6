#include "parallelSpMV.h"
#include "sequentialSpMV.h"

int block_width, nrowblks, ncolblks;
seqblock *seqMatrixBlock;

parblock *parMatrixBlock;

int numrows, numcols, nnonzero;
int *colptrs, *irem;
float *xrem;

void readMatrix(char* filename)
{
    FILE *fp;

    fp = fopen(filename, "rb");
    if(fp == NULL)
    {
        printf("invalid matrix file name");
        exit(0);
    }

    fread(&numrows, sizeof(int), 1, fp);
    fread(&numcols, sizeof(int), 1, fp);
    fread(&nnonzero, sizeof(int), 1, fp);

    colptrs = (int *)malloc(sizeof(int)*(numcols+1));
    irem = (int *)malloc(sizeof(int)*nnonzero);
    xrem = (float *)malloc(sizeof(float)*nnonzero);

    fread(colptrs, sizeof(int), numcols+1, fp);
    fread(irem, sizeof(int), nnonzero, fp);
    fread(xrem, sizeof(float), nnonzero, fp);
}

int checkDifference(float *vector1, float *vector2, int N)
{
    int i;
    long double error_norm, vector_norm;

    error_norm = 0.0;
    vector_norm = 0.0;

    for (i = 0; i < N; i++) 
    {
        error_norm += (vector1[i] - vector2[i])*(vector1[i] - vector2[i]);
        vector_norm += vector2[i]*vector2[i];
    }

    error_norm = sqrt(error_norm);
    vector_norm = sqrt(vector_norm);

    if( error_norm/vector_norm > 0.01 )
    {
         fprintf(stderr, "relative error norm is too big ");
         fprintf(stderr, "error_norm = %.6Lf, ",  error_norm);
         fprintf(stderr, "vector_norm = %.6Lf\n", vector_norm);
         return 0;
    }

    return 1;
}

double getElapsed(struct timeval *start, struct timeval *end)
{
    double secs_used=(end->tv_sec - start->tv_sec); //avoid overflow by subtracting first
    double micros_used= ((secs_used*1000000) + end->tv_usec) - (start->tv_usec);

    return micros_used/1000000.0;
}

void initVector(float *vector, int M)
{
    int i;
    srand(time(NULL));
    for (i = 0; i < M; i++) 
    {
        vector[i] = rand()*1.0/INT_MAX;
    }
}

void clear_output_vector(float* vector,int N)
{
    int i;
    for (i = 0; i < N; i++) 
    {
        vector[i] = 0.0;
    }
}

int main(int argc, char *argv[])
{
    char *matrix_file;
    int i, j;
    struct timeval start, end;
    double read_time;
    double sequential_conversion_time, sequential_csc_time , sequential_csb_time;
    double parallel_conversion_time, parallel_csc_time , parallel_csb_time;
    float *input_vector;
    float *sequential_csc_output, *sequential_csb_output;
    float *parallel_csc_output, *parallel_csb_output;

    if(argc != 3)
    {
        fprintf(stderr, "\nERROR: This program can be executed by providing a matrix file and CSB block size, respectively, as commmand line argument.\n\n");
        return 0;
    }

    matrix_file = argv[1];
    block_width = atoi(argv[2]);

    gettimeofday(&start,NULL);
    readMatrix(matrix_file);
    gettimeofday(&end,NULL);
    read_time = getElapsed(&start,&end);
    printf("CSC matrix read time: %.3f sec\n", read_time);

    /*for(i=0; i<=numcols;i++)
        printf("%d ", colptrs[i]);
    printf("\n");
    for(i=0; i < nnonzero; i++)
        printf("%d ", irem[i]);
    printf("\n");
    for(i=0; i < nnonzero; i++)
        printf("%f ", xrem[i]);
    printf("\n");*/

    //perform and time sequential matrix conversion
    gettimeofday(&start,NULL);
    sequentialMatrixConversion();
    gettimeofday(&end,NULL);
    sequential_conversion_time = getElapsed(&start,&end);
    printf("sequential matrix conversion time: %.3f sec\n", sequential_conversion_time);

    /*for(i = 0; i < nrowblks; i++)
        for(j = 0; j < ncolblks; j++)
        {
            seqblock blk = seqMatrixBlock[i*ncolblks + j];
            printf("%d %d %d %d %d\n", i, j, blk.nnz, blk.roffset, blk.coffset);
            for(int k = 0; k < blk.nnz; k++)
                printf("%d ",blk.rloc[k]);
            printf("\n");
            for(int k = 0; k < blk.nnz; k++)
                printf("%d ",blk.cloc[k]);
            printf("\n");
            for(int k = 0; k < blk.nnz; k++)
                printf("%.2lf ",blk.val[k]);
            printf("\n");
        }*/

    //perform and time parallel matrix conversion
    gettimeofday(&start,NULL);
    parallelMatrixConversion();
    gettimeofday(&end,NULL);
    parallel_conversion_time = getElapsed(&start,&end);
    printf("parallel matrix conversion time: %.3f sec\n", parallel_conversion_time);

    input_vector = (float *)malloc(numcols * sizeof(float));
    sequential_csc_output = (float *)malloc(numrows * sizeof(float));
    sequential_csb_output = (float *)malloc(numrows * sizeof(float));
    parallel_csc_output = (float *)malloc(numrows * sizeof(float));
    parallel_csb_output = (float *)malloc(numrows * sizeof(float));

    initVector(input_vector,numcols);
    clear_output_vector(sequential_csc_output,numrows);
    clear_output_vector(sequential_csb_output,numrows);
    clear_output_vector(parallel_csc_output,numrows);
    clear_output_vector(parallel_csb_output,numrows);

    // perform and time sequential CSC SpMV
    gettimeofday(&start,NULL);
    sequentialCSC_SpMV(input_vector, sequential_csc_output);
    gettimeofday(&end,NULL);
    sequential_csc_time = getElapsed(&start,&end);
    printf("sequential CSC SpMV time: %.3f sec\n", sequential_csc_time);

    // perform and time sequential CSB SpMV
    gettimeofday(&start,NULL);
    sequentialCSB_SpMV(input_vector, sequential_csb_output);
    gettimeofday(&end,NULL);
    sequential_csb_time = getElapsed(&start,&end);
    printf("sequential CSB SpMV time: %.3f sec\n", sequential_csb_time);

    // perform and time sequential CSC SpMV
    gettimeofday(&start,NULL);
    parallelCSC_SpMV(input_vector, parallel_csc_output);
    gettimeofday(&end,NULL);
    parallel_csc_time = getElapsed(&start,&end);
    printf("parallel CSC SpMV time: %.3f sec\n", parallel_csc_time);

    // perform and time sequential CSB SpMV
    gettimeofday(&start,NULL);
    parallelCSB_SpMV(input_vector, parallel_csb_output);
    gettimeofday(&end,NULL);
    parallel_csb_time = getElapsed(&start,&end);
    printf("parallel CSB SpMV time: %.3f sec\n", parallel_csb_time);

    printf("Checking the difference between sequential CSC and sequential CSB SpMV outputs: ");
    printf("%d\n",checkDifference(sequential_csc_output, sequential_csb_output, numrows));

    printf("Checking the difference between sequential CSC and parallel CSC SpMV outputs: ");
    printf("%d\n",checkDifference(sequential_csc_output, parallel_csc_output, numrows));

    printf("Checking the difference between sequential CSC and parallel CSB SpMV outputs: ");
    printf("%d\n",checkDifference(sequential_csc_output, parallel_csb_output, numrows));

    printf("Achieved speedup of parallel matrix conversion over sequential one = %.2f\n", sequential_conversion_time/parallel_conversion_time);
    printf("Achieved speedup of sequential CSB SpMV over sequential CSC SpMV = %.2f\n", sequential_csc_time/sequential_csb_time);
    printf("Achieved speedup of parallel CSC SpMV over sequential CSC SpMV = %.2f\n", sequential_csc_time/parallel_csc_time);
    printf("Achieved speedup of parallel CSB SpMV over sequential CSC SpMV = %.2f\n", sequential_csc_time/parallel_csb_time);

    //destructors
    free(input_vector);
    free(sequential_csc_output);
    free(sequential_csb_output);

    free(colptrs);
    free(irem);
    free(xrem);

    for(i = 0; i < nrowblks; i++)
    {
        for(j = 0; j < ncolblks; j++)
        {
            if(seqMatrixBlock[i * ncolblks + j].nnz > 0)
            {
                free(seqMatrixBlock[i * ncolblks + j].rloc);
                free(seqMatrixBlock[i * ncolblks + j].cloc);
                free(seqMatrixBlock[i * ncolblks + j].val);
            }
            if(parMatrixBlock[i * ncolblks + j].nnz > 0)
            {
                free(parMatrixBlock[i * ncolblks + j].rloc);
                free(parMatrixBlock[i * ncolblks + j].cloc);
                free(parMatrixBlock[i * ncolblks + j].val);
            }
        }
    }
    free(seqMatrixBlock);
    free(parMatrixBlock);

    return 0;
}
