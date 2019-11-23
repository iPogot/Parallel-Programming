#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <stdbool.h>
#include <string.h>


#define ISIZE 500
#define JSIZE 500

#define HOR_BLOCK_LIM  JSIZE - 6     //sum = 4982, vert = 2994, hor = 1988 
#define VERT_BLOCK_LIM ISIZE - 2

#define ROOT 0

#define START_NUMS 10


const double horiz_part = 0.4;

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

int calc(double a[][JSIZE], int m_f[][JSIZE], int rank, int size, int block, int hor_proc, int vert_proc, int hor_flag){
    int first_str = 0;
    int last_str = 0;

    int first_col = 0;
    int last_col = 0;
    
    if(hor_flag == 1 && (hor_proc - 1) != rank){
        first_col = rank * block + 3;
        last_col =  first_col + block;

    } else if((hor_flag == 1) && ((hor_proc - 1) == rank)) {
        first_col = (block * rank) + 3;
        last_col = HOR_BLOCK_LIM + 3;
    }

    if(hor_flag == 0 && rank != size - 1){
        first_str = (rank - hor_proc) * block;
        last_str = first_str + block;

    } else if(hor_flag == 0 && rank == size - 1) {
        first_str = (rank - hor_proc) * block;
        last_str = VERT_BLOCK_LIM;
    }
    
    int k = 0;
    int l = 0;
    
    if(hor_flag){
        for(int j = first_col; j < last_col; j++){
            for(int i = 0; i < 2; i++){
                m_f[i][j] = 1;
                l = j - 3;
                k = i + 2;
                while(k < ISIZE && l >= 0){
                    m_f[k][l] = 1;
                    a[k][l] = sin(0.00001 * a[k - 2][l + 3]);
    
                    l = l - 3;
                    k = k + 2; 
                }
                l = 0;
                k = 0;
            }
        }
    } else {
        for (int i = first_str; i < last_str; i++){
            for (int j = JSIZE - 3; j < JSIZE; j++){
                m_f[i][j] = 1;
                l = j - 3;
                k = i + 2;
                while(k < ISIZE && l >= 0){
                    m_f[k][l] = 1;
                    a[k][l] = sin(0.00001 * a[k - 2][l + 3]);
                    l = l - 3;
                    k = k + 2;
                }
                l = 0;
                k = 0;
            }
        }
    }

    if(rank == ROOT){
        for(int i = 0; i < 2; i++){
            for(int j = 0; j < 3; j++){
               m_f[i][j] = 1;
            }
        }

        for(int i = ISIZE - 2; i < ISIZE; i++){
            for(int j = JSIZE - 3; j < JSIZE; j++){
                m_f[i][j] = 1;
            }
        }
    }

    for (int i = 0; i < ISIZE; i++) {
		for (int j = 0; j < JSIZE; j++) {
			if (m_f[i][j] == 0) {
				a[i][j] = 0.0;
			}
		}
	}


    return 0;
}

int par_calc(int rank, int size, double a[][JSIZE], int m_f[][JSIZE], double res[][JSIZE]){
    FILE *ff;

    int hor_flag = 0;
    int block = 0;
    
    int hor_proc = 0;
    int vert_proc = 0;

    hor_proc  =  horiz_part * size;
    vert_proc = size - hor_proc;

    if(rank < hor_proc){
        hor_flag = 1;
        block = (HOR_BLOCK_LIM) / hor_proc;
    } else {
        hor_flag = 0;
        block = (VERT_BLOCK_LIM) / vert_proc;

    }


    calc(a, m_f, rank, size, block, hor_proc, vert_proc, hor_flag);

    MPI_Reduce(a, res, ISIZE * JSIZE, MPI_DOUBLE, MPI_SUM, ROOT, MPI_COMM_WORLD);

    if(rank == ROOT){
        ff = fopen("paral_result.txt","w");

        for(int i = 0; i < ISIZE; i++){
            for (int j = 0; j < JSIZE; j++){
                fprintf(ff,"%f ", res[i][j]);
            }
            fprintf(ff,"\n");
        }
        fclose(ff);
    }

    return 0;
}

int odered_calc(double a[][JSIZE]){
    FILE *ff;
    
    for (int i = 2; i < ISIZE; i++){
        for (int j = 0; j < JSIZE - 3; j++){
            a[i][j] = sin(0.00001 * a[i-2][j+3]);
        }
    }

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

//program Задача 1в
int main(int argc, char **argv)
{
    int rank = 0;
    int size = 0;

    double local_time = 0;
    double tick = 0;
    int errCode = 0;

    
    double a[ISIZE][JSIZE];
    int m_f[ISIZE][JSIZE] = {0};
    double res[ISIZE][JSIZE];

    

    double* buf = NULL;
    buf = (double*) calloc(START_NUMS, sizeof(double));

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

    for(int k = 0; k < START_NUMS; k++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0){
            local_time -= MPI_Wtime();
        }

        par_calc(rank, size, a, m_f, res);

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

        memset(m_f, 0, sizeof(int) * ISIZE *JSIZE);
    }

    if(rank == ROOT) {
        printf("______________PARAL_PROCESSING___________________\n");
        printf("Calc err =     %.10lf\n", error(buf));
    }

    for(int k = 0; k < START_NUMS; k++) {
            local_time -= MPI_Wtime();
            
            odered_calc(a);
    
            local_time += MPI_Wtime();
            buf[k] = local_time;
            local_time = 0;

            for (int i = 0; i < ISIZE; i++){
                for (int j = 0; j < JSIZE; j++){
                    res[i][j] = 10 * i + j;
                }
            }
    }
    

    if(rank == ROOT) {
        printf("\n\n");
        printf("______________ODERED_PROCESSING___________________\n");
        printf("Calc err =     %.10lf\n", error(buf));
    }

    MPI_Finalize();

    return 0;
}