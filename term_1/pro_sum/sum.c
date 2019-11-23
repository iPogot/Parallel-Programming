#include <stdio.h>
#include <mpi.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>

const unsigned char r_file[] = "read_file.txt";
const unsigned char p_file[] = "print_file.txt";

#define READY_TO_WORK       11
#define GO_AWAY             22
#define CATCH_THE_DATA      33
#define OVERFLOW            44
#define PACKET_INFORM       55
#define STOP                99
#define FINISH_WORK         100

#define OVERFLOW_FLAG_YES   121
#define OVERFLOW_FLAG_NO    333

#define ROOT                0



#define MESSAGE_LENGTH      4



void copy_part_of_digit(unsigned char* result, unsigned char* source, int length, int start) {
    for(int i = start; i < length; i++) {
        result[i] = source[i];
    }
}

void paste_block(unsigned char* input_block, unsigned char* result, const int length, const int place) {
    for(int i = 0; i < length; i++) {
        result[place - i] = input_block[length - i - 1];
    }

}

unsigned char* make_predict_res(unsigned char* another_res, int block_length, unsigned char* predict_incr) {
    
    *predict_incr = 1;
    unsigned char* predict_res = NULL;

    /*if(rank == 0) {
        predict_res = calloc(block + 1, sizeof(unsigned char));
    } else {
        predict_res = calloc(block, sizeof(unsigned char));
    }*/
    
    predict_res = calloc(block_length, sizeof(unsigned char));

    for(int i = block_length - 1; i >= 0; i--) {
        predict_res[i] = another_res[i] + *predict_incr;
        if(predict_res[i] >= 100) {
            *predict_incr = 1;
            predict_res[i] = predict_res[i] - 100;
        } else {
            *predict_incr = 0;
        }
    }
    return predict_res;
}

int read_file(unsigned char** digit_1, unsigned char** digit_2, int* full_digit_length, int* buf_length) {
    FILE *file; 
    unsigned char new_symbol;
    unsigned char prev_symbol;
    int buf_index = 0;
    int full_digit_length_1 = 0;
    int full_digit_length_2 = 0;

    if ((file = fopen(r_file, "r")) == NULL) { 
        printf("\nCan't open file\n");
        return -1;
    }

    *digit_1 = calloc(1, sizeof(unsigned char));
    
    while(1) { 
        new_symbol = fgetc(file);
        if(new_symbol == '\n') {
            *digit_1 = realloc(*digit_1, buf_index + 1);
            if(full_digit_length_1 % 2) {
                (*digit_1)[buf_index] = prev_symbol;
                *buf_length = buf_index + 1;
            } else {
                *buf_length = buf_index;
            }
            break;
        }

        full_digit_length_1++;

        if('0' <= new_symbol && new_symbol <= '9') {
            new_symbol = (unsigned char) new_symbol - '0';
        } else { 
            printf("Incorrect input\n");
            fclose(file); 
            return -1;
        }

        if(full_digit_length_1 % 2 == 0) {
            *digit_1 = realloc(*digit_1, buf_index + 1);
            (*digit_1)[buf_index] = prev_symbol * 10 + new_symbol;
            buf_index++;
        } else {
             prev_symbol = new_symbol;
        }
    }
    
    buf_index = 0;

    *digit_2 = calloc(1, sizeof(unsigned char));
    
    while(1) { 
        new_symbol = fgetc(file);
        if(new_symbol == '\n') {
            *digit_2 = realloc(*digit_2, buf_index + 1);
            if(full_digit_length_2 % 2) {
                (*digit_2)[buf_index] = prev_symbol;
                *buf_length = buf_index + 1;
            } else {
                *buf_length = buf_index;
            }
            break;
        }

        full_digit_length_2++;

        if('0' <= new_symbol && new_symbol <= '9') {
            new_symbol = (unsigned char) new_symbol - '0';
        } else { 
            printf("Incorrect input\n");
            fclose(file); 
            return -1;
        }

        if(full_digit_length_2 % 2 == 0) {
            *digit_2 = realloc(*digit_2, buf_index + 1);
            (*digit_2)[buf_index] = prev_symbol * 10 + new_symbol;
            buf_index++;
        } else {
             prev_symbol = new_symbol;
        }
    }

    if(full_digit_length_2 != full_digit_length_1) {
        printf("Incorrect input: length of digits isn't equal\n");
        fclose(file); 
        return -1; 
    } else {
        *full_digit_length = full_digit_length_2;
    }

    fclose(file);
    return 0;
}


