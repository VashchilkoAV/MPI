#include "omp.h"
#include "math.h"

#define PORTO_START 0.1e-4 //integrate from, double
#define PORTO_FINISH 1 //integrate to, double
#define PORTO_DOTS_PER_Half_a_PERIOD 16384

double integrated_fun(double x){
    return 1/x*sin(1/x);
}

long long p = PORTO_DOTS_PER_Half_a_PERIOD;
double x_k(long long k){
    return p/(M_PI*k);
}

double trap(double x_1, double x_2){
    return (x_2-x_1) * integrated_fun((x_2+x_1)/2);
}

int main() {
    double time_start, time_end;
    time_start = omp_get_wtime();//time measure section

    double left = PORTO_START;
    double right = PORTO_FINISH;
    long long left_number = p / (left * M_PI);
    long long right_number = p / (right * M_PI) + 1;
    long long count = left_number - right_number + 1;//p/(r*pi) ... k ... p/(l*pi)
    double left_dot = x_k(left_number);
    double right_dot = x_k(right_number);
    assert(left_dot > left && right_dot < right);
    double res = 0;
    res += trap(left, left_dot) + trap(right_dot, right);

#pragma omp parallel num_threads()
        {
            //printf("%d\n", omp_get_num_threads());
#pragma omp for reduction(+: res)
            for (long long i = right_number; i < left_number; ++i) {
                res += trap(x_k(i + 1), x_k(i));
            }
        }

        time_end = omp_get_wtime();
        //printf("%5lld) res %.20g time %f\n", p, res, time_end - time_start);
        printf("%f\n", time_end - time_start);
    //}
    return 0;
}