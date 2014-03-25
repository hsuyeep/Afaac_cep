#define  _BSD_SOURCE

#include "common.h"

#include <byteswap.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_SOCKETS	64


double   rate		   = 195312.5;
unsigned subbands	   = 61;
unsigned samples_per_frame = 16;
char	 packet[9000];
unsigned message_size;
int	 sockets[MAX_SOCKETS];
unsigned nr_sockets	   = 0;
unsigned packets_sent[MAX_SOCKETS], skipped, errors[MAX_SOCKETS];
char     names[MAX_SOCKETS][64];


// Fill the packet buffer with a known pattern
int populate_pkt ()
{ // 16-byte header with timestamp. There are samples_per_frame time samples for 
  // each subband, each sample is two polarizations, complex short
  int message_size = 16 + samples_per_frame * subbands * 8;
  short *payload = (short*)(packet + 16); // Go past the header
  unsigned short cnt = 0;

  for (unsigned short i=0; i<subbands; i++)
  { short *subband_pay = payload + i*samples_per_frame*2*2; // For re/im of 2pols
    for (int j=0; j<samples_per_frame; j++)
    { // Encode the pol/re_im, the subband number and a free running 8-bit count.
      subband_pay [4*j  ] = (cnt&255); // real of pol1
      subband_pay [4*j+1] = (cnt&255); // imag of pol1
      subband_pay [4*j+2] = (cnt&255); // real of pol2
      subband_pay [4*j+3] = (cnt&255); // imag of pol2
//    subband_pay [4*j  ] = (0x0000 + i*256 + cnt); // real of pol1
//    subband_pay [4*j+1] = (0x4000 + i*256 + cnt); // imag of pol1
//    subband_pay [4*j+2] = (0x8000 + i*256 + cnt); // real of pol2
//    subband_pay [4*j+3] = (0xc000 + i*256 + cnt); // imag of pol2
      cnt++;
    }
  }
}

void *log_thread(void *arg)
{
  while (1) {
    sleep(1);

    for (unsigned socket_nr = 0; socket_nr < nr_sockets; socket_nr ++)
      if (packets_sent[socket_nr] > 0 || errors[socket_nr] > 0)  {
	fprintf(stderr, "sent %u packets to %s, skipped = %u, errors = %u, mesg_size = %u\n", packets_sent[socket_nr], names[socket_nr], skipped, errors[socket_nr], message_size);
	packets_sent[socket_nr] = errors[socket_nr] = 0; // ignore race
      }

    skipped = 0;
  }

  return 0;
}


void send_packet(unsigned socket_nr, unsigned seconds, unsigned fraction)
{
#if defined __BIG_ENDIAN__
  * (int *) (packet +  8) = __bswap_32(seconds);
  * (int *) (packet + 12) = __bswap_32(fraction);
#else
  * (int *) (packet +  8) = seconds;
  * (int *) (packet + 12) = fraction;
#endif

  ++ packets_sent[socket_nr];

#if 1
  for (unsigned bytes_written = 0; bytes_written < message_size;) {
    ssize_t retval = write(sockets[socket_nr], packet + bytes_written, message_size - bytes_written);

    if (retval < 0) {
      ++ errors[socket_nr];
      perror("write");
      sleep(1);
      break;
    } else {
      bytes_written += retval;
    }
  }
#endif
}


void parse_args(int argc, char **argv)
{
  if (argc == 1) {
    fprintf(stderr, "usage: %s [-f frequency (default 195312.5)] [-s subbands (default 61)] [-t times_per_frame (default 16)] [udp:ip:port | tcp:ip:port | file:name | null: | - ] ... \n", argv[0]);
    exit(1);
  }

  int arg;

  for (arg = 1; arg < argc && argv[arg][0] == '-'; arg ++)
    switch (argv[arg][1]) {
      case 'a': set_affinity(argument(&arg, argv));
		break;

      case 'f': rate = atof(argument(&arg, argv));
		break;

      case 'r': set_real_time_priority();
		break;

      case 's': subbands = atoi(argument(&arg, argv));
		break;

      case 't': samples_per_frame = atoi(argument(&arg, argv));
		break;

      default : fprintf(stderr, "unrecognized option '%c'\n", argv[arg][1]);
		exit(1);
    }

  if (arg == argc)
    exit(0);

  enum proto proto;

  for (nr_sockets = 0; arg != argc && nr_sockets < MAX_SOCKETS; arg ++, nr_sockets ++)
    sockets[nr_sockets] = create_fd(argv[arg], 1, &proto, names[nr_sockets], sizeof names[nr_sockets]);

  if (arg != argc)
    fprintf(stderr, "Warning: too many sockets specified\n");
}


int main(int argc, char **argv)
{
  if_BGP_set_default_affinity();
  parse_args(argc, argv);
  message_size = 16 + samples_per_frame * subbands * 8;

  pthread_t thread;

  if (pthread_create(&thread, 0, log_thread, 0) != 0) {
    perror("pthread_create");
    exit(1);
  }

  unsigned clock_speed = 1024 * rate;
  struct timeval now;
  gettimeofday(&now, 0);
  unsigned long long packet_time = (now.tv_sec + now.tv_usec / 1e6) * rate;

  // while (1) {
  for (int pkt=0; pkt<10; pkt++) {
    packet_time += samples_per_frame;

    gettimeofday(&now, 0);
    unsigned long long now_us = 1000000ULL * now.tv_sec + now.tv_usec;
    unsigned long long pkt_us = 1000000ULL * (packet_time / rate);

    long long wait_us = pkt_us - now_us;

    if (wait_us > 10)
      usleep(wait_us);
    else if (pkt_us + 100000 < now_us) {
      unsigned skip = (unsigned long long) (-wait_us * 1e-6 * rate) / samples_per_frame;
      skipped += skip;
      packet_time += skip * samples_per_frame; // skip packets; keep modulo(samples_per_frame)
    }

    unsigned seconds  = 1024 * packet_time / clock_speed;
    unsigned fraction = 1024 * packet_time % clock_speed / 1024;

    for (unsigned socket_nr = 0; socket_nr < nr_sockets; socket_nr ++)
    { populate_pkt ();
      send_packet(socket_nr, seconds, fraction);
      fprintf (stderr, "Timestamp: 0x%x%x\n", *((int*)(packet+8)), 
               *((int*)(packet+12))); 
    }
  }

  return 0;
}
