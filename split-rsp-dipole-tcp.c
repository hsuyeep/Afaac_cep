/* Program to split specified RSP files into per dipole files (288 of them) and 
 * send them over TCP to remote ION processes.
 * Allows skipping split-dipole.sh, thus saving space on local disks.
 * NOTE: All 24 RSP files need to be specified!
 * pep/21Jan14
 * Usage: split-rsp-dipole-tcp /path/to/RSPfile/directory [start port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <dirent.h>
#include <unistd.h>
#include "common.h"

// Packet definitions
typedef struct {
   char bytes[16];
}HdrType;

typedef struct {
   HdrType header;
   // 8bit, 2 subband recording. 16 time samples/frame, byte complex, 2 pol, 
   // 12 dipoles worth 
   char payload[24][16][2][2]; 
} InPktType;

typedef struct {
    HdrType header;
    signed char	  payload[2][16][2][2];
} OutPktType;

int usage (char *progname)
{ fprintf (stderr, "Usage: %s /path/to/RSPfile/directory [start port = 4567]\n",
           progname);
  return 0;
}

enum {NRSP=24, NELEM=288, MAX_OUT_PKTS=1024};
int main (int argc, char **argv)
{ FILE *frsp[NRSP];
  OutPktType* outpkt[NELEM] = {NULL,};
  InPktType inpkt;
  int i = 0, fil=0, dip=0, stream_nr=0, pktcnt=0, j=0;
  unsigned int bad = 0;
  int station = 0, rsp = 0;
  char substr[16] = {0,};
  char testdat[128] = {0,};

  // NOTE limit on name size! Also hardcoded addresses and ports.
  int sk_out[NELEM] = {0,};
  int tcpport = 4567;
  char ipport[128] = {0,};
  char tcpsrc[32] = "10.142.5.94", tcpdest[32] = "10.141.5.23"; 
  
  // Write out per dipole data to file
  #if 1 // def DEBUG
  FILE *fout[NELEM] = {NULL,};
  for (i=0; i<NELEM; i++)
  { sprintf(testdat, "%03d.dip", i);
    if ((fout[i]=fopen(testdat, "wb")) == NULL)
     perror ("fopen");
  }
  #endif

  if (argc < 2) {usage(argv[0]); exit (-1);}
  
/* //TODO currently just specify filename on cmdline via shell regex.
  // Get RSP filenames
  DIR *dirp = NULL;
  struct dirent *dp = NULL;
  if ((dirp=opendir(argv[1])) == NULL)
  { perror ("opendir"); return -1;}

  if ((dp = readdir (dirp)) != NULL)
  { if (strcmp (
  }
*/

  // Open files
  for (i=0; i<NRSP; i++)
  { if ((frsp[i]=fopen (argv[1+i], "rb")) == NULL)
    { perror ("fopen"); break;}
	else
      fprintf (stderr, "Opened %s successfully.\n", argv[i+1]);
  }
  if (i<NRSP) { fprintf (stderr, "File problem!\n"); exit (-1);}
  
  fprintf (stderr, "Size of inpacket: %d, outpkt: %d bytes.\n", sizeof (InPktType), sizeof (OutPktType));
  // Create memory buffers
  for (i=0; i<NELEM; i++, tcpport++)
  { if ((outpkt[i]=(OutPktType*)calloc (sizeof (OutPktType), MAX_OUT_PKTS)) == NULL) 
	{ perror ("calloc"); break; }

    // Generate tcp streams to receiver. Starting port number configurable
    fprintf (stderr, "Creating TCP connection to %s:%d\n", tcpdest, tcpport);
    sprintf (ipport, "%s:%d", tcpdest, tcpport);
    sk_out[i] = create_IP_socket (ipport, 1, TCP);
  }
  if (i<NELEM) { fprintf (stderr, "calloc problem!\n"); exit (-1);}

  // For each RSP file, do till end of file:
  for (j=0; j<1; j++)
  for (fil=0; fil<NRSP; fil++)
  { if (fread (&inpkt, 1, sizeof (InPktType), frsp[fil]) < sizeof(InPktType))
	{ perror ("fread"); }
    
  	// Each RSP file packet has 12 streams or dipoles
  	// Check name of this file, set stream_nr accordingly
  	#ifdef DEBUG
  	fprintf (stderr, "%s\n", strstr(argv[fil+1], "CS00"));
  	#endif
  	strncpy (substr, strstr(argv[fil+1], "CS00"), 5); substr[5] = 0; 
  	station = atoi (substr+4);
  
  	strncpy (substr, strstr(argv[fil+1], "RSP"), 4); substr[4] = 0;
      rsp = atoi (substr+3);
  	#if 0 // # DEBUG
  	fprintf (stderr, "Working on station %d, RSP %d. Dip = %d, stream = %d\n",
               station, rsp, (station-2)*48, rsp*12); 
  	
  	#endif 

	// 12 dipole's data within an RSP board packet
	for (dip=(station-2)*48 + rsp*12,stream_nr=0; stream_nr<12; stream_nr++, dip++)
    { 

    #if 1 //def DEBUG
	  fprintf (stderr, "Working on station %d, RSP %d. Dip = %d, stream = %d\n",
             station, rsp, dip, stream_nr); 
	  // fprintf (stderr, "Writing data: %s", testdat);
	  //if (write (sk_out[dip], testdat, strlen(testdat)) < strlen(testdat))
 	  // { perror ("write"); }
	#endif

	  // Split into per dipole structures, fill dedicated buffer
	  memcpy ((void*)&(outpkt[dip]->header), (void*)(&inpkt.header), 
			  sizeof (HdrType));
      for (i=0; i< 1 * 16 * 2 * 2; i++) 
      { char value = inpkt.payload[2*stream_nr][0][0][i]; // Subband 1
  
        if (value < -128) value = -128, ++ bad;
        if (value >  127) value =  127, ++ bad;
  
        outpkt[dip]->payload[0][0][0][i] = value;
  
        value = inpkt.payload[2*stream_nr+1][0][0][i]; // Subband 2
  
        if (value < -128) value = -128, ++ bad;
        if (value >  127) value =  127, ++ bad;
  
        outpkt[dip]->payload[1][0][0][i] = value;
      }
      fwrite (outpkt[dip], 1, sizeof (OutPktType), fout[dip]); 
	  if (write (sk_out[dip], outpkt[dip], sizeof (OutPktType)) < sizeof (OutPktType))
	  { perror ("write"); }
	  else pktcnt++;
    }
  }
  
  fprintf (stderr, "Sent %d packets.\n", pktcnt);
  // Free buffers
  for (i=0; i<NELEM; i++)  if (outpkt[i]) free (outpkt[i]);

  // Close TCP connections

  // close files
  for (i=0; i<NRSP; i++)  if (frsp[i] != NULL) fclose (frsp[i]);

  return 0;
}
