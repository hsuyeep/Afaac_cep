#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s stream_nr", argv[0]);
    exit(1);
  }

  unsigned stream_nr = atoi(argv[1]);

  if (stream_nr >= 12) {
    fprintf(stderr, "stream_nr must be < 12");
    exit(1);
  }

  struct header {
    char bytes[16];
  };

  struct {
    struct header header;
    char payload[61][16][2][2];
  } in_packet;

  struct {
    struct header header;
    signed char	  payload[5][16][2][2];
  } out_packet;

  unsigned long long good = 0, bad = 0;
  unsigned long long *head = ((unsigned long long*)(&in_packet.header));

  while (fread(&in_packet, sizeof in_packet, 1, stdin) == 1) 
  { // fprintf (stderr, "0x%LX%LX\n", head[0], head[1]);
    out_packet.header = in_packet.header;

    for (unsigned i = 0; i < 5 * 16 * 2 * 2; i ++) {
      char value = in_packet.payload[5 * stream_nr][0][0][i];

      if (value < -128)
	value = -128, ++ bad;

      if (value > 127)
	value = 127, ++ bad;

      out_packet.payload[0][0][0][i] = value;
    }

    good += 5 * 16 * 2 * 2;

    if (fwrite(&out_packet, sizeof out_packet, 1, stdout) != 1) {
      perror("could not write in_packet");
      exit(1);
    }
  }

  fprintf(stderr, "good = %llu, bad = %llu\n", good, bad);
  return 0;
}
