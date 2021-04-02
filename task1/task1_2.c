#include "stdio.h"
#include "mpi.h"
#include "math.h"
#include "string.h"
#include "time.h"


#define EPS = 10E-6
#define zero(x) ((x < EPS || x > -EPS) ? x : 0.0)


double a = 0.0, b = 2.0;
double calc(unsigned long long order, double left, double h, double (*func)(double)) {
    return func(left+h*order)*h;
}

double fc(double x) {
    return sqrt(4-x*x);
}

int main(int argc, char** argv, char ** envp) {
    unsigned long long N = 0;
    char * pEnd;
    if (argc > 1) {
        N = strtoull(argv[1], &pEnd, 10);
    } else {
        printf("Not enough parameters provided\n");
        return 1;
    }

    clock_t begin = clock();

    MPI_Init(&argc, &argv);
    int numtasks, rank;
    unsigned long long i;
    double Sum = 0.0;
    double h = (b-a)/N;

    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    for (i = rank+1; i < N; i+=numtasks)
        Sum += calc(i, a, h, &fc);
    if (rank != 0) {
        double buf[1];
        buf[0] = Sum;
        //printf("My rank = %d, my sum = %lf\n", rank, Sum);
        MPI_Bsend(buf, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    } else {
        for (i = 0; i < numtasks-1; i++) {
            double buf[1];
            MPI_Recv(buf, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            Sum += buf[0];
            Sum += (calc(0, a, h, &fc)+calc(N, a, h, &fc))/2;
        }
        clock_t end = clock();
        printf("%lf ", Sum);
        printf("%lf\n", (double)(end - begin) / CLOCKS_PER_SEC);
    }
    MPI_Finalize();
    return 0;
}
