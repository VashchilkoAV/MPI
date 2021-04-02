#include "stdio.h"
#include "mpi.h"
#include "math.h"
#include "string.h"
#include "time.h"
#include "stdlib.h"

#define TAG 0
#define EPS 10E-6
#define zero(x) ((x < EPS || x > -EPS) ? x : 0.0)


double a = 0., b = 1., l = 1., c = 1.;

double mult = 0.3; 

double calc(double u0, double u1, double u2) {
    return u1 + mult*(u2-2*u1+u0);
}


int main(int argc, char** argv, char ** envp) {
    //collecting params
    unsigned long long NumSections = 0, T = 0;
    char * pEnd;
    if (argc > 2) {
        NumSections = strtoull(argv[1], &pEnd, 10);
        T = strtoull(argv[2], &pEnd, 10);
    } else {
        printf("Not enough parameters provided\n");
        return 1;
    }
    unsigned long long NumDots = NumSections-1;
    double starttime, endtime;
    int numtasks, rank;
    unsigned long long i;
    double h = l/NumSections;
    double thau = mult*h*h/(c*c);
    unsigned long long timetick = 0, timebound = (unsigned long long)ceil(T/thau);


    //starting up MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    //making lists
    unsigned long long netsize = NumDots % numtasks == 0 ? NumDots/numtasks+2 : NumDots/numtasks+3;
    unsigned long long lastSectionRealSize = NumDots % numtasks == 0 ? netsize : NumDots % (netsize-2) + 2;
    //printf("%d %d\n", netsize, lastSectionRealSize);
    double * u0 = (double*)malloc(netsize*sizeof(double));
    double * u1 = (double*)malloc(netsize*sizeof(double));
    memset(u0, 0, netsize*sizeof(double));
    memset(u1, 0, netsize*sizeof(double));
    /*
    for (i = 0; i < netsize; i++) {
        u0[i] = 0;
        u1[i] = 0;
    }
    */
    if (rank == 0) {
        u0[0] = a;
        u1[0] = a;
    }
    if (rank == numtasks-1) {
        u0[lastSectionRealSize-1] = b;
        u1[lastSectionRealSize-1] = b;
    }
    double * tmp = NULL;


    //main loop
    starttime = MPI_Wtime();
    for (timetick; timetick < timebound; timetick++) {
        if (numtasks != 1) { 
            if (rank == 0) {
                MPI_Send(&u0[netsize-2], 1, MPI_DOUBLE, rank+1, TAG, MPI_COMM_WORLD);
                MPI_Recv(&u0[netsize-1], 1, MPI_DOUBLE, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (rank == numtasks - 1) {
                if (rank % 2 == 0){
                    MPI_Send(&u0[1], 1, MPI_DOUBLE, rank-1, TAG, MPI_COMM_WORLD);
                    MPI_Recv(&u0[0], 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&u0[0], 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(&u0[1], 1, MPI_DOUBLE, rank-1, TAG, MPI_COMM_WORLD);
                }
            } else if (rank % 2 == 0) {
                MPI_Send(&u0[1], 1, MPI_DOUBLE, rank-1, TAG, MPI_COMM_WORLD);
                MPI_Send(&u0[netsize-2], 1, MPI_DOUBLE, rank+1, TAG, MPI_COMM_WORLD);
                MPI_Recv(&u0[0], 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&u0[netsize-1], 1, MPI_DOUBLE, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(&u0[0], 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&u0[netsize-1], 1, MPI_DOUBLE, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&u0[netsize-2], 1, MPI_DOUBLE, rank+1, TAG, MPI_COMM_WORLD);
                MPI_Send(&u0[0], 1, MPI_DOUBLE, rank-1, TAG, MPI_COMM_WORLD);
            }
        }
        if (rank != numtasks-1) {
            for (i = 1; i < netsize-1; i++) {
                u1[i] = calc(u0[i-1], u0[i], u0[i+1]);
            }
        } else {
            for (i = 1; i < lastSectionRealSize-1; i++) {
                u1[i] = calc(u0[i-1], u0[i], u0[i+1]);
            }
        }
        //printf("%lf\n", u1[1]);
        tmp = u0;
        u0 = u1;
        u1 = tmp;
    }
    
    //collecting results
    
    if (rank != 0 && rank != numtasks-1) {
        MPI_Send(&u0[1], netsize-2, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD);
    } else if (rank == numtasks-1 && numtasks != 1) {
        MPI_Send(&u0[1], lastSectionRealSize-2, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD);
    } else {
        double * buf = (double*)malloc(NumDots*2*sizeof(double));
        buf[0] = a;
        buf[NumDots+1] = b;
        memcpy(buf+1, u0+1, (netsize-2)*sizeof(double));
        if (numtasks > 1) {
            for (i = 1; i < numtasks; i++) {
                MPI_Recv(&buf[1+i*(netsize-2)], netsize-2, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        FILE * f = fopen("u0ult.txt", "w");
        for (i = 0; i < NumDots+2; i++) {
            fprintf(f, "%lf %lf\n", i*l/NumSections,  buf[i]);
        }
        fclose(f);
        free(buf);
        endtime = MPI_Wtime();
        printf("%lf\n", endtime-starttime);
    }   


    free(u0);
    free(u1);
    
    MPI_Finalize();
    return 0;
}
