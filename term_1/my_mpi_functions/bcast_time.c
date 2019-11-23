#include <stdio.h>
#include <mpi.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>


void my_bcast(void *data, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int rank;
    int size;
    
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == root) {
        for(int i = 0; i < size; i++) {
            if(i != root) {
                MPI_Send(data, count, datatype, i, 0, comm);
            }
        }   
    } else {
        MPI_Recv(data, count, datatype, root, 0, comm, MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

double error(const double* buf, int size) {
    double average = 0;
    double sum = 0;
    
    for(int i = 0; i < size; i++) {
        sum += buf[i];
    }
    average = sum / size;
    printf("Average_time = %.10lf\n", average);
    double mean = 0; 
    double sqr_sum = 0;
    mean = average;
    for(int i = 0; i < size; i++) {
        sqr_sum = sqr_sum + pow(buf[i] - mean, 2);
    }
    return sqrt(sqr_sum / (size * (size - 1)));
}



int main(int argc, char** argv)  {

    int rank = 0;
    int size = 0;
    int errCode = 0;
    int buf_recv = 0;
    int start_nums = 10000;    

    double local_time = 0;
    double tick = 0;
    double* buf = NULL;

    MPI_Init(&argc, &argv);

    tick = MPI_Wtick();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    buf = calloc(start_nums, sizeof(double));
    if(buf == NULL) {
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        MPI_Bcast(&buf_recv, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MPI_BCAST___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
        printf("_____________________________________________\n");
    }


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    for(int i = 0; i < start_nums; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
            local_time -= MPI_Wtime();

        my_bcast(&buf_recv, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[i] = local_time;
            local_time = 0;
        }
    }

    if(rank == 0) {
        printf("______________MY_BCAST___________________\n");
        printf("Calc err =     %.10lf\n", error(buf, start_nums));
        printf("Tick     =     %.10lf\n", tick);
        printf("_____________________________________________\n");
    }


    
    MPI_Finalize();

    return 0;
}




