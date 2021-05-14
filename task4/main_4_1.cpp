#include <cstdio>
#include <cmath>
#include <omp.h>


#define PORTO_START 0.0 //integrate from, double
#define PORTO_FINISH 1.0 //integrate to, double
#define PORTO_COUNT 4000l //count of dots
#define PORTO_LEN (PORTO_FINISH - PORTO_START)
#define PORTO_T 0.01
#define PORTO_h (PORTO_LEN/PORTO_COUNT)
#define PORTO_t  0.3*PORTO_h*PORTO_h
#define PORTO_Graph_dot_count 1000
#define PORTO_fileskip (PORTO_COUNT/PORTO_Graph_dot_count)
#define PORTO_FILEOUT true

//PORTO is name of my project, nothing more


long long count = PORTO_COUNT;
double start = PORTO_START, finish = PORTO_FINISH;
double left = 0.3, right = 1;
double h = PORTO_h, t = PORTO_t, T = PORTO_T;
long long dotcount = PORTO_COUNT, fileskip = PORTO_fileskip, T_n = T/t;


double um(double u0, double u1, double u2)
{
    return u1 + t/(h*h)*(u2-2*u1+u0);
}



int main(int argc, char **argv)
{
    double time_start, time_end;
    time_start = omp_get_wtime();//time measure section



    double * arr1 = new double [count+2] {}, * arr2 = new double [count+2] {};
    ++arr1; ++arr2;
    arr1[-1] = arr2[-1] = left;
    arr1[count] = arr2[count] = right;

    //printf("%lld\n", T_n);
    //T_n = 20;


#pragma omp parallel
    {
        for (long long j = 0; j<T_n; ++j) {
#pragma omp barrier
#pragma omp for schedule (static)
            for (long long i = 0; i < count; i++) {
                arr2[i] = um(arr1[i - 1], arr1[i], arr1[i + 1]);
            }
#pragma omp barrier
#pragma omp master
            {
                double* copy = arr2;
                arr2 = arr1;
                arr1 = copy;
            };
        }
    }


    time_end = omp_get_wtime();
    printf("%f\n", time_end - time_start);

    if (PORTO_FILEOUT) {
        FILE *file;
        file = fopen("data.txt", "w");
        //file = stdout;
        for (long long i = 0; i < dotcount; i += fileskip)
            fprintf(file, "%lf %lf\n", i * h, arr1[i]);
        fclose(file);
    }
    delete[](arr1-1);
    delete[](arr2-1);

    return 0;
}