#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define num_of_proc 10

int main(int argc, char **argv) {
    
    int nthreads, tid;
    int flag = 0;

    if (argc > 1) {
        nthreads = atoi(argv[1]);
    } else {
        nthreads = num_of_proc;
    }

    omp_set_num_threads(nthreads);
   
    #pragma omp parallel
    {   

        while(1) {
            if(omp_get_thread_num() == flag+1) {
                printf("tid = %d\n", omp_get_thread_num());
                flag++;
                break;
            }
        }
    }
   
    return 0;
}

