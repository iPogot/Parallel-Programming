#include <stdio.h>
#include <mpi.h>


int main(int argc, char** argv)  {

    int rank = 0;
    int size = 0;
    int errCode = 0;
    int digit = 0;
    int index = 0;
    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank == 0) {
        printf("# %d\n", rank);
        fflush(stdout);
        index = index + 1;
        MPI_Send(&index, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(&index, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("# %d\n", rank);
        index++;
        if(rank != size - 1) {
            MPI_Send(&index, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();

    return 0;
}
