#include <stdio.h>
#include <mpi.h>

#define TAG_UNBLOCK 0
#define TAG_ACK 1

int main(int argc, char** argv)  {

    int rank = 0;
    int size = 0;
    int errCode = 0;
    int digit = 0;
    int index = 1;
    int message = 0;
    
    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank == 0) {
        printf("# %d\n", rank);
        while(index < size) {
            MPI_Send(&message, 1, MPI_INT, index, TAG_UNBLOCK, MPI_COMM_WORLD);
            MPI_Recv(&message, 1, MPI_INT, rank + index, TAG_ACK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            index++;
        }
    }   else {
        MPI_Recv(&message, 1, MPI_INT, 0, TAG_UNBLOCK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("# %d\n", rank);
        MPI_Send(&message, 1, MPI_INT, 0, TAG_ACK, MPI_COMM_WORLD);
    }
    
    MPI_Finalize();

    return 0;
}

