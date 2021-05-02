#include "stdio.h"
#include "omp.h"

#include "math.h"
#include "string.h"
#include "stdlib.h"


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
    unsigned long long N = 0, i = 0, num_threads = 1;
    char * pEnd;
    if (argc > 2) {
        N = strtoull(argv[2], &pEnd, 10);
        num_threads = strtoull(argv[1], &pEnd, 10);
    } else {
        printf("Not enough parameters provided\n");
        return 1;
    }

    
    double Sum = 0.0;
    double h = (b-a)/N;
#pragma omp parallel num_threads(num_threads) reduction(+: Sum)
#pragma omp for 
    for (i = 0; i < N; i++)
        Sum += calc(i, a, h, &fc);
    printf("The sum = %lf\n", Sum);
    return 0;
}