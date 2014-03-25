#include <stdio.h>

int main ()
{ 
  struct header {
    char bytes[16];
  };

  char head[16] = {0,};

  for (int i=0; i<16; i++) head[i] = 0xf0+i;

  struct {
    struct header header;
    short	  payload[61][16][2][2];
  }in_packet;

  memcpy (in_packet.header.bytes, head, 16);
  for (int i=0; i<61*16*2*2; i++) in_packet.payload[0][0][0][i] = i;
    
  struct {
    struct header header;
    char payload[12][16][2][2]; // 12 dipoles, double subband
  }out1;

  struct {
    struct header header;
    char payload[24][16][2][2]; // 12 dipoles, double subband
  }out2;

  memcpy (&out2.header, head, 16);
  memcpy (&out1.header, head, 16);

      int sb = 2;
  int bad = 0;
      for (int dip=0,ant=0; dip<60; dip+=5,ant++) 
      { for (unsigned i = 0; i < 1 * 16 * 2 * 2; i ++) 
        {
          short value = in_packet.payload[dip+sb][0][0][i]; // Subband 1
    
          if (value < -128)
    	     value = -128, ++ bad;
    
          if (value > 127)
    	     value = 127, ++ bad;
    
          out1.payload[ant][0][0][i] = value & 255;
          out2.payload[2*ant][0][0][i] = value & 255;

          value = in_packet.payload[dip+sb+1][0][0][i]; // Subband 2
    
          if (value < -128)
    	     value = -128, ++ bad;
    
          if (value > 127)
    	     value = 127, ++ bad;
    
          out2.payload[2*ant+1][0][0][i] = value & 255;
        }
      }
/*
  for (int j=0; j<12; j++)
  { for (int i=0; i<16*2*2; i++)
    { out1.payload[j][0][0][i] = i*2 & 255;
    } 
  }

  for (int j=0; j<12; j++)
  { for (int i=0; i<16*2*2; i++)
    { out2.payload[2*j][0][0][i] = i*2 & 255;
      out2.payload[2*j+1][0][0][i] = (i*2+j) & 255;
    } 
  }
*/

  fprintf (stderr, "out1.header: ");
  for (int i=0; i<16; i++)
    fprintf (stderr, "%d ", out1.header.bytes[i]);
  
  fprintf (stderr, "\nout2.header: ");
  for (int i=0; i<16; i++)
    fprintf (stderr, "%d ", out2.header.bytes[i]);

 return 0;
}
