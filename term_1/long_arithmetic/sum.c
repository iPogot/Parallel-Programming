#include <stdio.h>
#include <mpi.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>

#define full_digit_length 1024 
#define half_digit_length 512



const unsigned char r_file[] = "read_file.txt";
const unsigned char p_file[] = "print_file.txt";


int calc_size_of_block(int* block, int size) {
    if(half_digit_length % size == 0) {
        *block = half_digit_length / size;
    } else {
        printf("Incorrect counts of processes\n");
        return -1;
    }

    return 0;
}
void copy_true_answear(unsigned char* answear, unsigned char* predict_res, int block, const int rank) {
    for(int i = block - 1; i >= 0; i--) {
        answear[i] = predict_res[i];
    }
}

unsigned char* make_predict_res(unsigned char* another_res, int block, const int rank, unsigned char* predict_incr) {
    //unsigned char elem = 0;
    *predict_incr = 1;
    unsigned char* predict_res = NULL;

    if(rank == 0) {
        predict_res = calloc(block + 1, sizeof(unsigned char));
    } else {
        predict_res = calloc(block, sizeof(unsigned char));
    }
    
    for(int i = block - 1; i >= 0; i--) {
        predict_res[i] = another_res[i] + *predict_incr;
        //printf("%d_", predict_res[i]);
        if(predict_res[i] >= 100) {
            *predict_incr = 1;
            predict_res[i] = predict_res[i] - 100;
        } else {
            *predict_incr = 0;
        }
    }
    return predict_res;
}

int read_file(unsigned char* digit_1, unsigned char* digit_2) {
    FILE *file; 
    unsigned char new_symbol;
    unsigned char prev_symbol;
    int half_digit_index = 0;

    if ((file = fopen(r_file, "r")) == NULL) { 
        printf("\nCan't open file\n");
        return -1;
    }

    for (int i = 0; i < full_digit_length; i++) { 
        new_symbol = fgetc(file);

        if('0' <= new_symbol && new_symbol <= '9') {
            new_symbol = (unsigned char) new_symbol - '0';
        } else { 
            printf("Incorrect input\n");
            fclose(file); 
            return -1;
        }

        if(i % 2) {
            digit_1[half_digit_index] = prev_symbol * 10 + new_symbol;
            half_digit_index++;
        }

        prev_symbol = new_symbol;
    }

    fgetc(file); 
    half_digit_index = 0;

    for (int i = 0; i < full_digit_length; i++) { 
        new_symbol = fgetc(file);

        if('0' <= new_symbol && new_symbol <= '9') {
            new_symbol = (unsigned char) new_symbol - '0';
        } else { 
            printf("Incorrect input\n");
            fclose(file); 
            return -1;
        }

        if(i % 2) {
            digit_2[half_digit_index] = prev_symbol * 10 + new_symbol;
            half_digit_index++;
        }

        prev_symbol = new_symbol;
    }

    fclose(file);
    return 0;
}

