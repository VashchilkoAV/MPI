#include "stdio.h"
#include "omp.h"

#include "math.h"
#include "string.h"
#include "stdlib.h"

#define TAG 0
#define EPS 10E-6
#define zero(x) ((x < EPS || x > -EPS) ? x : 0.0)


double a = 1., b = 2., l = 1., c = 1.;

double mult = 0.3; 

double calc(double u0, double u1, double u2) {
    return u1 + mult*(u2-2*u1+u0);
}


int main(int argc, char** argv, char ** envp) {
    //collecting params
    unsigned long long NumSections = 0, num_threads = 1;
    double T = 0.;
    char * pEnd;
    if (argc > 3) {
        num_threads = strtoull(argv[1], &pEnd, 10);
        NumSections = strtoull(argv[2], &pEnd, 10);
        T = strtod(argv[3], &pEnd);
    } else {
        printf("Not enough parameters provided\n");
        return 1;
    }
    unsigned long long i;
    double h = l/NumSections;
    double thau = mult*h*h/(c*c);
    unsigned long long timetick = 0, timebound = (unsigned long long)ceil(T/thau);

    double * u0 = (double*)malloc((NumSections+1)*sizeof(double));
    double * u1 = (double*)malloc((NumSections+1)*sizeof(double));
    memset(u0, 0, (NumSections+1)*sizeof(double));
    memset(u1, 0, (NumSections+1)*sizeof(double));
    u0[0] = a;
    u1[0] = a;
    u0[NumSections] = b;
    u1[NumSections] = b;
    double * tmp = NULL;

#pragma omp parallel num_threads(num_threads) private(timetick)
    {
        for (timetick; timetick*thau < T; timetick++) {
#pragma omp for schedule(dynamic, 5000)
            for (i = 1; i < NumSections; i++) {
                u1[i] = calc(u0[i-1], u0[i], u0[i+1]);
            }
#pragma omp master
            {
                tmp = u0;
                u0 = u1;
                u1 = tmp;
            }
#pragma omp barrier
        }
    }

    //FILE * f = fopen("u0ult.txt", "w");
    //for (i = 0; i < NumSections+1; i++) {
    //    fprintf(f, "%lf %lf\n", i*l/NumSections,  u0[i]);
    //}
    //fclose(f);
    free(u0);
    free(u1);
    return 0;
}
