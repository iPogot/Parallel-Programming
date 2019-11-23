#include <stdio.h>
#include <mpi.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>

#define BLOCK_SIZE 4 

void copy_after_index(int* input, int* output, int index) {
    int j = 0;
    for(int i = index * BLOCK_SIZE; i < index * BLOCK_SIZE + BLOCK_SIZE; i++) {
        output[j] = input[i];
        j++;
    }
}

int my_scatter(void *sbuf,int scount, MPI_Datatype stype, void *rbuf,int rcount,MPI_Datatype rtype, int root, MPI_Comm comm) {

    int rank = 0;
    int size = 0;

    int* local_buf = NULL;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == root) {
        for(int j = 0; j < size; j++) {

            if(j != root) {
                local_buf = (int*)calloc(BLOCK_SIZE, sizeof(int));
                copy_after_index(sbuf, local_buf, j);

                MPI_Send(local_buf, scount, MPI_INT, j, 0, comm);
            } else {
                copy_after_index(sbuf, rbuf, j);
            }

        }
    } else {
        MPI_Recv(rbuf, rcount, MPI_INT, root, 0, comm, MPI_STATUS_IGNORE);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    return 0;
}


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

int main(int argc, char** argv)  {

    int rank = 0;
    int size = 0;
    int errCode = 0;
    int start_nums = 0; 
    int buf_recv = 0;
    int* sbuf = NULL;
    int* rbuf = NULL;

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

    if(rank == 0) {
        sbuf = (int*)calloc(size * BLOCK_SIZE, sizeof(int));
        if(!sbuf) {
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i < size * BLOCK_SIZE; i++) {
            sbuf[i] = i;
        }
    } 

    rbuf = (int*)calloc(BLOCK_SIZE, sizeof(int));
    if(!rbuf) {
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

        MPI_Scatter(sbuf, BLOCK_SIZE, MPI_INT, rbuf, BLOCK_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MPI_SCATTER___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        my_scatter(sbuf, BLOCK_SIZE, MPI_INT, rbuf, BLOCK_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MY_SCATTER___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
    }

    
    MPI_Finalize();

    return 0;
}