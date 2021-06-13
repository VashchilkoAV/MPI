#include <iostream>
#include <stdlib.h>
#include <stack>
#include <tuple>
#include <omp.h>
#include <queue>
#include <math.h>

const unsigned LocalStackSize = 8;
const double eps = 0.000001;


struct Job {
    double left;
    double right;
};

const double left = 0., right = 1.;

double func(double arg) {
    return 1/arg/arg*sin(1/arg)*sin(1/arg);
}

double integrateTrapezoid(double left, double right, double (*func)(double)) {
    return 0.5*(func(left)+func(right))*(right-left);
}

std::pair<bool, double> DoJob(Job& job, double (*func)(double)) {
    double res = 0., doubleRes = 0.;
    res = integrateTrapezoid(job.left, job.right, func);
    doubleRes = integrateTrapezoid(job.left, (job.right-job.left)/2, func) + integrateTrapezoid((job.right-job.left)/2, job.right, func);
    if (res - doubleRes < eps && res-doubleRes > -eps) {
        return {true, res};
    } else {
        return {false, 0.};
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

    std::stack<Job> globalStack;
    double h = (right-left)/numThreads;
    int numLazy = 0;
    double result = 0.;

    #pragma omp parallel num_threads(numThreads) reduction(+: result)
    {
        std::stack<Job> localStack;
        std::queue<Job> queue;
        int threadNum = omp_get_thread_num();
        localStack.push({threadNum*h, threadNum*(h+1)});
        Job currentJob;
        while (1) {
            if (!localStack.empty()) {
                currentJob = localStack.top();
                localStack.pop();
                auto jobResult = DoJob(currentJob, func);
                if (jobResult.first) {
                    result += jobResult.second;
                } else {
                    double left = currentJob.left;
                    double right = currentJob.right;
                    localStack.push({left, (right-left)/2});
                    queue.push({(right-left)/2, right});
                }
            } else {
                Job tmpJob;
                bool needToCheckTheQueue = false;
                bool needToBreak = false;
                #pragma omp critical 
                {
                    if (!globalStack.empty()) {
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
                        numLazy--;
                    } else if (!queue.empty()) {
                        needToCheckTheQueue = true;
                        numLazy--;
                    } else if (numLazy == numThreads) {
                        needToBreak = true;
                    } else {
                        numLazy++;
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
    }

    std::cout << result << "\n";

    return 0;
}