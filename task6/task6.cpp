#include <iostream>
#include <stdlib.h>
#include <stack>
#include <tuple>
#include <omp.h>
#include <queue>
#include <math.h>

const unsigned LocalStackSize = 8;
const double eps = 0.000001;
const double delta = 0.0000001;


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
    doubleRes = integrateTrapezoid(job.left, (job.right+job.left)/2, func) + integrateTrapezoid((job.right+job.left)/2, job.right, func);
    if (((res - doubleRes) < eps) && ((res-doubleRes) > -eps) || (job.right - job.left < delta)) {
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

    std::cout << numThreads << "\n";

    std::stack<Job> globalStack;
    double h = (right-left)/numThreads;
    int numLazy = 0;
    double result = 0.;

    #pragma omp parallel num_threads(numThreads) reduction(+: result)
    {
        std::stack<Job> localStack;
        std::queue<Job> queue;
        int threadNum = omp_get_thread_num();
        localStack.push({threadNum*h, (threadNum+1)*h});
        bool is_working = true;
        Job currentJob;
        while (1) {
            if (!localStack.empty()) {
                currentJob = localStack.top();
                localStack.pop();
                //std::cout << currentJob.left << " " << currentJob.right << "\n";
                auto jobResult = DoJob(currentJob, func);
                if (jobResult.first) {
                    //std::cout << "OK\n";
                    result += jobResult.second;
                } else {
                    //std::cout << "NO\n";
                    double lleft = currentJob.left;
                    double lright = currentJob.right;
                    localStack.push({lleft, (lright+lleft)/2});
                    queue.push({(lright+lleft)/2, lright});
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
                        //std::cout << numLazy << " lazy " << "2d\n";
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
        std::cout << "endloop\n";
    }

    std::cout << result << "\n";

    return 0;
}