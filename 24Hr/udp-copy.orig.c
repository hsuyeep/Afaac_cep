/* gcc -O2 udp-copy.c -o udp-copy -I/usr/local/ofed/mpi/gcc/mvapich-0.9.7-mlx2.2.0/include -L/usr/local/ofed/mpi/gcc/mvapich-0.9.7-mlx2.2.0/lib/shared/ -lmpich */

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define IP(A,B,C,D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

static const struct map {
  struct pair {
    unsigned	   ip_address;
    unsigned short port;
  } source, destination;
  const char *name;
} mapping[] = {
  IP(10,161,0, 1), 4346, IP(10,170,0, 2), 4346, "CS001_RSP0",
  IP(10,161,0, 1), 4347, IP(10,170,0, 4), 4347, "CS001_RSP1",
  IP(10,161,0, 2), 4348, IP(10,170,0, 1), 4348, "CS001_RSP2",
  IP(10,161,0, 2), 4349, IP(10,170,0, 3), 4349, "CS001_RSP3",
  IP(10,161,0, 3), 4346, IP(10,170,0,10), 4346, "CS008_RSP0",
  IP(10,161,0, 3), 4347, IP(10,170,0, 9), 4347, "CS008_RSP1",
  IP(10,161,0, 4), 4348, IP(10,170,0,12), 4348, "CS008_RSP2",
  IP(10,161,0, 4), 4349, IP(10,170,0,11), 4349, "CS008_RSP3",
  IP(10,161,0, 5), 4346, IP(10,170,0,18), 4346, "CS010_RSP0/CS030_RSP0",
  IP(10,161,0, 5), 4347, IP(10,170,0,17), 4347, "CS010_RSP1/CS030_RSP1",
  IP(10,161,0, 6), 4348, IP(10,170,0,20), 4348, "CS010_RSP2/CS030_RSP2",
  IP(10,161,0, 6), 4349, IP(10,170,0,19), 4349, "CS010_RSP3/CS030_RSP3",
  IP(10,161,0, 7), 4346, IP(10,170,0,26), 4346, "CS031_RSP0",
  IP(10,161,0, 7), 4347, IP(10,170,0,25), 4347, "CS031_RSP1",
  IP(10,161,0, 8), 4348, IP(10,170,0,28), 4348, "CS031_RSP2",
  IP(10,161,0, 8), 4349, IP(10,170,0,27), 4349, "CS031_RSP3",
  IP(10,161,0, 9), 4346, IP(10,170,0,34), 4346, "CS032_RSP0",
  IP(10,161,0, 9), 4347, IP(10,170,0,33), 4347, "CS032_RSP1",
  IP(10,161,0,10), 4348, IP(10,170,0,36), 4348, "CS032_RSP2",
  IP(10,161,0,10), 4349, IP(10,170,0,35), 4349, "CS032_RSP3",
  IP(10,161,0,11), 4346, IP(10,170,0,42), 4346, "CS016_RSP0",
  IP(10,161,0,11), 4347, IP(10,170,0,41), 4347, "CS016_RSP1",
  IP(10,161,0,12), 4348, IP(10,170,0,44), 4348, "CS016_RSP2",
  /*IP(10,161,0,12), 4349, IP(10,170,0,43), 4349, "CS016_RSP3",*/
  IP(10,161,0,12), 4349, IP(10,170,0,52), 4349, "CS016_RSP3",
};

int rank;
int sk_in, sk_out;

void init(int argc, char **argv)
{
  struct sockaddr_in sa;
  int    receive_buffer_size;

  if (argc != 2 && argc != 3) {
    fprintf(stderr, "Usage: %s rank [filename]\n", argv[0]);
    exit(1);
  }

  rank = atoi(argv[1]);

  if ((sk_in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket");
    exit(1);
  }

  memset(&sa, 0, sizeof sa);
  sa.sin_family       = AF_INET;
  sa.sin_addr.s_addr  = htonl(mapping[rank].source.ip_address);
  sa.sin_port         = htons(mapping[rank].source.port);

  if (bind(sk_in, (struct sockaddr *) &sa, sizeof sa) < 0) {
    perror("bind");
    exit(1);
  }

  receive_buffer_size = 8 * 1024 * 1024;

  if (setsockopt(sk_in, SOL_SOCKET, SO_RCVBUF, &receive_buffer_size, sizeof receive_buffer_size) < 0)
    perror("setsockopt failed:");

  if (argc == 2) {
    if ((sk_out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("socket");
      exit(1);
    }

    memset(&sa, 0, sizeof sa);
    sa.sin_family       = AF_INET;
    sa.sin_addr.s_addr  = htonl(mapping[rank].destination.ip_address);
    sa.sin_port         = htons(mapping[rank].destination.port);

    if (connect(sk_out, (struct sockaddr *) &sa, sizeof sa) < 0) {
      perror("connect");
      exit(1);
    }
  } else {
    char filename[4096];

    sprintf(filename, "%s.%s", argv[2], mapping[rank].name);

    if ((sk_out = open(filename, O_CREAT | O_WRONLY, 0664)) < 0) {
      perror("open");
      exit(1);
    }
  }
}


int main(int argc, char **argv)
{
  time_t   previous_time = 0, current_time;
  unsigned i, forwarded = 0;

  init(argc, argv);

  while (1) {
    char buffer[9000];
    int  size;

    if ((size = read(sk_in, buffer, 9000)) < 0) {
      perror("read");
      sleep(1);
    } else {
#if 0
      if (rank == 1)
	for (i = 0; i < 32; i ++)
	  ((short *) buffer + 16 + 22)[i] *= 16;
      else if (rank == 2)
	memset(buffer + 16 + 22 * 16 * 8, 0, 16 * 8);
#endif

      if (write(sk_out, buffer, size) < size) {
	perror("write");
	sleep(1);
      }
    }

    ++ forwarded;

    if ((current_time = time(0)) != previous_time) {
      previous_time = current_time;
      fprintf(stderr, "%d: forwarded %u packets\n", rank, forwarded);
      forwarded = 0;
    }
  }

  return 0;
}