int make_blocks(const int size, unsigned char* digit_1, unsigned char* digit_2, unsigned char** buf_op_1, unsigned char** buf_op_2, int block) {

    MPI_Scatter(digit_1, block, MPI_CHAR, *buf_op_1, block, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Scatter(digit_2, block, MPI_CHAR, *buf_op_2, block, MPI_CHAR, 0, MPI_COMM_WORLD);
    return 0;
}

int sum_blocks(int rank, int size, char* buf_op_1, char* buf_op_2, int block, unsigned char* buf_sum) {
    unsigned char local_incr = 0;
    unsigned char recv_incr = 0;
    unsigned char send_incr = 0;
    unsigned char predict_incr = 0;

    unsigned char* predict_res = NULL;
    
    
    for(int i = block - 1; i >= 0; i--) {
        buf_sum[i] = buf_op_2[i] + buf_op_1[i] + local_incr;
        
        //printf("%d + %d + %d = %d \n", incr, buf_op_1[i], buf_op_2[i], *buf_sum[j]);
        if(buf_sum[i] >= 100) {
            local_incr = 1;
            buf_sum[i] = buf_sum[i] - 100;
        } else {
            local_incr = 0;
        }
        //printf("%d_", buf_sum[i]);
    }
    
    predict_res = make_predict_res(buf_sum, block, rank, &predict_incr);
    
    /*/
    printf("\n");
    for(int i = block - 1; i >= 0; i--) {
        printf("%d_", predict_res[i]);
    } /*/
    
    
    if(rank == size - 1) {
        MPI_Send(&local_incr, 1, MPI_UNSIGNED_CHAR, rank - 1, 0, MPI_COMM_WORLD);
    } else if(rank != 0) {
        MPI_Recv(&recv_incr, 1, MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if((recv_incr & predict_incr) | local_incr) {
            send_incr = 1;
        } else {
            send_incr = 0;
        }
        
        MPI_Send(&send_incr, 1, MPI_UNSIGNED_CHAR, rank - 1, 0, MPI_COMM_WORLD);
    } else if(rank == 0) {
        MPI_Recv(&recv_incr, 1, MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    if((rank != size - 1) & (recv_incr == 1)) {
        copy_true_answear(buf_sum, predict_res, block, rank);
    } 
   
    //printf("Rank[%d] buf_sum = %d_%d\n", rank, buf_sum[0], buf_sum[1]);
    
    if((rank == 0) & ((local_incr == 1) | (recv_incr & predict_incr))) {
        buf_sum[block] = 1;
    }

    return 0;
}

int collect_data(unsigned char** result, unsigned char* buf_sum, int* length, int rank, int size, int block) {
    if(rank == 0) {
        *length = half_digit_length;
        
        if(buf_sum[block]) {
            (*length)++;
            (*result) = calloc(half_digit_length + 1, sizeof(unsigned char));
        } else {
            (*result) = calloc(half_digit_length, sizeof(unsigned char));
        }
        
    }

    MPI_Gather(buf_sum, block, MPI_UNSIGNED_CHAR, *result, block, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    
    if((rank == 0) & (buf_sum[block])) {
        for(int i = *length - 1; i > 0; i--) {
            (*result)[i] = (*result)[i - 1];
        }
        (*result)[0] = 1;
    }
    
    return 0;
}

int print_result(unsigned char* res, int length) {
    FILE* file;
    if ((file = fopen(p_file, "w")) == NULL) { 
        printf("\nCan't open file\n");
        return -1;
    }
    for(int i = 0; i < length; i++) {
        if((res[i] < 10) & (i != 0)) {
            fprintf(file,"0%d", res[i]);
            //printf("0%d_", res[i]);
        } else {
            //printf("%d_", res[i]);
            fprintf(file,"%d", res[i]);
        }
    }

    fclose(file);
    
    return 0;

}

int main(int argc, char **argv) { 
    int rank = 0;
    int size = 0;
    int errCode = 0;
    double work_time = 0;
    double tick = 0;

    unsigned char* digit_1 = NULL;
    unsigned char* digit_2 = NULL;
    
    int block = 0;

    unsigned char* result = NULL;
    int length = 0;
    
    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == 0) {
        tick = MPI_Wtick();
        work_time -= MPI_Wtime();
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    digit_1 = calloc(half_digit_length, sizeof(unsigned char));
    digit_2 = calloc(half_digit_length, sizeof(unsigned char));

    if(rank == 0) {
        if(read_file(digit_1, digit_2)) {
            printf("Error with reading file\n");
            return -1;
        }
    }
    if(calc_size_of_block(&block, size)) {
        printf("Error with calculation size of block\n");
        return -1;
    }
    unsigned char* buf_op_1 = calloc(block, sizeof(unsigned char));
    unsigned char* buf_op_2 = calloc(block, sizeof(unsigned char));
    unsigned char* buf_sum = NULL;

    if(rank == 0) {
        buf_sum = calloc(block + 1, sizeof(unsigned char));
    } else {
        buf_sum = calloc(block, sizeof(unsigned char));
    }

    if(!buf_op_1 & !buf_op_2 & !buf_sum) {
        printf("Error with allocation memory to process's buffers");
    }

    if(make_blocks(size, digit_1, digit_2, &buf_op_1, &buf_op_2, block)) {
        printf("Some troubles with division of digit_1 and digit_2 on segments\n");
        return -1;
    }
    
    
    sum_blocks(rank, size, buf_op_1, buf_op_2, block, buf_sum);
    /*if(rank == 0) {
        for(int i = 0; i <= block; i++) {
            printf("%d_", buf_sum[i]);
        }
    }*/

    collect_data(&result, buf_sum, &length, rank, size, block);

     
    /*if(rank == 0) {
        for(int i = 0; i < length; i++) {
            printf("%d_", result[i]);
            
        }
    }*/


    if(print_result(result, length)) {
        printf("Error: can't open output file\n");
        return -1;
    } 
    
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) {
        work_time += MPI_Wtime();
        printf("Work time = %.8lf\n", work_time);
        printf("Time tick = %.8lf\n", tick);
    }

    MPI_Finalize();
    
    return 0; 
}
