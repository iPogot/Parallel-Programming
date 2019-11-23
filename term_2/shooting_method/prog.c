#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

#define ROOT        0

#define NUM_STEPS   10000


const char *name_file_preshooting = "res_without_correction.csv";
const char *name_file_shooting = "res_corrects.csv";


const int y_0 = 0;
const int y_n = -2;
const double t_0 = 0;
const double t_n = 3 * M_PI;

typedef struct {
        double y, v;
} point_t;

double f(double x) {
    return sin(x);
}

void print_in_file(const char *file_neame, double step, double* source) {
    FILE* file = fopen(file_neame, "w");
				
	fprintf(file, "t,x\n");
		
	double t = 0;
	for (int i = 0; i < NUM_STEPS; i++) {
			
		fprintf(file, "%.10f,%.10f\n", t, source[i]);
		t += step;
	}

	fclose(file);
}

int shooting(const char* filename_preshoot, const char* filename_shoot, int rank, int size, int y_0, int y_n, double t_0, double t_n, double (*f)(double) ) {

    int first_step =  0;
    int last_step  = 0;

    point_t point = {
                .y = 0, 
                .v = 0
    };
    

    int num_proc_steps = 0;
    double step_size = 0;
    double proc_reg_len = 0;
    
    point_t* initial_p = NULL;
    point_t* final_p = NULL;
    initial_p = (point_t*) calloc(size, sizeof(point_t));
    final_p = (point_t*) calloc(size, sizeof(point_t));

    num_proc_steps = NUM_STEPS / size;
    step_size = t_n / NUM_STEPS; 
    proc_reg_len = t_n / size;

    if (rank == size-1) {
        first_step =  num_proc_steps * rank;
        last_step = NUM_STEPS;
    } else {
        first_step =  num_proc_steps * rank;
        last_step   = num_proc_steps * (rank + 1);
    }

    // gathering back final ratio x(t) from all the intervals
    int* recvcounts = NULL;
    int* displs = NULL;

    recvcounts = (int*)calloc(size, sizeof(int));
    displs = (int*)calloc(size, sizeof(int));

    for (int i = 0; i < size; i++) {
        recvcounts[i] = NUM_STEPS / size;
        displs[i] = (NUM_STEPS * i) / size;
    }
    recvcounts[size - 1] = NUM_STEPS - displs[size - 1];

    double* proc_result_y = NULL;
    double* proc_result_v = NULL;

    proc_result_y = (double*) calloc(NUM_STEPS, sizeof(double));
    proc_result_v = (double*) calloc(NUM_STEPS, sizeof(double));
    
    double* final_result_y = NULL;
    double* final_result_v = NULL;

    final_result_y = (double*) calloc(NUM_STEPS, sizeof(double));
    final_result_v = (double*) calloc(NUM_STEPS, sizeof(double));

    //first calculating last y and v in processes blocks 
    for (int i = first_step; i < last_step; i++) {
        point.y += point.v * step_size;
        point.v += f(i * step_size) * step_size;
        proc_result_y[i] = point.y;
        proc_result_v[i] = point.v;
    }

    MPI_Gatherv(proc_result_y + first_step, last_step - first_step, MPI_DOUBLE, 
                final_result_y, recvcounts, displs, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
    
    if(rank == ROOT) {  
        print_in_file(name_file_preshooting, step_size, final_result_y);
    }
    //printf("asdasd\n");

    MPI_Gather(&point, sizeof(point_t), MPI_CHAR, final_p, sizeof(point_t), MPI_CHAR, ROOT, MPI_COMM_WORLD);
    

    if (rank == ROOT) {
        point_t res_point_shoot;

        //result point for process rank = 0 region
        res_point_shoot.y = point.y;
        res_point_shoot.v = point.v;

        //printf("__y = %f __\n", point.y);
        //printf("__v = %f __\n", point.v);

        for (int rank = 1; rank < size; rank++) {
            //printf("__y = %f __\n", final_p[rank].y);
            //printf("__v = %f __\n", final_p[rank].v);
        }

        //find resulting y* withiout correction
        for (int rank = 1; rank < size; rank++) {
            res_point_shoot.y += res_point_shoot.v * proc_reg_len;
            res_point_shoot.y += final_p[rank].y;
            res_point_shoot.v += final_p[rank].v;
        }

        //printf("__y = %f __\n", res_point_shoot.y);
        //printf("__v = %f __\n", res_point_shoot.v);


        
        initial_p[0].y = y_0;
        initial_p[0].v = (y_n - res_point_shoot.y) / t_n;

        //corection of initial and final points
        for (int rank = 1; rank < size; rank++) {
            initial_p[rank].y = final_p[rank - 1].y + initial_p[rank - 1].y + initial_p[rank - 1].v * proc_reg_len;
            initial_p[rank].v = final_p[rank - 1].v + initial_p[rank - 1].v;
        }
    }

    //send correct inital points to all processes 
    MPI_Scatter(initial_p, sizeof(point_t), MPI_CHAR, &point, sizeof(point_t), MPI_CHAR, ROOT, MPI_COMM_WORLD);

    if(rank == ROOT) {
        //printf("__x = %f __\n", point.y);
        //printf("__v = %f __\n", point.v);
    }
       
    double res_point_y;
    double res_point_v;

    res_point_y = point.y;
    res_point_v = point.v;

    for (int i = first_step; i < last_step; i++) {
        res_point_y += res_point_v * step_size;
        res_point_v += f(i * step_size) * step_size;
        proc_result_y[i] = res_point_y;
        proc_result_v[i] = res_point_v;
    }

    if(rank == 2) {
        for(int i = 0; i < size; i++) {
            //printf("%d \n", recvcounts[i]);
        }
    }

    MPI_Gatherv(proc_result_y + first_step, last_step - first_step, MPI_DOUBLE, 
                final_result_y, recvcounts, displs, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);

    if(rank == ROOT) {
        for(int i = first_step; i < last_step; i++) {
            //printf("__ %f __\n", final_result_y[i]);
        }
    }

    if(rank == ROOT) {  
        print_in_file(name_file_shooting, step_size, final_result_y);
    }
    return 0;

}



int main(int argc, char **argv) {

    int rank = 0;
    int size = 0;

    int errCode = 0;

    if(errCode = MPI_Init(&argc, &argv)) {
        return errCode;
    }


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    shooting(name_file_preshooting, name_file_shooting, rank, size, y_0, y_n, t_0, t_n, f);

    MPI_Finalize();
    return 0;
}
