#include <iostream>
#include <stdlib.h>
#include <stack>
#include <tuple>
#include <omp.h>
#include <queue>
#include <math.h>

const unsigned LocalStackSize = 16;
const double eps = 0.00001;
const double delta = 0.000000000001;


struct Job {
    double left;
    double right;
    double fleft;
    double fright;
    double integral;
};

const double a = 0.0001, b = 1.;

double func(double arg) {
    return sin(1/arg)/arg*sin(1/arg)/arg;
}

double func2(double arg) {
    return arg;
}

double integrateTrapezoid(double left, double right, double (*func)(double)) {
    return (func(left)+func(right))*(right-left)/2;
}

std::pair<std::pair<bool, double>, std::pair<Job, Job>> DoJob(Job& job, double (*func)(double)) {
    //std::cout << "kk\n";
    double mid = (job.left+job.right)/2;
    double fmid = func(mid);
    double ileft = (job.fleft+fmid)/2*(mid-job.left);
    double iright = (job.fright+fmid)/2*(job.right-mid);
    double res = ileft + iright;
    //std::cout << res << " " << job.integral << "\n";
    if (fabs(res-job.integral) > eps*fabs(res)) {
        return {{false, 0.}, {{job.left, mid, job.fleft, fmid, ileft}, {mid, job.right, fmid, job.fright, iright}}};
    } else {
        return {{true, res}, {}};
    }
}

int main(int argc, char** argv) {
    unsigned long numThreads = 1;
    if (argc > 1) {
        char *pEnd;
        numThreads = strtoul(argv[1], &pEnd, 10);
    } else {
        std::cout << "Not enough parametres provided\n";
        return 1;
    }

    //std::cout << numThreads << "\n";

    std::stack<Job> globalStack;
    double h = (b-a)/numThreads;
    int numLazy = 0;
    double result = 0.;

    #pragma omp parallel num_threads(numThreads) reduction(+: result)
    {
        std::stack<Job> localStack;
        std::queue<Job> queue;
        int threadNum = omp_get_thread_num();
        bool is_working = true;
        Job currentJob;

        double left = a+threadNum*h;
        double right = a+(threadNum+1)*h;
        double fleft = func(left);
        double fright = func(right);
        double integral = (fleft+fright)/2*(right-left);
        localStack.push({left, right, fleft, fright, integral});
        while (1) {
            if (!localStack.empty()) {
                currentJob = localStack.top();
                localStack.pop();
                //std::cout << currentJob.left << " " << currentJob.right << "\n";
                auto jobResult = DoJob(currentJob, func);
                if (jobResult.first.first) {
                    //std::cout << "OK\n";
                    double r = jobResult.first.second;
                    if (isnan(r)) {
                        r = 0.;
                    }
                    result += r;
                } else {
                    //std::cout << "NO\n";
                    localStack.push(jobResult.second.first);
                    queue.push(jobResult.second.second);
                    if (queue.size() >= LocalStackSize) {
                        #pragma omp critical
                        {
                            Job tmpJob;
                            while (!queue.empty()) {
                                tmpJob = queue.front();
                                queue.pop();
                                globalStack.push(tmpJob);
                            }
                        }
                    }
                }
            } else {
                Job tmpJob;
                bool needToCheckTheQueue = false;
                bool needToBreak = false;
                #pragma omp critical 
                {
                    //std::cout << omp_get_thread_num() << "\n";
                    //std::cout << numLazy << "\n"; 
                    if (is_working) {
                        numLazy++;
                    }
                    if (!globalStack.empty()) {
                        is_working = true;
                        numLazy--;
                        for (unsigned i = 0; i < LocalStackSize; i++) {
                            if (!globalStack.empty()) {
                                tmpJob = globalStack.top();
                                globalStack.pop(); 
                                localStack.push(tmpJob);
                            } else {  
                                break;
                            }
                        }
                        while (!queue.empty()) {
                            tmpJob = queue.front();
                            queue.pop();
                            globalStack.push(tmpJob);
                        }
                        //std::cout << numLazy << " lazy " << "1d\n";
                    } else if (!queue.empty()) {
                        is_working = true;
                        needToCheckTheQueue = true;
                        numLazy--;
                       // std::cout << numLazy << " lazy " << "2d\n";
                    } else if (numLazy == numThreads) {
                        needToBreak = true;
                    } else {
                        //std::cout << "not working\n";
                        is_working = false;
                    }            
                }

                if (needToBreak) {
                    break;
                }
                if (needToCheckTheQueue) {
                    needToCheckTheQueue = false;
                    while (!queue.empty()) {
                        tmpJob = queue.front();
                        queue.pop();
                        localStack.push(tmpJob);
                    }
                }
            }
        }
        //std::cout << "endloop\n";
    }

    std::cout << result << "\n";

    return 0;
}