int make_blocks(const int buf_length, unsigned char* digit_1, unsigned char* digit_2, 
                unsigned char** buf_operand_1, unsigned char** buf_operand_2, int* block_length, int* residual_buf_length)   {
    int rank = 0;
    int size = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    
    
    if(rank == 0) {
        *block_length = buf_length / (size - 1);
        *residual_buf_length = buf_length % (size - 1);
        printf("buf_length = %d\nblock_length = %d\nresidual_buf_length = %d\n", buf_length, *block_length, *residual_buf_length);
        
    }

    MPI_Bcast(block_length, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank != 0) {
        *buf_operand_1 = calloc(*block_length, sizeof(unsigned char));
        *buf_operand_2 = calloc(*block_length, sizeof(unsigned char));

        if(!(*buf_operand_1) | !(*buf_operand_2)) {
            printf("Calloc error for rank != 0\n");
            return -1;
        }
    } else {
        *buf_operand_1 = calloc(*residual_buf_length, sizeof(unsigned char));
        *buf_operand_2 = calloc(*residual_buf_length, sizeof(unsigned char));
        
        if(!(*buf_operand_1) | !(*buf_operand_2)) {
            printf("Calloc error for rank == 0\n");
            return -1;
        }
       
        copy_part_of_digit(*buf_operand_1, digit_1, *residual_buf_length, 0);
        copy_part_of_digit(*buf_operand_2, digit_2, *residual_buf_length, 0);
    }

    if(rank == 0) {
        for(int i = 1; i < size; i++) {
            MPI_Send(digit_1 + (buf_length - i * (*block_length)) * sizeof(unsigned char), *block_length, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(*buf_operand_1, *block_length, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }   
        /*for(int i = 0; i < *block_length; i++) {
            printf("%d_", (*buf_operand_1)[i]);
        }
        printf("\n");*/
        
    if(rank == 0) {
        for(int i = 1; i < size; i++) {
            MPI_Send(digit_2 + (buf_length - i * (*block_length)) * sizeof(unsigned char), *block_length, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(*buf_operand_2, *block_length, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    /*
    if(rank == 1){
        for(int i = 0; i < *block_length; i++) {
            printf("###__%d_##", (*buf_operand_1)[i]);
        }
        printf("\n");
    }*/
    
    return 0;
}
int sum_blocks(const int block_length, const unsigned char* buf_operand_1, const unsigned char* buf_operand_2, unsigned char* local_incr, unsigned char* buf_sum) {
    *local_incr = 0;
    for(int i = block_length - 1; i >= 0; i--) {
        buf_sum[i] = buf_operand_1[i] + buf_operand_2[i] + *local_incr;
    
        //printf("%d + %d + %d = %d \n", incr, buf_op_1[i], buf_op_2[i], *buf_sum[j]);
        if(buf_sum[i] >= 100) {
            *local_incr = 1;
            buf_sum[i] = buf_sum[i] - 100;
        } else {
            *local_incr = 0;
        }
        /*
        if(rank == 1) {
            printf("%d_\n", local_incr);
        }*/
    }
}

int child_chat(const int block_length, unsigned char* buf_sum, int* s_message, int* r_message, unsigned char* send_incr) 
{
    int rank = 0;
    int size = 0;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    unsigned char* buf_operand_1 = NULL;
    unsigned char* buf_operand_2 = NULL;

    unsigned char local_incr = 0;
    unsigned char recv_incr = 0;
    unsigned char predict_incr = 0;

    unsigned char* predict_res = NULL;

    int current_block_length = 0;

    if(rank == size - 1) {
        s_message[0] = OVERFLOW;
        s_message[1] = rank;
        s_message[2] = *send_incr;
        MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, ROOT, 2, MPI_COMM_WORLD);
    }
    
    s_message[0] = READY_TO_WORK;
    s_message[1] = rank;
    MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, ROOT, 1, MPI_COMM_WORLD);                  //ready to write
    MPI_Send(buf_sum, block_length, MPI_UNSIGNED_CHAR, ROOT, 1, MPI_COMM_WORLD);            //write result
    //printf("######rank = %d\n", rank);

    MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, ROOT, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("######rank = %d\n", rank);
    if(r_message[0] == GO_AWAY) {
        //printf("___rank = %d\n", rank);
        return 0;
    } else if(r_message[0] == PACKET_INFORM) {
        buf_operand_1 = realloc(buf_operand_1, r_message[1]);
        buf_operand_2 = realloc(buf_operand_2, r_message[1]);
    
        s_message[0] = READY_TO_WORK;
        s_message[1] = rank;
        MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, ROOT, 1, MPI_COMM_WORLD);              //send acknowledgement
        
        
        MPI_Recv(buf_operand_1, r_message[1], MPI_UNSIGNED_CHAR, ROOT, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(buf_operand_2, r_message[1], MPI_UNSIGNED_CHAR, ROOT, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
       
        sum_blocks(r_message[1], buf_operand_1, buf_operand_2, &local_incr, buf_sum);
        current_block_length = r_message[1];

        predict_res = make_predict_res(buf_sum, r_message[1], &predict_incr);

        MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, ROOT, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(r_message[0] == OVERFLOW) {
            //printf("___%d____\n", rank);
            recv_incr = r_message[2];

            if(recv_incr == 1) {
                copy_part_of_digit(buf_sum, predict_res, current_block_length, 0);
            } 

            if((local_incr == 1) | (recv_incr & predict_incr)) {
                *send_incr = 1;
            }
            
            s_message[0] = OVERFLOW;
            s_message[1] = rank;
            s_message[2] = *send_incr;
            MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, ROOT, 2, MPI_COMM_WORLD);
        }



        //Collecting
        s_message[0] = FINISH_WORK;
        s_message[1] = rank;
        s_message[2] = current_block_length;
        if(*send_incr) {
            s_message[3] = OVERFLOW_FLAG_YES;
        } else {
            s_message[3] = OVERFLOW_FLAG_NO;
        }
        MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, ROOT, 3, MPI_COMM_WORLD);                          //ready to write
        MPI_Send(buf_sum, current_block_length, MPI_UNSIGNED_CHAR, ROOT, 3, MPI_COMM_WORLD);            //write result

        /*if(rank == 1) {
            printf("\n");
            for(int i = 0; i < current_block_length; i++) {
                printf("%d_", buf_sum[i]);
            }
            printf("\n");
        }
        
        if(rank == 2) {
            printf("\n");
            for(int i = 0; i < current_block_length; i++) {
                printf("%d_", buf_sum[i]);
            }
            printf("\n");
        }*/
        //printf("rank = %d\n", rank);
        
    }
    //printf("rank = %d\n", rank);
    return 0;
}

unsigned char* root_chat(const int block_length, unsigned char* buf_sum, const int residual_buf_length, 
                    int* s_message, int* r_message, unsigned char* result, int result_length, 
                    unsigned char* digit_1, unsigned char* digit_2)  
{
    int* memmory_buf = NULL;
    int memmory_buf_length = 0;
    int child_num = 0;
    int counter = 1;
    int place_to_insert = 0;
    
    int size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int flag = 0;
    int overflow_flag = 0;
    int flag_last = 0;
    //int residual_counter = residual_buf_length;
    int digit_pointer = residual_buf_length;


    unsigned char* recv_buf = calloc(block_length, sizeof(unsigned char));

    if(residual_buf_length % block_length) {
        memmory_buf_length = residual_buf_length / block_length + 1;
        memmory_buf = calloc(memmory_buf_length, sizeof(unsigned char));
    } else {
        memmory_buf_length = residual_buf_length / block_length;
        memmory_buf  = calloc(memmory_buf_length, sizeof(unsigned char)); 
    }
    child_num = memmory_buf_length - 1;
    //printf("child_num = %d\n", child_num);
    
    while(counter < size) {
        MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("rank = %d\n", r_message[1]);
        if(r_message[0] == READY_TO_WORK) {
            
            if(flag) {
                s_message[0] = GO_AWAY;
                s_message[1] = 0;
                MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, r_message[1], 1, MPI_COMM_WORLD);
            }
            
            MPI_Recv(recv_buf, block_length, MPI_UNSIGNED_CHAR, r_message[1], 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            place_to_insert = result_length * block_length - 1 - (r_message[1] - 1) * block_length;
            paste_block(recv_buf, result, block_length, place_to_insert);
            
            digit_pointer = digit_pointer - block_length;
            if((digit_pointer >= 0) & (!flag)) {
                if(digit_pointer == 0) {
                    flag = 1;
                }

                s_message[0] = PACKET_INFORM;
                s_message[1] = block_length;
                MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, r_message[1], 1, MPI_COMM_WORLD);

                MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, r_message[1], 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //printf("rank = %d\n", r_message[1]);
                if(r_message[0] == READY_TO_WORK) {
                    MPI_Send(digit_1 + digit_pointer, block_length, MPI_UNSIGNED_CHAR, r_message[1], 1, MPI_COMM_WORLD);
                    MPI_Send(digit_2 + digit_pointer, block_length, MPI_UNSIGNED_CHAR, r_message[1], 1, MPI_COMM_WORLD);
                    memmory_buf[child_num] = r_message[1];
                    child_num--;
                }
            } else if(!flag) {
                flag = 1;
                //printf("rank = %d\n", r_message[1]);
                s_message[0] = PACKET_INFORM;
                s_message[1] = digit_pointer * (-1);
                MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, r_message[1], 1, MPI_COMM_WORLD);

                MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, r_message[1], 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if(r_message[0] == READY_TO_WORK) {
                    MPI_Send(digit_1, digit_pointer * (-1), MPI_UNSIGNED_CHAR, r_message[1], 1, MPI_COMM_WORLD);
                    MPI_Send(digit_2, digit_pointer * (-1), MPI_UNSIGNED_CHAR, r_message[1], 1, MPI_COMM_WORLD);
                    memmory_buf[child_num] = r_message[1];
                    child_num--;
                }
            }
            
        }
        
        counter++;
    }

    /*PRINTING
    for(int i = 0; i < result_length * block_length; i++) {
        printf("_%d_", result[i]);
    }*/
    
    MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, size - 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("OVERFLOW == %d\n", r_message[1]);
    child_num = memmory_buf_length - 1;
    while(child_num >= 0) {
        
        if(r_message[0] == OVERFLOW) {
            //printf("sdasdasdasda\n");
            //printf("OVERFLOW == %d\n", memmory_buf_length - 1);
            s_message[0] = OVERFLOW;
            s_message[1] = ROOT;
            s_message[2] = r_message[2];
            MPI_Send(s_message, MESSAGE_LENGTH, MPI_INT, memmory_buf[child_num], 2, MPI_COMM_WORLD);
            
            MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, memmory_buf[child_num], 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        child_num--;
    }
    
    counter = 0;
    
    
    while(counter < memmory_buf_length) {
        
        MPI_Recv(r_message, MESSAGE_LENGTH, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        if(r_message[0] == FINISH_WORK) {
            
            MPI_Recv(recv_buf, r_message[2], MPI_UNSIGNED_CHAR, r_message[1], 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            place_to_insert = 0;
            for(int i = 0; i < memmory_buf_length; i++) {
            
                if(memmory_buf[i] == r_message[1]) {
                    
                    if(i == memmory_buf_length - 1) {
                        
                        flag_last = r_message[1];
                        overflow_flag = r_message[3];
                    }
                    place_to_insert = place_to_insert + r_message[2];
                    break;
                } else {
                    place_to_insert = place_to_insert + block_length;
                    
                }
                
            }
            
            if(flag_last != r_message[2]) {
                for(int i = 0; i < r_message[2]; i++) {
                    result[place_to_insert - i - 1] = recv_buf[r_message[2] - i - 1];
                }
            } else {
                for(int i = 0; i < r_message[2]; i++) {
                    result[place_to_insert - i] = recv_buf[r_message[2] - i - 1];
                }

            }         
           
            if(overflow_flag) {
                result[0] = 1;
            }

        }
        counter++;
    }
    /*
    printf("result_length = %d\n", result_length);
    for(int i = 0; i < result_length * block_length; i++) {
        printf("_%d_", result[i]);
    }*/
}

int main_chat(const int block_length, unsigned char* buf_sum, const unsigned char* buf_operand_1, 
                const unsigned char* buf_operand_2, unsigned char* send_incr)                                                             
{
    int rank = 0;
    int size = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    unsigned char local_incr = 0;
    unsigned char recv_incr = 0;
    
    unsigned char predict_incr = 0;

    unsigned char* predict_res = NULL;
    sum_blocks(block_length, buf_operand_1, buf_operand_2, &local_incr, buf_sum);
    if(rank != 1) {
        predict_res = make_predict_res(buf_sum, block_length, &predict_incr);
    }

    if(rank == 1) {
        MPI_Send(&local_incr, 1, MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD);
    } else if(rank != size - 1) {
        MPI_Recv(&recv_incr, 1, MPI_UNSIGNED_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if((recv_incr & predict_incr) | local_incr) {
            *send_incr = 1;
        } else {
            *send_incr = 0;
        }
        MPI_Send(send_incr, 1, MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD);
    } else if(rank == size - 1) {
        MPI_Recv(&recv_incr, 1, MPI_UNSIGNED_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    if((rank != 1) & (recv_incr == 1)) {
        copy_part_of_digit(buf_sum, predict_res, block_length, 0);
    } 
    
    if((rank == size - 1) & ((local_incr == 1) | (recv_incr & predict_incr))) {
        *send_incr = 1;
    }
        
        
}


int sum_control(unsigned char* buf_operand_1, unsigned char* buf_operand_2, const int block_length,
            unsigned char** buf_sum, const int residual_buf_length, unsigned char* result, int result_length,
            unsigned char* digit_1, unsigned char* digit_2) {
    int rank = 0;
    int size = 0;

    int* s_message = NULL;
    int* r_message = NULL;

    unsigned char send_incr = 0;

    s_message = calloc(MESSAGE_LENGTH, sizeof(int));
    r_message = calloc(MESSAGE_LENGTH, sizeof(int));


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    

    *buf_sum = calloc(block_length, sizeof(unsigned char));
    
    
    if(rank != 0) {
        
        
    }
    if(rank != 0) {
        main_chat(block_length, *buf_sum, buf_operand_1, buf_operand_2, &send_incr);
    }

    if(rank == 0) {
        root_chat(block_length, *buf_sum, residual_buf_length, s_message, r_message, result, result_length, digit_1, digit_2);
    } else { 
        child_chat(block_length, *buf_sum, s_message, r_message, &send_incr);
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
            fprintf(file,"%d", res[i]);
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
    int full_digit_length = 0;
    int buf_length = 0;
    
    int block_length = 0;
    int residual_buf_length = 0;
    unsigned char* buf_operand_1 = NULL;
    unsigned char* buf_operand_2 = NULL;

    unsigned char* buf_sum = NULL;

    unsigned char* result = NULL;
    int result_length = 0;
    
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


    if(rank == 0) {
        if(read_file(&digit_1, &digit_2, &full_digit_length, &buf_length)) {
            
            printf("Error with reading file\n");
            return -1;
        }
    }
     

    if(make_blocks(buf_length, digit_1, digit_2, &buf_operand_1, &buf_operand_2, &block_length, &residual_buf_length)) {
        printf("Some troubles with division of digit_1 and digit_2 on segments\n");
        return -1;
    }

    if(buf_length % block_length)   {
        result_length = buf_length / block_length + 1;
        result = calloc(result_length, sizeof(unsigned char));
    } else {
        result_length = buf_length / block_length;
        result = calloc(result_length, sizeof(unsigned char));
    }

    sum_control(buf_operand_1, buf_operand_2, block_length, &buf_sum, residual_buf_length, result, result_length, digit_1, digit_2);
    
    
    /*if(rank == 0) {
        printf("____________\n");
        for(int i = 0; i < result_length * block_length; i++) {
            printf("%d_", result[i]);
        }
    }*/
    print_result(result, result_length * block_length);
    
    
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) {
        work_time += MPI_Wtime();
        printf("Work time = %.8lf\n", work_time);
        printf("Time tick = %.8lf\n", tick);
    }
    //printf("rank = %d\n", rank);
    MPI_Finalize();
    
    return 0; 
}
