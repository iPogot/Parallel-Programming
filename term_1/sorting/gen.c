#include <stdio.h>
#include <stdlib.h>

int main()
{
  FILE *file;
  file = fopen("numbers.txt", "w");
  fprintf(file, "20000\n");
  for(long int i = 0; i < 20000; i++)
  {
    fprintf(file, "%d ", rand() % 60000 - 30000);
  }
  fclose(file);
  return 0;

}
