#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_MESSAGE_SIZE        (128 * 1024 * 1024)


int server_socket, sock;
char buffer[MAX_MESSAGE_SIZE] __attribute__((aligned(16)));


void do_connect(const char *server, short port)
{
  struct sockaddr_in sa;
  struct hostent     *this_host;

  if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  if ((this_host = gethostbyname(server)) == 0) {
    perror("gethostbyname");
    exit(1);
  }

  memcpy(&sa.sin_addr, this_host->h_addr, this_host->h_length);
  sa.sin_family = AF_INET;
  sa.sin_port   = htons(port);

  if (bind(server_socket, (struct sockaddr *) &sa, sizeof sa) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(server_socket, 5) < 0) {
    perror("listen");
    exit(1);
  }

  if ((sock = accept(server_socket, 0, 0)) < 0) {
    perror("accept");
    exit(1);
  }
}


void open_named_pipe(const char *name)
{
  if ((sock = open(name, O_RDWR)) < 0) {
    perror("open");
    exit(1);
  }
}


void do_write(size_t size)
{
  while (size > 0) {
    ssize_t retval;

    if ((retval = write(sock, buffer, size)) < 0) {
      perror("write");
      exit(1);
    } else {
      size -= retval;
    }
  }
}


void do_read(size_t size)
{
  while (size > 0) {
    ssize_t retval;

    if ((retval = read(sock, buffer, size)) < 0) {
      perror("read");
      exit(1);
    } else {
      size -= retval;
    }
  }
}


int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s 0|1\n", argv[0]);
    exit(1);
  }

  int rank = atoi(argv[1]);
  //do_connect(rank == 0 ? "127.0.0.1" : "127.0.0.1", rank == 0 ? 4567 : 4568);
  open_named_pipe("/cephome/romein/src/pipe");

  for (ssize_t size = 16; size <= MAX_MESSAGE_SIZE; size <<= 1) {
    unsigned total = (1 << 29) / size;

    if (total > 65536)
      total = 65536;

    printf("size = %u\n", size);

    for (int count = 0; count < total; count ++)
      if (rank == 0)
	do_write(size);
      else
	do_read(size);
  }

  return 0;
}
