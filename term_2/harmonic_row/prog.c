#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define MAX_ITERATION   1000000000
#define DEF_THR_NUM     32
#define RUN_TIMES       50

int main(int argc, char **argv) {
    double sum = 0;
    double res_sum = 0;

    int nthreads, tid;
    int flag = 0;

    if (argc > 1) {
        nthreads = atoi(argv[1]);
    } else {
        nthreads = DEF_THR_NUM;
    }
    
    flag = nthreads;
    omp_set_num_threads(nthreads);
   for(int i = 0; i < RUN_TIMES; i++) {
        #pragma omp parallel firstprivate(sum)
        {
            int n = 1;
            #pragma omp for private(n)
            for (n = 1; n < MAX_ITERATION; n++) {
                sum += 1.0 / (double)(n);
            }   
            //printf("sum[%d] = %f\n", omp_get_thread_num(), sum);
            
            while(1) {
                if(omp_get_thread_num() == flag - 1) {
                    //printf("tid = %d\n", omp_get_thread_num());
                    flag--;
                    res_sum += sum;
                    break;
                }
            }
        }
        printf("res_sum = %.20f\n", res_sum);
        res_sum = 0.0;
        flag = nthreads;
    }
    
    return 0;
}