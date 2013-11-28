#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <time.h>

/* compile with
 *
 * g++ -O3 read_timestamps.cc  -std=c++0x -o read_timestamps
 *
 */

// All data is in Little Endian format!
//
#define NRSUBBANDS 2 
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

int main() {
  struct RSP packet;
  char buf[26];

  while( fread(&packet, sizeof packet, 1, stdin) == 1 ) {
    time_t seconds = packet.header.timestamp;
    ctime_r(&seconds, buf);
    printf("%10u %8u = %s\n", packet.header.timestamp, packet.header.blockSequenceNumber, buf);
  }
}
