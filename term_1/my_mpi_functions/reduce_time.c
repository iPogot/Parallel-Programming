#include <stdio.h>
#include <mpi.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>

#define BUF_SIZE 4

double average(const double* buf, int size) {
    double sum = 0;
    for(int i = 0; i < size; i++) {
        sum += buf[i];
    }
    printf("Average_time = %.10lf\n", sum / size);
    return sum / size;
}

double error(const double* buf, int size) {
    double mean = 0; 
    double sqr_sum = 0;
    mean = average(buf, size);
    for(int i = 0; i < size; i++) {
        sqr_sum = sqr_sum + pow(buf[i] - mean, 2);
    }
    return sqrt(sqr_sum / (size * (size - 1)));
}


int my_reduce(void *sbuf, void *rbuf, int count, MPI_Datatype type, MPI_Op op, int root, MPI_Comm comm) {
    int rank = 0;
    int size = 0;
    
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int sum = 0;

    MPI_Barrier(MPI_COMM_WORLD);

    int** data = calloc(size, sizeof(int*)); 
    for(int i = 0; i < size; i++) {
        data[i] = calloc(count, sizeof(int));
    }
    if(!data) {
        printf("\nRANK [%d] calloc error\n", rank);
        return -1;
    }

    if(rank != root) {
        MPI_Send(sbuf, count, MPI_INT, root, 0, comm);
    } else { 
        for(int i = 0; i < size; i++) {
            if(i != root) {
                MPI_Recv(data[i], count, MPI_INT, i, 0, comm, MPI_STATUS_IGNORE);
            } else {
                data[i] = sbuf;
            }
        }
    }
    if(rank == root) {
        if(op == MPI_SUM) {
            for(int i = 0; i < count; i++) {
                for(int j = 0; j < size; j++) {
                    sum += data[j][i];
                }
                ((int *)rbuf)[i] = sum;
                sum = 0;
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    return 0;
}

int main(int argc, char** argv)  {

    int rank = 0;
    int size = 0;
    int errCode = 0;
    int buf_recv = 0;
    int start_nums = 0;   

    int* rbuf = NULL;
    int sbuf[BUF_SIZE] = {0, 1, 2, 3}; 
    
    double local_time = 0;
    double tick = 0;
    double total_time = 0;
    double* buf = NULL;

    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }

    tick = MPI_Wtick();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    rbuf = (int*)calloc(4, sizeof(int));
    if(!rbuf) {
        printf("\nRANK [%d]\n", rank);
        exit(EXIT_FAILURE);
    }

    start_nums = 100000;
    buf = calloc(start_nums, sizeof(double));
    if(buf == NULL) {
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        MPI_Reduce(sbuf, rbuf, BUF_SIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MPI_REDUCE___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
    }


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        my_reduce(sbuf, rbuf, BUF_SIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MY_REDUCE___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
    }

    
    MPI_Finalize();

    return 0;
}