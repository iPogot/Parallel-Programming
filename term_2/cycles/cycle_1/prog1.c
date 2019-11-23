#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define ISIZE 1000
#define JSIZE 1000

#define ROOT 0

#define START_NUMS 100

double average(const double* buf) {
    double sum = 0;
    for(int i = 0; i < START_NUMS; i++) {
        sum += buf[i];
    }
    printf("Average_time = %.10lf\n", sum / START_NUMS);
    return sum / START_NUMS;
}

double error(const double* buf) {
    double mean = 0; 
    double sqr_sum = 0;
    mean = average(buf);
    for(int i = 0; i < START_NUMS; i++) {
        sqr_sum = sqr_sum + pow(buf[i] - mean, 2);
    }
    return sqrt(sqr_sum / (START_NUMS * (START_NUMS - 1)));
}

int calc(double a[][JSIZE], int rank, int size, int block){
    int* p = NULL;

    int first_str = 0;
    int last_str = 0;

    first_str = rank * block;

    if(rank != size -1) {
        last_str = (rank + 1) * block;
    } else {
        last_str = ISIZE; 
    }
    
    for(int i = first_str; i < last_str; i++){

        for (int j = 0; j < JSIZE; j++){
            a[i][j] = sin(0.00001 * a[i][j]);
        }
    } 
    
    return 0;
}

int collect(int rank, int size, double a[][JSIZE], int block) {

    int first_str = 0;
    int last_str = 0;

    int src = 1;
    

    first_str = rank * block;

    if(rank != size -1) {
        last_str = (rank + 1) * block;
    } else {
        last_str = ISIZE; 
    }

    if(rank != ROOT) {
        for(int i = first_str; i < last_str; i++){
            MPI_Send(a[i], ISIZE, MPI_DOUBLE, ROOT, i, MPI_COMM_WORLD);
        }
    } else if(rank == ROOT) {
        for(int i = block; i < ISIZE; i++){
            if(i >= block * (src + 1)){
                src++;
            }
            
            MPI_Recv(a[i], ISIZE, MPI_DOUBLE, src, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

}

int par_calc(int rank, int size, double a[][JSIZE]){
    FILE *ff;

    int block = 0;

    block = ISIZE / size;

    double* buf = NULL;

    double local_time = 0;

    buf = (double*) calloc(START_NUMS, sizeof(double));

    for(int k = 0; k < START_NUMS; k++) {
        
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0){
            local_time -= MPI_Wtime();
        }
        
        calc(a, rank, size, block);

        collect(rank, size, a, block);

        MPI_Barrier(MPI_COMM_WORLD);

        if(rank == 0) {
            local_time += MPI_Wtime();
            buf[k] = local_time;
            local_time = 0;
        }

        for (int i = 0; i < ISIZE; i++){
            for (int j = 0; j < JSIZE; j++){
                a[i][j] = 10 * i + j;
            }
        }
    }


    if(rank == 0) {
        printf("______________PARAL_PROCESSING___________________\n");
        printf("Calc err =     %.10lf\n", error(buf));
    }

    if(rank == ROOT){
        ff = fopen("paral_result.txt","w");

        for(int i = 0; i < ISIZE; i++){
            for (int j = 0; j < JSIZE; j++){
                fprintf(ff,"%f ",a[i][j]);
            }
            fprintf(ff,"\n");
        }
        fclose(ff);
    }

    return 0;
}

int odered_calc(double a[][JSIZE]){
    FILE *ff;

    double* buf = NULL;

    double local_time = 0;

    buf = (double*) calloc(START_NUMS, sizeof(double));

    for(int k = 0; k < START_NUMS; k++) {


        for (int i = 0; i < ISIZE; i++){
            for (int j = 0; j < JSIZE; j++){
                a[i][j] = 10 * i + j;
            }
        }
        
        local_time -= MPI_Wtime();
    
        
        for (int i = 0; i < ISIZE; i++){
            for (int j = 0; j < JSIZE; j++){
                a[i][j] = sin(0.00001 * a[i][j]);
            }   
        }

        local_time += MPI_Wtime();

        buf[k] = local_time;
        local_time = 0;

    }


    
    printf("\n\n");
    printf("______________ODERED_PROCESSING___________________\n");
    printf("Calc err =     %.10lf\n", error(buf));
    

    ff = fopen("odered_result.txt","w");

    for(int i = 0; i < ISIZE; i++){
        for (int j = 0; j < JSIZE; j++){
            fprintf(ff,"%f ",a[i][j]);
        }
        fprintf(ff,"\n");
    }
    
    fclose(ff);
    
    return 0;
}


//program from page 11
int main(int argc, char **argv)
{
    int rank = 0;
    int size = 0;

    double local_time = 0;
    double tick = 0;
    int errCode = 0;

    double* buf = NULL;
    
    buf = (double*) calloc(START_NUMS, sizeof(double));

    double a[ISIZE][JSIZE];
    
    for (int i = 0; i < ISIZE; i++){
        for (int j = 0; j < JSIZE; j++){
            a[i][j] = 10 * i + j;
        }
    }

    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    
    par_calc(rank, size, a);
    
    //printf("%d", rank);

    if(rank == ROOT){
        odered_calc(a);
    }
    
            

    MPI_Finalize();

    return 0;
}


