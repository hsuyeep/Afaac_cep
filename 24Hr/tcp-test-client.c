/*
CIOD:
/bgsys/drivers/ppcfloor/gnu-linux/bin/powerpc-bgp-linux-gcc -O2 tcp-test-client.c -o tcp-test-client -std=c99 -I/bgsys/drivers/ppcfloor/comm/include -lmpich.cnk -ldcmfcoll.cnk -ldcmf.cnk -lpthread -lrt -lSPI.cna -L/bgsys/drivers/ppcfloor/comm/lib -L/bgsys/drivers/ppcfloor/runtime/SPI

ZOID:
/bgl/BlueLight/ppcfloor/blrts-gnu/bin/powerpc-bgl-blrts-gnu-gcc -std=gnu99 -O2 tcp-test-client.c -o tcp-test-client -I/bgl/BlueLight/ppcfloor/bglsys/include -L/cephome/romein/projects/zoid/glibc-build-zoid -L/bgl/BlueLight/ppcfloor/bglsys/lib -lmpich.rts -lmsglayer.rts -lrts.rts -ldevices.rts -lnss_files -lnss_dns -lresolv -lc -lnss_files -lnss_dns -lresolv -lm
*/

#define HAVE_MPI

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined HAVE_MPI
#include <mpi.h>
#endif

#define MAX_MESSAGE_SIZE	(128 * 1024 * 1024)


unsigned long long rdtsc(void)
{
  unsigned high, low, retry;

   __asm__ __volatile__
  (
    "0:\n\t"
    "mfspr %0,269\n\t"
    "mfspr %1,268\n\t"
    "mfspr %2,269\n\t"
    "cmpw %2,%0\n\t"
    "bne- 0b\n\t"
  :
    "=r" (high), "=r" (low), "=r" (retry)
  );
  
  return ((unsigned long long) high << 32) + low;
}


int  sock;
char buffer[MAX_MESSAGE_SIZE] __attribute__((aligned(16)));


void create_TCP_connection(const char *client, short port)
{
  struct sockaddr_in sa;
  struct hostent     *host;

  if ((host = gethostbyname(client)) == 0) {
    perror("gethostbyname");
    exit(1);
  }

  memcpy(&sa.sin_addr, host->h_addr, host->h_length);
  sa.sin_family       = AF_INET;
  sa.sin_port         = htons(port);

  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  while (connect(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
    if (errno == ECONNREFUSED) {
      sleep(1);
    } else {
      perror("connect");
      exit(1);
    }
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
  for (size_t bytes_to_go = size; bytes_to_go > 0;) {
    ssize_t retval;

    if ((retval = write(sock, buffer, bytes_to_go)) < 0) {
      perror("write");
      exit(1);
    } else {
      bytes_to_go -= retval;
    }
  }
}


double do_read(size_t size)
{
  for (size_t bytes_to_go = size; bytes_to_go > 0;) {
    ssize_t retval;

    if ((retval = read(sock, buffer, bytes_to_go)) < 0) {
      perror("read");
      exit(1);
    } else {
      bytes_to_go -= retval;
    }
  }
}


int main(int argc, char **argv)
{
  int rank;

#if defined HAVE_MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#else
  rank = 0;
#endif

  //create_TCP_connection(rank == 0 ? "127.0.0.1" : "127.0.0.1", rank == 0 ? 4567 : 4568);
  open_named_pipe("/cephome/romein/src/pipe");

  for (ssize_t size = 16; size <= MAX_MESSAGE_SIZE; size <<= 1) {
    unsigned total = (1 << 29) / size;

    if (total > 65536)
      total = 65536;

    unsigned long long start_time = rdtsc();

    for (int count = 0; count < total; count ++)
      if (rank == 0)
	do_write(size);
      else
	do_write(size);

    unsigned long long stop_time = rdtsc();
    double s	= (stop_time - start_time) / 700e6;
    double B	= total * size;
    double MB	= B / 1e6;
    double MB_s = MB / s;
    double Mb_s = 8 * MB_s;

    printf("%s: size = %u, avg = %lf Mb/s\n", rank == 0 ? "read" : "write", size, Mb_s);
  }

#if defined HAVE_MPI
  MPI_Finalize();
#endif

  return 0;
}
