#include <iostream>
#include <cmath>
#include <chrono>
#include <mpi.h>


#define PORTO_START 0.0 //integrate from, double
#define PORTO_FINISH 1.0 //integrate to, double
#define PORTO_COUNT 10000l //count of dots
#define PORTO_LEN (PORTO_FINISH - PORTO_START)
#define PORTO_T 0.001
#define PORTO_h (PORTO_LEN/PORTO_COUNT)
#define PORTO_t  0.3*PORTO_h*PORTO_h
#define PORTO_Graph_dot_count 100
#define PORTO_fileskip (PORTO_COUNT/PORTO_Graph_dot_count)
#define PORTO_FILEOUT true

//PORTO is name of my project, nothing more



class Timer {
public:
    Timer() {
        begin = std::chrono::steady_clock::now();
    }

    ~Timer() {
        auto end = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
        //std::cout << "Time is " << elapsed_ms.count()*0.1e-8 << " s\n";
        std::cout << elapsed_ms.count()*0.1e-8;
    }

private:
    std::chrono::time_point<std::chrono::_V2::steady_clock, std::chrono::duration<long int, std::ratio<1, 1000000000>>>
    begin;

};

class MPI_unit
{
    const int Tag = 0;
    const int root = 0;
    int rank = 0, commSize = 0;
    void fun(double *arr, long long count);
    double left = 0.3, right = 1;
    double h = PORTO_h, t = PORTO_t, T = PORTO_T;
    long long dotcount = PORTO_COUNT, fileskip = PORTO_fileskip, T_n = T/t;

    inline double um(double u0, double u1, double u2)
    {
        return u1 + t/(h*h)*(u2-2*u1+u0);
    }
public:
    //int getrank() const {return rank;}
    //int getcommSize() const {return  commSize;}
    MPI_unit(int argc, char *argv[]){
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    }
    ~MPI_unit() {MPI_Finalize();}
    void run();
    void exchange(double * recL, double * recR, double sendL, double sendR);
};

void MPI_unit::exchange(double *recL, double *recR, double sendL, double sendR) {
    MPI_Status status;
    if (commSize == 1) {
        *recR = right;//might be function
        *recL = left;//might be function
        return;
    }
    if (rank == 0){
        *recL = left;//might be function
        MPI_Send(&sendR, 1, MPI_DOUBLE, rank+1, Tag, MPI_COMM_WORLD);
        MPI_Recv(recR, 1, MPI_DOUBLE, rank+1, Tag, MPI_COMM_WORLD, &status);
    }
    else if (rank == commSize-1){
        *recR = right;//might be function
        if (rank%2==0){
            MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD);
            MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD, &status);
        } else{
            MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD, &status);
            MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD);
        }

    }
    else if (rank%2==0){
        MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD);
        MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD, &status);
        MPI_Send(&sendR, 1, MPI_DOUBLE, rank+1, Tag, MPI_COMM_WORLD);
        MPI_Recv(recR, 1, MPI_DOUBLE, rank+1, Tag, MPI_COMM_WORLD, &status);
    }
    else{
        MPI_Recv(recR, 1, MPI_DOUBLE, rank+1, Tag, MPI_COMM_WORLD, &status);
        MPI_Send(&sendR, 1, MPI_DOUBLE, rank+1, Tag, MPI_COMM_WORLD);
        MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD, &status);
        MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, Tag, MPI_COMM_WORLD);
    }
}



void MPI_unit::fun(double *arr, long long count){
    for (long long j = 0; j<T_n; ++j){
        exchange(arr-1, arr+count, *(arr), *(arr+count-1));
        double copy = arr[-1];
        //     val
        //      |
        //v0 - v1 - v2
        for(long long i = 0; i<count; ++i){
            double c = arr[i];
            arr[i] = um(copy, arr[i], arr[i+1]);
            copy = c;
        }
    }
}

