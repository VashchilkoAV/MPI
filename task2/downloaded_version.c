#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

double um(double u0, double u1, double u2, double t, double h)
{
	return u1 + t/(h*h)*(u2-2*u1+u0);
}

void change_u(int rank, int size, double* recL, double sendL, double* recR, double sendR)
{	
	if (size == 1)
		return;
	if (rank == 0)
	{
		MPI_Send(&sendR, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD);
		MPI_Recv(recR, 1, MPI_DOUBLE, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	else if (rank == size-1)
        {
		if(rank%2 == 0)
		{
			MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
			MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		else
		{
			MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
		}
        }
	else if (rank%2 == 0)
        {
                MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
		MPI_Send(&sendR, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD);
		MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(recR, 1, MPI_DOUBLE, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
	else
        {
		MPI_Recv(recL, 1, MPI_DOUBLE, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(recR, 1, MPI_DOUBLE, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&sendR, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD);
                MPI_Send(&sendL, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);

        }
}

int main(int argc, char** argv)
{
	int N = atoi(argv[1]);
	int T = atoi(argv[2]);
	double h = 1.0/N, t = 0.3*h*h;
	int rank, size;
	int i = 0, j = 1;	
	double* u = (double*)malloc((N+1)*sizeof(double));
        for (i = 0; i <= N; i++)
                u[i] = 0;
        u[N] = 1;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	int K = N/size, k = N%size;
	double starttime, endtime;

	if(rank == 0)
	{	
		starttime = MPI_Wtime();
		double* u1 = (double*)malloc((K+k+1)*sizeof(double));
        	for (i = 0; i <= K+k; i++)
                	u1[i] = 0;
        	double* u2 = (double*)malloc((K+k+1)*sizeof(double));
        	for (i = 0; i <= K+k; i++)
                	u2[i] = 0;
		if (size == 1)
		{	
			u1[K+k] = 1;
			u2[K+k] = 1;
		}
		
		while (j*t <= T)
		{
			for (i = 1; i < K+k; i++)
				u2[i] = um(u1[i-1], u1[i], u1[i+1], t, h);
			double v = u[N];
			if (size != 1)
			{
				change_u(rank, size, 0, 0, &v, u1[K+k]);
				u2[K+k] = um(u1[K+k-1], u1[K+k], v, t, h);
			}
			for (i = 0; i < K+k+1; i++)
				u1[i] = u2[i];
			j++;
		}

		for (i = 0; i < K+k+1; i++)
                        u[i] = u2[i];
                for (i = 1; i < size; i++)
                {
                        double* u_copy = (double*)malloc((K+1)*sizeof(double));
                        MPI_Recv(u_copy, K+1, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			for (j = 0; j < K+1; j++)
				u[K+k+(i-1)*K+j] = u_copy[j];
                }
		
		FILE * f = fopen("u0ult.txt", "w");
		for(i = 0; i <= N; i++)
			fprintf(f, "%lf\n", u[i]);
		fclose(f);
		endtime = MPI_Wtime();
		printf("%lf\n", endtime-starttime);
	}
	else if (rank == size-1)
	{
		double* u1 = (double*)malloc((K+1)*sizeof(double));
                for (i = 0; i <= K; i++)
                        u1[i] = 0;
                double* u2 = (double*)malloc((K+1)*sizeof(double));
                for (i = 0; i <= K; i++)
                        u2[i] = 0;
		u1[K] = 1;
		u2[K] = 1;
	
		while (j*t <= T)
                {
                        for (i = 1; i < K; i++)
                        	u2[i] = um(u1[i-1], u1[i], u1[i+1], t, h);
                        double v = u[0];
                        change_u(rank, size, &v, u1[0], 0, 0);
                        u2[0] = um(v, u1[0], u1[1], t, h);
                      	for (i = 0; i < K+1; i++)
                        	u1[i] = u2[i];
                       	j++;
                }
		
		MPI_Send(u2, K+1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

	}
	else
	{
		double* u1 = (double*)malloc((K+1)*sizeof(double));
                for (i = 0; i <= K; i++)
                        u1[i] = 0;
                double* u2 = (double*)malloc((K+1)*sizeof(double));
                for (i = 0; i <= K; i++)
                        u2[i] = 0;
	
		while (j*t <= T)
                {
                        for (i = 1; i < K; i++)
                                u2[i] = um(u1[i-1], u1[i], u1[i+1], t, h);
                        double v = u[0], w = u[N];
                        change_u(rank, size, &v, u1[0], &w, u1[K+k]);	
                        u2[0] = um(v, u1[0], u1[1], t, h);
			u2[K] = um(u1[K-1], u1[K], w, t, h);
                        for (i = 0; i < K+1; i++)
                                u1[i] = u2[i];
                        j++;
                }

		MPI_Send(u2, K+1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

	}

	MPI_Finalize();
	return 0;
}
