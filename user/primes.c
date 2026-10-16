#include "kernel/types.h"
#include "user/user.h"

#define MAX_NUMBER 280

static int read_int(int fd, int *value)
{
  int total = 0;

  while(total < sizeof(int)){
    int n = read(fd,
                 (char *)value + total,
                 sizeof(int) - total);

    if(n < 0)
      return -1;

    if(n == 0){
      if(total == 0)
        return 0;

      return -1;
    }

    total += n;
  }

  return 1;
}

static int write_int(int fd, int value)
{
  int total = 0;

  while(total < sizeof(int)){
    int n = write(fd,
                  (char *)&value + total,
                  sizeof(int) - total);

    if(n <= 0)
      return -1;

    total += n;
  }

  return 0;
}

static void sieve(int input_fd) __attribute__((noreturn));

static int wait_child(void)
{
  int status;

  if(wait(&status) < 0)
    return -1;

  if(status != 0)
    return -1;

  return 0;
}

static void sieve(int input_fd)
{
  int prime;
  int number;
  int status;
  int next_pipe[2];
  int pid;

  status = read_int(input_fd, &prime);

  if(status == 0){
    close(input_fd);
    exit(0);
  }

  if(status < 0){
    fprintf(2, "primes: read error\n");
    close(input_fd);
    exit(1);
  }

  printf("prime %d\n", prime);

//Find the first number that survives this filter.
//Only create the next process when such a number exists.
  while((status = read_int(input_fd, &number)) == 1){
    if(number % prime != 0)
      break;
  }

  if(status == 0){
    close(input_fd);
    exit(0);
  }

  if(status < 0){
    fprintf(2, "primes: read error\n");
    close(input_fd);
    exit(1);
  }

  if(pipe(next_pipe) < 0){
    fprintf(2, "primes: pipe failed\n");
    close(input_fd);
    exit(1);
  }

  pid = fork();

  if(pid < 0){
    fprintf(2, "primes: fork failed\n");
    close(input_fd);
    close(next_pipe[0]);
    close(next_pipe[1]);
    exit(1);
  }

  if(pid == 0){
    close(next_pipe[1]);
    close(input_fd);

    sieve(next_pipe[0]);
  }

  close(next_pipe[0]);

  if(write_int(next_pipe[1], number) < 0){
    fprintf(2, "primes: write error\n");
    close(input_fd);
    close(next_pipe[1]);
    wait(0);
    exit(1);
  }

  while((status = read_int(input_fd, &number)) == 1){
    if(number % prime != 0){
      if(write_int(next_pipe[1], number) < 0){
        fprintf(2, "primes: write error\n");
        close(input_fd);
        close(next_pipe[1]);
        wait(0);
        exit(1);
      }
    }
  }

  close(input_fd);
  close(next_pipe[1]);

  if(status < 0){
    fprintf(2, "primes: read error\n");
    wait(0);
    exit(1);
  }

  if(wait_child() < 0)
    exit(1);

  exit(0);
}

int main(int argc, char *argv[])
{
  if(argc != 1){
    fprintf(2, "usage: primes\n");
    exit(1);
  }

  int first_pipe[2];
  int pid;

  if(pipe(first_pipe) < 0){
    fprintf(2, "primes: pipe failed\n");
    exit(1);
  }

  pid = fork();

  if(pid < 0){
    fprintf(2, "primes: fork failed\n");
    close(first_pipe[0]);
    close(first_pipe[1]);
    exit(1);
  }

  if(pid == 0){
    close(first_pipe[1]);
    sieve(first_pipe[0]);
  }

  close(first_pipe[0]);

  for(int number = 2; number <= MAX_NUMBER; number++){
    if(write_int(first_pipe[1], number) < 0){
      fprintf(2, "primes: write error\n");
      close(first_pipe[1]);
      wait(0);
      exit(1);
    }
  }

  close(first_pipe[1]);

  if(wait_child() < 0)
    exit(1);

  exit(0);
}