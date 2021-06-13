#include "stdio.h"
#include "math.h"
#include "mpi.h"


#define PORTO_COUNT 10000000
#define PORTO_Send_Results true
#define PORTO_print false
#define PORTO_Set_Values true


//PORTO is name of my project, nothing more

#define PORTO_NUMERIC_SYSTEM 10





class MPI_unit
{
    const int Tag = 0;
    const int root = 0;
    int rank = 0, commSize = 0;
    void fun(int *arr1, int *arr2, int *arr3, int *arr4, long long count);

public:
    MPI_unit(int argc, char *argv[]){
        
    }
    ~MPI_unit() {MPI_Finalize();}
    void run();
    void exchange(int* arr3, int* arr4, int*& res);
};

void MPI_unit::exchange(int* arr3, int* arr4, int*&res){
    if (commSize == 1){
        res = arr3; return;
    } else
    if (rank==commSize-1){
        res = arr3;
        MPI_Send(&res[0], 1, MPI_INT, rank-1, Tag, MPI_COMM_WORLD);
        return;
    }else
    if (rank == 0){
        int reg = 0;
        MPI_Status status;
        MPI_Recv(&reg, 1, MPI_INT, rank+1, Tag, MPI_COMM_WORLD, &status);
        res = (reg==0)?arr3:arr4;
        return;
    } else{
        int reg = 0;
        MPI_Status status;
        MPI_Recv(&reg, 1, MPI_INT, rank+1, Tag, MPI_COMM_WORLD, &status);
        res = (reg==0)?arr3:arr4;
        MPI_Send(&res[0], 1, MPI_INT, rank-1, Tag, MPI_COMM_WORLD);
        return;
    }
}



void MPI_unit::fun(int *arr1, int *arr2, int *arr3, int *arr4, long long count){
    arr4[count] = 1;//перенос разряда
    for (long long i = count; i>0; --i){
        arr3[i]+=arr1[i]+arr2[i];
        arr3[i-1] = arr3[i]/PORTO_NUMERIC_SYSTEM;
        arr3[i]%=PORTO_NUMERIC_SYSTEM;

        arr4[i]+=arr1[i]+arr2[i];
        arr4[i-1] = arr4[i]/PORTO_NUMERIC_SYSTEM;
        arr4[i]%=PORTO_NUMERIC_SYSTEM;
    }
}

void MPI_unit::run()
{
    if (rank == root)
    {
        Timer timer;
        //MPI root
        //fprintf(stderr, "I'm root, commSize is %d\n", commSize);//print config information
        MPI_Status status;
        long long count = PORTO_COUNT;

        int *arr1 = new int [count+1] {}, *arr2 = new int [count+1] {},
            *arr3 = new int [count+1] {}, *arr4 = new int [count+1] {};
        if (PORTO_Set_Values) {
            for (int i = 1; i <= count; ++i) arr1[i] = PORTO_NUMERIC_SYSTEM - 1;
            arr2[count] = 1;
        }

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
            if (PORTO_Set_Values) {
                MPI_Send(arr1 + 1 + msg[2 * i], msg[2 * i + 1], MPI_INT, i, Tag, MPI_COMM_WORLD);
                MPI_Send(arr2 + 1 + msg[2 * i], msg[2 * i + 1], MPI_INT, i, Tag, MPI_COMM_WORLD);
            }
            //sending tasks
        }
        if(!PORTO_Set_Values){
            for (int i=1; i<=msg[1]; ++i){
                arr1[i] = PORTO_NUMERIC_SYSTEM-1;
                arr2[i] = PORTO_NUMERIC_SYSTEM-1;
            }
        }
        //SUM
        fun(arr1, arr2, arr3, arr4, msg[1]);
        int* res;
        exchange(arr3, arr4, res);
        //fprintf(stderr, "I'm %d, local res is\n", rank);

        if (PORTO_Send_Results)
        for (int i = root+1; i < commSize; ++i)
        {
            MPI_Recv(res+1+msg[2*i], msg[2*i+1], MPI_DOUBLE, i, Tag, MPI_COMM_WORLD, &status);

        }
        if(PORTO_print) {
            for (int i = 0; i <= count; ++i)
                std::cout << res[i] << ' ';
            std::cout << '\n';
        }
        delete[] msg;
        delete[] arr3;
        delete[] arr4;
        delete[] arr1;
        delete[] arr2;
        //printf("#####\nCommSize is %d \nDotcount is %ld\n", commSize, PORTO_COUNT);
    }
    else
    {
        //MPI client
        MPI_Status status;
        long long msg[2];//two ints to receive task
        //usleep(1000 + 100*rank);
        MPI_Recv(msg, 2, MPI_LONG_LONG, root, Tag, MPI_COMM_WORLD, &status);
        int *arr1 = new int [msg[1]+1] {}, *arr2 = new int [msg[1]+1] {}, *arr3 = new int [msg[1]+1] {}, *arr4 = new int [msg[1]+1] {};
        if (PORTO_Set_Values) {
            MPI_Recv(arr1 + 1, msg[1], MPI_INT, root, Tag, MPI_COMM_WORLD, &status);
            MPI_Recv(arr2 + 1, msg[1], MPI_INT, root, Tag, MPI_COMM_WORLD, &status);
        } else{
            for (int i=1; i<=msg[1]; ++i){
                arr1[i] = PORTO_NUMERIC_SYSTEM-1;
                arr2[i] = PORTO_NUMERIC_SYSTEM-1;
            }
        }
        //usleep(1000 + 100*rank);
        //fprintf(stderr, "I'm %d, start is %d, count is %d\n", rank, msg[0], msg[1]);

        fun(arr1, arr2, arr3, arr4, msg[1]);
        int *res;
        exchange(arr3, arr4, res);

        //fprintf(stderr, "I'm %d, local res is %f\n", rank, res);
        if (PORTO_Send_Results)
        MPI_Send(res+1, msg[1], MPI_INT, root, Tag, MPI_COMM_WORLD);//sending results
        delete[]arr1;
        delete[]arr2;
        delete[]arr3;
        delete[]arr4;
    }

}

int main(int argc, char *argv[]) {
    int rank, commSize;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    
}

//compile:  mpic++ main.cpp
//run:      mpirun -np 8 ./a.out
//to know time run: time mpirun -np 8 ./a.out

