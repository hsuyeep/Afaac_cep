/* split-rsp-8bit2sb modified to write out all dipoles simultaneously.
 * pep/25Nov13
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "usage: %s RSPfile.raw filesuffix", argv[0]);
    exit(1);
  }

  FILE *fin = fopen (argv[1], "rb");
  int dipoff = 0;
  int statoff = 2;
  int rsp = 0;
  char fname [128];
  sscanf (strstr (argv[1], "RSP"), "RSP%d", &rsp);
  
  switch (rsp)
  {
    case 0: dipoff =  0; break; // dipole numbers 00-11
    case 1: dipoff = 12; break; // dipole numbers 12-23
    case 2: dipoff = 24; break; // dipole numbers 24-35
    case 3: dipoff = 36; break; // dipole numbers 36-47
  };

  
  struct header {
    char bytes[16];
  };

  struct {
    struct header header;
    char payload[24][16][2][2];
  } in_packet;

  struct {
    struct header header;
    signed char	  payload[2][16][2][2];
  } out_packet;

  unsigned long long good = 0, bad = 0;
  unsigned long long *head = ((unsigned long long*)(&in_packet.header));

  // Open 12 files corresponding to the 12 dipoles available within a single RSP 
  // file. Find out Dipole number to append to filename.
  
  FILE *fout[12];
  char stationname[16];
  strncpy (stationname, strtok (strrchr (argv[1], '/')+1, "-"), 16);

  for (int i=0; i<12; i++)
  { sprintf (fname, "%sD%02d_%s.dip", stationname, dipoff+i, argv[2]);
    fout[i] = fopen (fname, "wb");
  }

  while (fread(&in_packet, sizeof in_packet, 1, fin) == 1) 
  { // fprintf (stderr, "0x%LX%LX\n", head[0], head[1]);
    out_packet.header = in_packet.header;

    for (int dipno=0; dipno<12; dipno++)
    { // Construct a packet containing data from a single dipole at offset dipno.
      for (unsigned i = 0; i < 1 * 16 * 2 * 2; i ++)
      { char value = in_packet.payload[2*dipno][0][0][i]; // Subband 1
  
        if (value < -128)
  	value = -128, ++ bad;
  
        if (value > 127)
  	value = 127, ++ bad;
  
        out_packet.payload[0][0][0][i] = value;
  
        value = in_packet.payload[2*dipno+1][0][0][i]; // Subband 2
  
        if (value < -128)
  	value = -128, ++ bad;
  
        if (value > 127)
  	value = 127, ++ bad;
  
        out_packet.payload[1][0][0][i] = value;
      }
      good += 1 * 16 * 2 * 2 * 2;

      if (fwrite(&out_packet, sizeof out_packet, 1, fout[dipno]) != 1) 
      { perror("could not write in_packet");
        exit(1);
      }
    }
  }

  fprintf(stderr, "good = %llu, bad = %llu\n", good, bad);

  for (int i=0; i<12; i++)
    if (fout[i]) fclose (fout[i]);

  return 0;
}
