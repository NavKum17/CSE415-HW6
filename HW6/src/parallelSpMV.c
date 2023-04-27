#include "parallelSpMV.h"

int t = 40;

void parallelMatrixConversion()
{
    int i, r, c, k, k1, k2, blkr, blkc;
    int **top;

    //set number of row blocks and column blocks
    nrowblks = ceil(numrows/(double)(block_width));
    ncolblks = ceil(numcols/(double)(block_width));

    //allocate memory 
    parMatrixBlock = (parblock *)malloc(nrowblks*ncolblks * sizeof(parblock));

    //top[i][j] is the counter that will be used for
    //total assigned nonzeros to block i,j
    top = (int **)malloc(nrowblks * sizeof(int *));
    for( i = 0 ; i < nrowblks; i++ )
        top[i] = (int *)malloc(ncolblks * sizeof(int));

    //initialization
    #pragma omp parallel for num_threads(t) shared(parMatrixBlock, top) private(blkr, blkc)
    for(blkr = 0 ; blkr < nrowblks ; blkr++)
    {
        for(blkc = 0 ; blkc < ncolblks ; blkc++)
        {
            top[blkr][blkc] = 0;
            parMatrixBlock[blkr * ncolblks + blkc].nnz = 0;
        }
    }
    //calculating nnz per block
    #pragma omp parallel for num_threads(t) shared(parMatrixBlock) private(c, k)
    for(c = 0 ; c < numcols ; c++)
    {
        k1 = colptrs[c];
        k2 = colptrs[c + 1] - 1;
        blkc = ceil((c + 1)/(double)block_width);
        for(k = k1 - 1 ; k < k2 ; k++)
        {
            r = irem[k];
            blkr = ceil(r/(double)block_width);
            parMatrixBlock[(blkr - 1) * ncolblks + (blkc - 1)].nnz++;
        }
    }

    //allocating memory based on nonzero counts of each block
    #pragma omp parallel for num_threads(t) shared(parMatrixBlock, block_width, ncolblks) private(blkr, blkc)
    for(blkr = 0 ; blkr < nrowblks ; blkr++)
    {
        for(blkc = 0 ; blkc < ncolblks ; blkc++)
        {
            parMatrixBlock[blkr * ncolblks + blkc].roffset = blkr * block_width;
            parMatrixBlock[blkr * ncolblks + blkc].coffset = blkc * block_width;

            if(parMatrixBlock[blkr * ncolblks + blkc].nnz>0)
            {
                parMatrixBlock[blkr * ncolblks + blkc].rloc = (unsigned short int *)malloc(parMatrixBlock[blkr * ncolblks + blkc].nnz * sizeof(unsigned short int));
                parMatrixBlock[blkr * ncolblks + blkc].cloc = (unsigned short int *)malloc(parMatrixBlock[blkr * ncolblks + blkc].nnz * sizeof(unsigned short int));
                parMatrixBlock[blkr * ncolblks + blkc].val = (float *)malloc(parMatrixBlock[blkr * ncolblks + blkc].nnz * sizeof(float));
            }
            else
            {
                parMatrixBlock[blkr * ncolblks + blkc].rloc = NULL;
                parMatrixBlock[blkr * ncolblks + blkc].cloc = NULL;
            }
        }
    }

    //assigning each nonzero on CSC matrix to its corresponding position on CSB matrix
    #pragma omp parallel for num_threads(t) shared(parMatrixBlock, irem, xrem, colptrs, ncolblks, top, numcols) private(k1, k, k2, blkc, blkr, r, c)
    for(c = 0 ; c < numcols ; c++)
    {
        k1 = colptrs[c];
        k2 = colptrs[c + 1] - 1;
        blkc = ceil((c + 1)/(double)block_width);

        for(k = k1 - 1 ; k < k2 ; k++)
        {
            r = irem[k];
            blkr = ceil(r/(double)block_width);

            parMatrixBlock[(blkr - 1) * ncolblks + blkc - 1].rloc[top[blkr - 1][blkc - 1]] = r - parMatrixBlock[(blkr - 1) * ncolblks + blkc - 1].roffset;
            parMatrixBlock[(blkr - 1) * ncolblks + blkc - 1].cloc[top[blkr - 1][blkc - 1]] = (c + 1) -  parMatrixBlock[(blkr - 1) * ncolblks + blkc - 1].coffset;
            parMatrixBlock[(blkr - 1) * ncolblks + blkc - 1].val[top[blkr - 1][blkc - 1]] = xrem[k];

            top[blkr - 1][blkc - 1] = top[blkr - 1][blkc - 1] + 1;
        }
    }

    //uncomment this part to see if your matrix conversion
    //function works on small inputs
    /*for(i = 0; i < nrowblks; i++)
    {
        for(int j = 0; j < ncolblks; j++)
        {
            parblock blk = parMatrixBlock[i * ncolblks + j];
            for(int k = 0; k < blk.nnz; k++)
            {
                printf("%d %d %f\n", blk.rloc[k]+blk.roffset, blk.cloc[k]+blk.coffset, blk.val[k]);
            }
        }
    }*/

    for(i = 0 ; i < nrowblks ; i++)
    {
        free(top[i]);
    }
    free(top);
}

void parallelCSC_SpMV(float *x, float *b)
{
    int i, j;
    #pragma omp parallel for num_threads(t) private(i,j) shared(x, b, colptrs, irem, xrem)   
    for(i = 0; i < numcols; i++)
    {
        for(j = colptrs[i] - 1; j < colptrs[i+1] - 1; j++)
        {
            b[irem[j] - 1] += xrem[j]*x[i];
        }
    }
}

void parallelCSB_SpMV(float *x, float *b)
{
    int i, j, k;
    parblock blk;
    #pragma omp parallel for num_threads(t) private(i,j,k,blk) shared(x,b,parMatrixBlock,nrowblks,ncolblks)
    for(i = 0; i < nrowblks; i++)
    {
        for(j = 0; j < ncolblks; j++)
        {
            blk = parMatrixBlock[i * ncolblks + j];
	    
            for(k = 0; k < blk.nnz; k++)
            {
                b[ blk.rloc[k] + blk.roffset - 1 ] += blk.val[k] * x[ blk.cloc[k] + blk.coffset - 1];
            }
        }
    }
}

