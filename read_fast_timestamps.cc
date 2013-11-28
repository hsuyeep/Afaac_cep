#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <time.h>

/* compile with
 *
 * g++ -O3 read_fast_timestamps.cc  -std=c++0x -o read_fast_timestamps
 *
 */

// All data is in Little Endian format!
//
#define NRSUBBANDS 24 
#define NRTIMES 16
#define NRPOL 2

struct RSP {
  struct Header {
    uint8_t  version;
    uint8_t  sourceInfo;
    uint16_t configuration;
    uint16_t station;
    uint8_t  nrBeamlets;
    uint8_t  nrBlocks;
    uint32_t timestamp;
    uint32_t blockSequenceNumber;
  } header;

  char       data[NRSUBBANDS * NRTIMES * NRPOL * 2];
} __attribute((__packed__));

int main(int argc, char *argv[]) {
  struct RSP packet;
  char buf[26];
  FILE *fid = fopen (argv[1], "rb");

  // First packet.
  fread(&packet, sizeof packet, 1, fid);
  time_t seconds = packet.header.timestamp;
  ctime_r(&seconds, buf);
  printf("Start time: %10u %8u = %s", packet.header.timestamp, packet.header.blockSequenceNumber, buf);

  // Last packet
  fseek (fid, -sizeof (packet), SEEK_END); 
  fread(&packet, sizeof packet, 1, fid);
  seconds = packet.header.timestamp;
  ctime_r(&seconds, buf);
  printf("End   time: %10u %8u = %s", packet.header.timestamp, packet.header.blockSequenceNumber, buf);
  
  fclose (fid);
}
