#include <iostream>
#include <stdlib.h>
#include <stack>
#include <tuple>
#include <omp.h>
#include <queue>
#include <math.h>

const unsigned LocalQueueSize = 16;
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

std::pair<std::pair<bool, double>, std::pair<Job, Job>> DoJob(Job& job, double (*func)(double)) {
    double mid = (job.left+job.right)/2;
    double fmid = func(mid);
    double ileft = (job.fleft+fmid)/2*(mid-job.left);
    double iright = (job.fright+fmid)/2*(job.right-mid);
    double res = ileft + iright;
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

    std::stack<Job> globalStack;
    double h = (b-a)/numThreads;
    int numLazy = 0;
    double result = 0.;
    double tstart = omp_get_wtime();

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
                auto jobResult = DoJob(currentJob, func);
                if (jobResult.first.first) {;
                    double r = jobResult.first.second;
                    if (isnan(r)) {
                        r = 0.;
                    }
                    result += r;
                } else {
                    localStack.push(jobResult.second.first);
                    queue.push(jobResult.second.second);
                    if (queue.size() >= LocalQueueSize) {
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
                    } else if (!queue.empty()) {
                        is_working = true;
                        needToCheckTheQueue = true;
                        numLazy--;
                    } else if (numLazy == numThreads) {
                        needToBreak = true;
                    } else {
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
    }

    //std::cout << result << "\n";
    std::cout << omp_get_wtime() - tstart << "\n";

    return 0;
}