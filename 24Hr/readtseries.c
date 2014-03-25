// Program to recover a timeseries from per dipole raw data as recorded by 
// udp-copy.c 
// pep/28Oct13

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>

typedef struct {
    uint8_t  version;
    uint8_t  sourceInfo;
    uint16_t configuration;
    uint16_t station;
    uint8_t  nrBeamlets;
    uint8_t  nrBlocks;
    uint32_t timestamp;
    uint32_t blockSequenceNumber;
} HdrType;

typedef  struct 
{ HdrType header;
  // short payload[61][16][2][2];
  char payload[61][16][2][2];
} InPktType;

enum {MaxPkt=16};
int main (int argc, char *argv[])
{ time_t seconds;

  InPktType *pktbuf = calloc (sizeof (InPktType), 128);
  fprintf (stderr, "%% Size of InPktType: %lu\n", sizeof (InPktType));
  if (pktbuf == NULL) perror ("calloc:");

  FILE *fraw = fopen (argv[2], "rb");
  if (fraw == NULL) perror ("fopen");

  int stream_nr = atoi (argv[1]);
  int pkt = 0;
  while (fread (pktbuf, 1, MaxPkt*sizeof (InPktType), fraw) != sizeof (InPktType))
  { for (pkt=0; pkt<MaxPkt; pkt++)
    { 
      seconds = pktbuf[pkt].header.timestamp;
      // fprintf (stderr, "# Timestamp: %10lu %8u = %s\n", pktbuf[0].header.timestamp, pktbuf[0].header.blockSequenceNumber, ctime(&seconds));
  
      for (unsigned i=0; i<16; i ++) 
      // for (unsigned i=0; i<5 * 16 * 2 * 2; i ++) 
      {
        short re = pktbuf[pkt].payload[5 * stream_nr][i][1][0];
        short im = pktbuf[pkt].payload[5 * stream_nr][i][1][1];
        fprintf (stderr, "%4d %4d\n", re, im);
      }
    }
  }

  if (pktbuf) free (pktbuf);
  if (fraw) fclose (fraw);

  return 0;
}
