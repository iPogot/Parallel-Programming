#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

#define ROOT            0

#define COL_NUM         12
#define ROW_NUM         12

#define MSG_ROW_NUM     2

int initialize(double **field) {
    
    for(int j = 0; j < COL_NUM; j++) {
        field[0][j] = 100;
    }
    
    for(int i = 1; i < ROW_NUM; i++) {
        field[i][COL_NUM - 1] = 200;
    }
        
    for(int j = COL_NUM - 2; j >= 0; j--) {
        field[ROW_NUM - 1][j] = 300;
    }
    
    for(int i = ROW_NUM - 2; i > 0; i--) {
        field[i][0] = 400;
    }
}

int calc(int first_row, int last_row, double **field, double **rcv_field, int step) {
    //printf("First_row = %d,\nLast_row = %d,\nCurent_step = %d\n", first_row, last_row, step);
    for(int i = first_row; i <= last_row; i++) {
        for(int j = 1; j < ROW_NUM - 1; j++) {
            field[i][j] = (rcv_field[i][j - 1] + rcv_field[i][j + 1] + rcv_field[i - 1][j] + rcv_field[i + 1][j]    ) / 4;
        }
        
    }
    
}


int jacobi_calc(int rank, int size, double** prev_field, double** new_field, int n_steps) {
    int first_calc_row = 1;
    int last_calc_row = MSG_ROW_NUM;
    int cur_step = 0;


    int first_rcv_row = 0;
    int last_rcv_row = 0;

    int ready_to_work = 0;

    int num_rcv_blk = 0;

    int fin_rcv_flag = 0;
    int fin_snd_flag = 0;
    int start_flg = 0;

    cur_step = rank + 1;

    int dest_rank = 0;

    if(rank != size - 1) {
        dest_rank = rank + 1;
    }else {
        dest_rank = 0;
    }
   
    
    initialize(prev_field);
    initialize(new_field);

   int k = 0;

    while(cur_step < n_steps) {
        if(rank == ROOT) {
            
            //if(cur_step == 39){
                
            //}
    
            
            last_calc_row = MSG_ROW_NUM;
            first_calc_row = 1;

            first_rcv_row = 1; 
            last_rcv_row = MSG_ROW_NUM;
            num_rcv_blk = 0;
            
            fin_rcv_flag = 0;
            fin_snd_flag = 0;       // = 1 when sending messages finished 


            while(fin_snd_flag == 0) {
                if(cur_step == 1) {
                    start_flg = 1;        
                } else {
                    start_flg = 0;
                    if(fin_rcv_flag == 0) {
                        //printf("RANK = %d,\nfirst_calc_row = %d,\nlast_calc_row = %d,\nCurent_step = %d,\nfirst_rcv_row = %d,\nlast_rcv_row = %d,\nfin_snd_flag = %d,\nfin_rcv_flag = %d,\nnum_rcv_blk = %d\n\n",
                        //     rank, first_calc_row, last_calc_row, cur_step, first_rcv_row, last_rcv_row, fin_snd_flag, fin_rcv_flag, num_rcv_blk);

                        for(int i = first_rcv_row; i <= last_rcv_row; i++) {
                            MPI_Recv(prev_field[i], COL_NUM, MPI_DOUBLE, size - 1, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        }
                        
                        
                        
                        if(last_rcv_row < ROW_NUM - 2) {
                            first_rcv_row = last_rcv_row + 1;
                            last_rcv_row = first_rcv_row + MSG_ROW_NUM - 1;
                            num_rcv_blk += 1; 
                            
                        } else {
                            
                            first_rcv_row = 0;
                            last_rcv_row = 0;
                            num_rcv_blk = 0;
                            fin_rcv_flag = 1;
                        }
                    }
                }   
                
                if(num_rcv_blk == 2 || fin_rcv_flag == 1 || start_flg == 1) {
                    calc(first_calc_row, last_calc_row, new_field, prev_field, cur_step);
                    
                    for(int i = first_calc_row; i <= last_calc_row; i++) {
                        MPI_Send(new_field[i], COL_NUM, MPI_DOUBLE, dest_rank, i, MPI_COMM_WORLD);
                    }   

                    if(last_calc_row < ROW_NUM - 2) {
                        first_calc_row = last_calc_row + 1;
                        last_calc_row = first_calc_row + MSG_ROW_NUM - 1;
                    } else {
                        first_calc_row = 0;
                        last_calc_row = 0;
                        fin_snd_flag = 1;
                    }

                }
                
            }
            /*printf("Jacobi_____________________________\n");
                printf("ROOT_______________________________\n");
                printf("STEP = %d\n", cur_step);
                for(int i = 0; i < COL_NUM; i++) {
                    for(int j = 0; j < COL_NUM; j++) {
                        printf("%3.0f_", new_field[i][j]);
                    }
                    printf("\n");
                }*/
        } else  {
            last_calc_row = MSG_ROW_NUM;
            first_calc_row = 1;

            first_rcv_row = 1; 
            last_rcv_row = MSG_ROW_NUM;
            num_rcv_blk = 0;
            
            fin_rcv_flag = 0;
            fin_snd_flag = 0;

            

            while(fin_snd_flag == 0) {
                
                if(fin_rcv_flag == 0) {
                    
                    for(int i = first_rcv_row; i <= last_rcv_row; i++) {
                        MPI_Recv(prev_field[i], COL_NUM, MPI_DOUBLE, rank - 1, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    }
                    
                    

                    if(last_rcv_row < ROW_NUM - 2) {
                        first_rcv_row = last_rcv_row + 1;
                        last_rcv_row = first_rcv_row + MSG_ROW_NUM - 1;
                        num_rcv_blk += 1; 
                    } else {
                        first_rcv_row = 0;
                        last_rcv_row = 0;
                        num_rcv_blk = 0;                            
                        fin_rcv_flag = 1;
                    }
                    
                }

                

                if(num_rcv_blk >= 2 || fin_rcv_flag == 1) {                       //2 bloсks of rows have been recieved (time to work and send) 
                    calc(first_calc_row, last_calc_row, new_field, prev_field, cur_step);        

                    for(int i = first_calc_row; i <= last_calc_row; i++) {
                        MPI_Send(new_field[i], COL_NUM, MPI_DOUBLE, dest_rank, i, MPI_COMM_WORLD);
                    }
                    
                    if(last_calc_row < ROW_NUM - 2) {
                        first_calc_row = last_calc_row + 1;
                        last_calc_row = first_calc_row + MSG_ROW_NUM - 1;
                    } else {
                        first_calc_row = 0;
                        last_calc_row = 0;
                        fin_snd_flag = 1;
                    }
                }      
            }
            printf("RANK = 1_______________________________\n");
            printf("STEP = %d\n", cur_step);
            for(int i = 0; i < COL_NUM; i++) {
                for(int j = 0; j < COL_NUM; j++) {
                    printf("%3.0f_", new_field[i][j]);
                }
                printf("\n");
            }
            
        }
        
        cur_step += size;
    }

    return 0;
}

int main(int argc, char** argv) {

    int rank = 0;
    int size = 0;

    int errCode = 0;

    double** new_field = NULL;
    double** prev_field = NULL;


    int n_steps = 100;

    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    new_field = (double **) calloc(ROW_NUM, sizeof(double *));

    for(int j = 0; j < COL_NUM; j++) {
        
        new_field[j] = (double *) calloc(COL_NUM, sizeof(double));

    }



    prev_field = (double **) calloc(ROW_NUM, sizeof(double *));

    for(int j = 0; j < COL_NUM; j++) {
        
        prev_field[j] = (double *) calloc(COL_NUM, sizeof(double));

    }

    
    jacobi_calc(rank, size, prev_field, new_field, n_steps);

    
    MPI_Finalize();
    

    return 0;
}