void MPI_unit::run()
{
    //fprintf(stderr, "run working, rank = %d\n", rank);

    if (rank == root)
    {
        Timer timer;
        //MPI root
        //fprintf(stderr, "I'm root, commSize is %d\n", commSize);//print config information
        MPI_Status status;
        long long count = PORTO_COUNT;
        double start = PORTO_START, finish = PORTO_FINISH;

        double * arr = new double [count+2] {};//line array with whole picture to fill
        ++arr;
        long long partSize = count/commSize;//count_of_dots to each task
        long long shift = count%commSize;//remaining dots for distribution
        long long *msg = new long long[2* commSize];//distribution massive//0 - number of start, 1 - count
        /*massive with start index on 2*i positions and
        count of lines on 2*i+1 positions. 0 is root,
        i = 1..commSize-1 are clients*/
        for (int i = root; i < shift; ++i) {
            msg[2*i] = (partSize + 1) * i;
            msg[2*i + 1] = partSize + 1;
        } //clients with count_of_dots partSize+1 to distribute remainig dots
        for (int i = shift; i < commSize; ++i) {
            msg[2*i] = partSize * i + shift;
            msg[2*i + 1] = partSize;
        } //clients with count_of_dots partSize
        for (int i = root+1; i < commSize; ++i)
        {
            //fprintf(stderr, "I'm root, send to %d start %lld count %lld\n", i, msg[2*i], msg[2*i+1]);//printing configs
            MPI_Send(msg + 2*i, 2, MPI_LONG_LONG, i, Tag, MPI_COMM_WORLD);
            //MPI_Send(arr + msg[2*i], msg[2*i+1], MPI_DOUBLE, i, Tag, MPI_COMM_WORLD);
            //sending tasks
        }
        //INTEGRATE
        fun(arr, msg[1]);
        //fprintf(stderr, "I'm %d, local res is\n", rank);

        if (PORTO_FILEOUT)
        for (int i = root+1; i < commSize; ++i)
        {
            MPI_Recv(arr+msg[2*i], msg[2*i+1], MPI_DOUBLE, i, Tag, MPI_COMM_WORLD, &status);

        }
        if (PORTO_FILEOUT) {
            FILE *file;
            file = fopen("data.txt", "w");
            for (long long i = 0; i < dotcount; i += fileskip)
                fprintf(file, "%lf %lf\n", i * h, arr[i]);
            fclose(file);
        }
        delete[] msg;
        --arr;
        delete[] arr;
        //printf("#####\nCommSize is %d \nDotcount is %ld\n", commSize, PORTO_COUNT);
    }
    else
    {
        //MPI client
        MPI_Status status;
        long long msg[2];//two ints to receive task
        //usleep(1000 + 100*rank);
        MPI_Recv(msg, 2, MPI_LONG_LONG, root, Tag, MPI_COMM_WORLD, &status);
        double *arr = new double [msg[1]+2] {};
        ++arr;
        //MPI_Recv(arr, msg[1], MPI_LONG_LONG, root, Tag, MPI_COMM_WORLD, &status);
        //usleep(1000 + 100*rank);
        //fprintf(stderr, "I'm %d, start is %d, count is %d\n", rank, msg[0], msg[1]);

        long long count = PORTO_COUNT;
        double start = PORTO_START, finish = PORTO_FINISH;
        //INTEGRATE
        fun(arr, msg[1]);

        //fprintf(stderr, "I'm %d, local res is %f\n", rank, res);
        if (PORTO_FILEOUT)
        MPI_Send(arr, msg[1], MPI_DOUBLE, root, Tag, MPI_COMM_WORLD);//sending results
        --arr;
        delete[]arr;
    }

}

int main(int argc, char *argv[])
{
    MPI_unit boss(argc, argv);
    boss.run();
}

//compile:  mpic++ main.cpp
//run:      mpirun -np 8 ./a.out
//to know time run: time mpirun -np 8 ./a.out

