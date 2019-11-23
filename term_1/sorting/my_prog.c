#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>


struct transmit {
    int*  numbers;
    int   block;
    int   odd_flag;
    int   id;
};


void change(int* digit_1, int *digit_2)
{
  int x = *digit_1;
  *digit_1 = *digit_2;
  *digit_2 = x;
}


void *compare(void *param)
{
  struct transmit *tr = (struct transmit*) param;

  for(int i = tr->block * tr->id + tr->odd_flag; i < tr->block * (tr->id + 1) + tr->odd_flag; i += 2) {
    if (tr->numbers[i] > tr->numbers[i+1]) {
      change(&tr->numbers[i], &tr->numbers[i+1]);
    }
  }
}
 
 
int main(int argc, char** argv)
{
  pthread_t tread_id[32];
 
  int   pthread_numbers = atoi(argv[1]); 
  int   numbers_count = 0; 
   
  int*  numbers = NULL;
 
  FILE *r_file;
  FILE *p_file;
 
  struct transmit thread[32];
  int start_time = 0;
  int end_time = 0;
 
  r_file = fopen("read_file.txt","r");
  p_file = fopen("print_file.txt","w");
 
  fscanf(r_file, "%d", &numbers_count);
 
  numbers = (int *) calloc(numbers_count, sizeof(int)); 
 
  for(int i = 0; i < numbers_count; i++) {
    fscanf(r_file, "%d", &numbers[i]);
  }
 
  start_time -= clock();
 
  //Распределение чисел по блокам
  int odd = 0;
  if((numbers_count / pthread_numbers) % 2) {
      odd = 1;
  } else {
    odd = 0;
  }

  for(int i = 0; i < pthread_numbers - 1; i++) {
    thread[i].numbers = numbers;
    thread[i].id = i;
    thread[i].block = numbers_count / pthread_numbers + odd;
  }
   
 
  for(int i = 0; i < numbers_count - 1; i++)
  {
    for(int j = 0; j < pthread_numbers - 1; j++)
    {
      
      thread[j].odd_flag = i % 2;
      pthread_create(&tread_id[j], NULL, compare, &thread[j]);
    }
    
    for(int j = thread[0].block * (pthread_numbers - 1) + (i % 2); j < numbers_count - 1; j += 2) {
      if (numbers[j] > numbers[j+1]) {
        change(&numbers[j], &numbers[j+1]);
      }
    }


    for(int j = 0; j < pthread_numbers - 1; j++) {
      pthread_join(tread_id[j], NULL);
    }
  }
 
  fclose(r_file);
  end_time += clock();
 
  printf("Work time = %.8f\n", ((float)end_time - start_time) / 10000);
 
  for(int i = 0; i < numbers_count; i++) {
    fprintf(p_file, "%d ", numbers[i]);
  }
  free(numbers);
  fclose(p_file);
  return 0;
}
 
