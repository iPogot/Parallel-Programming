#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ITERATION  1000000000
#define DEF_THR_NUM    32
#define START_NUM      100

int main( int argc, char **argv){

        int nthreads = 0;
        long double sum = 0;
        long double res_sum = 1;

        int thread_num = 0;

        if (argc > 1) {
                nthreads = atoi(argv[1]);
        } else {
                nthreads = DEF_THR_NUM;
        }
        
        omp_set_num_threads(nthreads);
        
        
        long double* add = (long double*) calloc(nthreads, sizeof(long double));


        for(int k = 0; k < START_NUM; k++) {
                
                res_sum = 1;
                sum = 0;

                for (int  j = 0; j < nthreads; j++) {
                        add[j] = 1;
                }
                #pragma omp parallel firstprivate(sum, thread_num)
                {      
                        thread_num = omp_get_thread_num();

                        #pragma omp for
                        for (int  i = 1; i < MAX_ITERATION; i++){   
                                                                        
                                add[thread_num] = add[thread_num] / (long double) i;
                                sum += add[thread_num];
                        }

                        #pragma omp for
                        for (int i = 1; i < nthreads; i++) {
                                add[i] *=  add[i - 1];
                        }
                        
                        #pragma omp for
                        for (int i = 0; i < nthreads; i++) {
                                if(thread_num) {
                                        sum *= add[thread_num - 1];
                                }
                                
                        }
                        

                        #pragma omp for
                        for(int i = 0; i < nthreads; i++) {
                                res_sum += sum;
                        }
                }
                printf("%.20Lf\n", res_sum); 
                
        }
                 
}

