/*
 * By Andrew Osborne
 * This program creates a key file of a specified length using standard UNIX
 * randomization methods.
 *
 * Example syntax:
 *
 * keygen keylength
 *
 * where keylength is the length of the key file in characters. keygen outputs
 * to stdout. If keylength specificies 256 characters, the resultant file will
 * be 257 characters in length due to the added newline.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static char const alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
static int LEN_ALPHA = 27;

int main(int argc, char *argv[]) 
{
  int keylength, k;

  if (argc < 2) {
    perror("invalid number of arguments!");
    exit(EXIT_FAILURE);
  }

  keylength = atoi(argv[1]);

  unsigned int seed = time(NULL);
  srand(seed);

  for (int i=0; i < keylength; i++) {
    k = rand() % LEN_ALPHA;
    printf("%c", alpha[k]);
  }

  printf("\n");

  exit(EXIT_SUCCESS);

} 
