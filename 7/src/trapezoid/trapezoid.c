#include "stdlib.h"
#include "stdio.h"
#include "mpi.h"
#include "math.h"


double Trap(double l, double r, int count, double h) 
{
    double estimate, x;
    int i;

    estimate = (exp(l) + exp(r))/2.0;  

    for (i = 1; i <= count-1; i++) {
        x = l + i * h;
        estimate += exp(x);
    }

    estimate = estimate * h;

    return estimate;
}


double p2p_reduce(double a, double b, int n, int my_rank, int comm_sz)
{
    int local_n, source;
    double h, local_a, local_b;
    double local_int, total_int;

    // h will be the same for all processes 

    // determine local_a and local_b values for each MPI process
    // for simplicity, you can assume n to be a perfect multiple of comm_sz

    // aggregate the partial results at the root (MPI rank = 0) process
    // using point-to-point communication primitives, i.e. MPI_Send and MPI_Recvs

    return total_int;
}


double coll_reduce(double a, double b, int n, int my_rank, int comm_sz)
{
    int local_n, source;
    double h, local_a, local_b;
    double local_int, total_int;

    // h will be the same for all processes 

    // determine local_a and local_b values for each MPI process
    // for simplicity, you can assume n to be a perfect multiple of comm_sz

    // aggregate the partial results at the root (MPI rank = 0) process
    // using MPI collective operation(s).

    return total_int;
}


int main(int argc, char* argv[])
{
    int my_rank, comm_sz, n;
    int trial, ntrials = 10;
    double a, b;
    double p2p_integral, coll_integral, solution;
    double t_start, t_end;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    if (argc == 4) {
        // get the command line arguments                               
        a = strtod(argv[1], NULL);
        b = strtod(argv[2], NULL);
        n = strtol(argv[3], NULL, 10);

    }
    else {
        fprintf(stderr, "\nERROR: This program requires 3 command-line inputs:\n");
        fprintf(stderr, "         a: starting point for the integral\n");
        fprintf(stderr, "         b: end point for the integral\n");
        fprintf(stderr, "         n: number of sample points\n");
        exit(1);
    }

    if (my_rank == 0) {
        printf("#procs = %d\n", comm_sz);
        printf("#trapezoids = %d\n", n);
        printf("Interval %f to %f\n", a, b);
        solution = exp(b) - exp(a);
        printf("Analytic solution = %.15e\n", solution);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    t_start = MPI_Wtime();
    for (trial = 0; trial < ntrials; ++trial)
        p2p_integral = p2p_reduce(a, b, n, my_rank, comm_sz);
    t_end = MPI_Wtime();

    if (my_rank == 0) {
        if (p2p_integral >= 0.99*solution && p2p_integral <= 1.01 * solution) {
            printf("\n\nRIGHT Point-to-Point SOLUTION!\n");
            printf("Integral = %.15e\n", p2p_integral);
            printf("Time = %.6f\n", (t_end - t_start)/ntrials);
        }
        else
            printf("\n\nWRONG Point-to-Point SOLUTION!\n");
    }

    MPI_Barrier(MPI_COMM_WORLD);

    t_start = MPI_Wtime();
    for (trial = 0; trial < ntrials; ++trial)
        coll_integral = coll_reduce(a, b, n, my_rank, comm_sz);
    t_end = MPI_Wtime();

    if (my_rank == 0) {
        if (coll_integral >= 0.99*solution && coll_integral <= 1.01 * solution) {
            printf("\n\nRIGHT Collectives SOLUTION!\n");
            printf("Integral = %.15e\n", coll_integral);
            printf("Time = %.6f\n", (t_end - t_start)/ntrials);
        }
        else
            printf("\n\nWRONG Collectives SOLUTION!\n");
    }

    MPI_Finalize();
    return 0;
} //end main 
