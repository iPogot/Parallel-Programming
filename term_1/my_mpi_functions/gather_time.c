#include <stdio.h>
#include <mpi.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>


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

void my_gather(void *sbuf, int scount, MPI_Datatype stype, void* rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    int rank = 0;
    int size = 0;
    void* data = NULL;
    int j = 0;
    void* t_rbuf = NULL;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rank != root) {
        MPI_Send(sbuf, scount, MPI_INT, root, 0, comm);
    } else {
        for(int i = 0; i < size; i++) {
            if(i != root) {
                MPI_Recv(rbuf + i * sizeof(int) * rcount, rcount, MPI_INT, i, 0, comm, MPI_STATUS_IGNORE);

            } else {
                *(int *)(rbuf + i * sizeof(int) * rcount) = *(int *)sbuf;
                
            }
        }
    
    }
    MPI_Barrier(MPI_COMM_WORLD);
        
}

int main(int argc, char** argv)  {

    int rank = 0;
    int size = 0;
    int errCode = 0;
    int start_nums = 0;    
    int* rbuf = NULL;
    int sbuf = 0;

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
        rbuf = (int*)calloc(size, sizeof(int));
        if(!rbuf) {
            exit(EXIT_FAILURE);
        }
        //sbuf = rank;
    } 
    sbuf = rank;
    

    start_nums = 100000;
    buf = calloc(start_nums, sizeof(double));
    if(buf == NULL) {
        exit(EXIT_FAILURE);
    }
    
    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        MPI_Gather(&sbuf, 1, MPI_INT , rbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);
        //printf("# %d\n", i);

        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MPI_GATHER___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
    }


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        my_gather(&sbuf, 1, MPI_INT, rbuf , 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MY_GATHER___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
    }
    
    MPI_Finalize();

    return 0;
}