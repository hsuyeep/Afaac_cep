/* Copyright 2008, John W. Romein, Stichting ASTRON
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <errno.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <netdb.h>
#include <netinet/in.h>
#if 1
#include <assert.h>
#include <poll.h>
#include <sys/mman.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


enum proto { UDP, TCP, File, Eth, StdIn, StdOut, StdErr } input_proto, output_proto;

int  sk_in, sk_out;
char *source, *destination;

#if 1
void *ring_buffer;
#endif


static void   *flatMemoryAddress = (void *) 0x50000000;
static size_t flatMemorySize     = 1536 * 1024 * 1024;

static void mmapFlatMemory()
  /* 
    
  mmap a fixed area of flat memory space to increase performance. 
  currently only 1.5 GiB can be allocated, we mmap() the maximum
  available amount

  */
{
  int fd = open("/dev/flatmem", O_RDONLY);

  if (fd < 0) { 
    perror("open(\"/dev/flatmem\", O_RDONLY)");
    exit(1);
  }
 
  if (mmap(flatMemoryAddress, flatMemorySize, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED) {
    perror("mmap flat memory");
    exit(1);
  } 
    
  close(fd);
}   
    

int create_IP_socket(char *arg, int is_output, enum proto proto)
{
  char		     *colon;
  struct sockaddr_in sa;
  struct hostent     *host;
  int		     sk, old_sk, buffer_size = 8 * 1024 * 1024;
  unsigned short     port;
  
  if ((colon = strchr(arg, ':')) == 0) {
    fprintf(stderr, "badly formatted IP:PORT address");
    exit(1);
  }

  port = colon[1] != '\0' ? atoi(colon + 1) : 0;
  *colon = '\0';

  if ((host = gethostbyname(arg)) == 0) {
    perror("gethostbyname");
    exit(1);
  }

  memset(&sa, 0, sizeof sa);
  sa.sin_family = AF_INET;
  sa.sin_port   = htons(port);
  memcpy(&sa.sin_addr, host->h_addr, host->h_length);

  if ((sk = socket(AF_INET, proto == UDP ? SOCK_DGRAM : SOCK_STREAM, proto == UDP ? IPPROTO_UDP : IPPROTO_TCP)) < 0) {
    perror("socket");
    exit(1);
  }

  if (is_output) {
    while (connect(sk, (struct sockaddr *) &sa, sizeof sa) < 0) {
      if (errno == ECONNREFUSED) {
	sleep(1);
      } else {
	perror("connect");
	exit(1);
      }
    }

    //if (setsockopt(sk, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof buffer_size) < 0)
      //perror("setsockopt failed");
  } else {
    if (bind(sk, (struct sockaddr *) &sa, sizeof sa) < 0) {
      perror("bind");
      exit(1);
    }

    if (proto == TCP) {
      listen(sk, 5);
      old_sk = sk;

      if ((sk = accept(sk, 0, 0)) < 0) {
	perror("accept");
	exit(1);
      }
      
      close(old_sk);
    }

    //if (setsockopt(sk, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof buffer_size) < 0)
      //perror("setsockopt failed");
  }

#if 0
  struct tpacket_req req;

  req.tp_block_size = 16384;
  req.tp_block_nr   = 64;
  req.tp_frame_size = 8192;
  req.tp_frame_nr   = req.tp_block_size / req.tp_frame_size * req.tp_block_nr;

  if (setsockopt(sk, SOL_SOCKET, PACKET_RX_RING, &req, sizeof req) < 0) {
    perror("ring buffer setsockopt");
    exit(1);
  }
#endif

#if 0
  int *buf;

  if ((buf = mmap(0, req.tp_block_size * req.tp_block_nr, PROT_READ|PROT_WRITE, MAP_SHARED, sk, 0)) == MAP_FAILED) {
    perror("ring buffer mmap");
    exit(1);
  }

  printf("%d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
#endif

  return sk;
}


int create_file(char *arg, int is_output)
{
  int fd;

  if ((fd = open(arg, is_output ? O_CREAT | O_WRONLY : O_RDONLY, 0666)) < 0) {
    perror("opening input file");
    exit(1);
  }

  return fd;
}


int create_stdio(int is_output, enum proto proto)
{
  switch (proto) {
    case StdIn	: if (is_output) {
		    fprintf(stderr, "Cannot write to stdin\n");
		    exit(1);
		  }

		  return dup(0);

    case StdOut	: if (!is_output) {
		    fprintf(stderr, "Cannot read from stdout\n");
		    exit(1);
		  }

		  return dup(1);

    case StdErr	: if (!is_output) {
		    fprintf(stderr, "Cannot read from stdout\n");
		    exit(1);
		  }

		  return dup(2);
  }
}


void read_mac(const char *arg, char mac[6])
{
  if (sscanf(arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", mac, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5) != 6) {
    fprintf(stderr, "bad MAC address");
    exit(1);
  }
}


int create_raw_eth_socket(char *arg, int is_output)
{
  int sk;

  if ((sk = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
    perror("raw socket");
    exit(1);
  }

  char  src_mac[6], dst_mac[6], proto;
  short type, dst_port;
  int   has_src_mac = 0, has_dst_mac = 0, has_type = 0, has_proto = 0;
  int	has_dst_port = 0;

  if (strtok(arg, ",") != 0) {
    do {
      if (strncmp("src=", arg, 4) == 0) {
	read_mac(arg + 4, src_mac);
	has_src_mac = 1;
      } else if (strncmp("dst=", arg, 4) == 0) {
	read_mac(arg + 4, dst_mac);
	has_dst_mac = 1;
      } else if (strncmp("type=", arg, 5) == 0) {
	type = atoi(arg + 5);
	has_type = 1;
      } else if (strncmp("proto=", arg, 6) == 0) {
	proto = atoi(arg + 6);
	has_proto = 1;
      } else if (strncmp("dst_port=", arg, 9) == 0) {
	dst_port = atoi(arg + 9);
	has_dst_port = 1;
      } else {
	fprintf(stderr, "unknown option \"%s\"", arg);
	exit(1);
      }
    } while ((arg = strtok(0, ",")) != 0);

#define MAX_FILTER_LENGTH 16
    struct sock_filter mac_filter_insn[MAX_FILTER_LENGTH], *prog = mac_filter_insn + MAX_FILTER_LENGTH;
    unsigned jump_offset = 1;

    // FILTER IS CONSTRUCTED IN REVERSED ORDER!
    //
    * -- prog = (struct sock_filter) BPF_STMT(BPF_RET + BPF_K, 0); // wrong packet; ret 0
    * -- prog = (struct sock_filter) BPF_STMT(BPF_RET + BPF_K, 65535); // right packet; ret everything
    
    if (has_proto) {
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, proto, 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_B + BPF_ABS , 14 + 9);
      jump_offset += 2;
    }
    
    if (has_type) {
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, htons(type), 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_H + BPF_ABS , 12);
      jump_offset += 2;
    }

    if (has_dst_port) {
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, htons(dst_port), 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_H + BPF_ABS , 14 + 20 + 2);
      jump_offset += 2;
    }

    if (has_src_mac) {
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, htonl(* (int *) (src_mac + 2)), 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_W + BPF_ABS , 8);
      jump_offset += 2;
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, htons(* (short *) src_mac), 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_H + BPF_ABS , 6);
      jump_offset += 2;
    }

    if (has_dst_mac) {
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, htons(* (short *) (dst_mac + 4)), 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_H + BPF_ABS , 4);
      jump_offset += 2;
      * -- prog = (struct sock_filter) BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, htonl(* (int *) dst_mac), 0 , jump_offset);
      * -- prog = (struct sock_filter) BPF_STMT(BPF_LD + BPF_W + BPF_ABS , 0);
      jump_offset += 2;
    }

    struct sock_fprog filter;
    memset(&filter, 0, sizeof(struct sock_fprog));
    filter.filter = prog;
    filter.len = mac_filter_insn + MAX_FILTER_LENGTH - prog;

    if (setsockopt(sk, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) < 0) {
      perror("error creating filter");
      exit(1);
    }
  }

#if 1
  struct tpacket_req req;

  req.tp_block_size = 131072;
  req.tp_block_nr   = 64;
  req.tp_frame_size = 8192;
  req.tp_frame_nr   = req.tp_block_size / req.tp_frame_size * req.tp_block_nr;

  if (setsockopt(sk, SOL_PACKET, PACKET_RX_RING, &req, sizeof req) < 0) {
    perror("ring buffer setsockopt");
    exit(1);
  }

  if ((ring_buffer = mmap(0, req.tp_block_size * req.tp_block_nr, PROT_READ|PROT_WRITE, MAP_SHARED, sk, 0)) == MAP_FAILED) {
    perror("ring buffer mmap");
    exit(1);
  }
#endif

  return sk;
}


int create_fd(char *arg, int is_output, enum proto *proto)
{
  if (strncmp(arg, "udp:", 4) == 0 || strncmp(arg, "UDP:", 4) == 0) {
    *proto = UDP;
    arg += 4;
  } else if (strncmp(arg, "tcp:", 4) == 0 || strncmp(arg, "TCP:", 4) == 0) {
    *proto = TCP;
    arg += 4;
  } else if (strncmp(arg, "file:", 5) == 0) {
    *proto = File;
    arg += 5;
  } else if (strncmp(arg, "eth:", 4) == 0) {
    *proto = Eth;
    arg += 4;
  } else if (strncmp(arg, "stdin:", 6) == 0) {
    *proto = StdIn;
    arg = "stdin";
  } else if (strncmp(arg, "stdout:", 7) == 0) {
    *proto = StdOut;
    arg = "stdout";
  } else if (strncmp(arg, "stderr:", 7) == 0) {
    *proto = StdErr;
    arg = "stderr";
  } else if (strchr(arg, ':') != 0) {
    *proto = UDP;
  } else if (strcmp(arg, "-") == 0) {
    *proto = is_output ? StdOut : StdIn;
    arg = is_output ? "stdout" : "stdin";
  } else {
    *proto = File;
  }

  if (is_output)
    destination = arg;
  else
    source	= arg;

  switch (*proto) {
    case UDP	:
    case TCP	: return create_IP_socket(arg, is_output, *proto);

    case File	: return create_file(arg, is_output);

    case Eth	: return create_raw_eth_socket(arg, is_output);

    case StdIn	:
    case StdOut:
    case StdErr	: return create_stdio(is_output, *proto);
  }
}


void init(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: \"%s src-addr dest-addr\", where addr is [tcp:|udp:]ip-addr:port or [file:]filename\n", argv[0]);
    exit(1);
  }

  mmapFlatMemory();

  sk_in  = create_fd(argv[1], 0, &input_proto);
  sk_out = create_fd(argv[2], 1, &output_proto);

  setlinebuf(stdout);
}

#if 0
static unsigned offset = 0;

size_t get_packet()
{
  void *frame = ((char *) ring_buffer + offset * 8192);
  struct tpacket_hdr *hdr = frame;
  unsigned char *data = (char *) frame + hdr->tp_net;
  return hdr->tp_snaplen;
}

void packet_done()
{
  hdr->tp_status = TP_STATUS_KERNEL;

  if (++ offset == 1024)
    offset = 0;
}
#endif

int main(int argc, char **argv)
{
  time_t   previous_time = 0, current_time;
  unsigned i, nr_packets = 0, nr_bytes = 0;
  int      size;

  init(argc, argv);

#if 1
  if (input_proto == Eth) {
    unsigned offset = 0;
    while (1) {
      void *frame = ((char *) ring_buffer + offset * 8192);
      struct tpacket_hdr *hdr = frame;

#if 1
      if (hdr->tp_status == TP_STATUS_KERNEL) {
	struct pollfd pfd;

	pfd.fd = sk_in;
	pfd.revents = 0;
	pfd.events = POLLIN|POLLERR;

	if (poll(&pfd, 1, -1) < 0)
	  perror("poll");
      }
#else
      while (* (volatile long *) &hdr->tp_status == TP_STATUS_KERNEL)
	;
#endif

      assert(hdr->tp_status == TP_STATUS_USER); // FIXME

      //printf("status = %d %d %d %d %d %d %d\n", hdr->tp_status, hdr->tp_len, hdr->tp_snaplen, hdr->tp_mac, hdr->tp_net, hdr->tp_sec, hdr->tp_usec);
      unsigned char *mac = (char *) frame + hdr->tp_mac;
      //printf("mac = %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      unsigned char *data = (char *) frame + hdr->tp_net;
      //printf("data =");
      //for (i = 0; i < 48; i ++)
	//printf(" %hhx", ((unsigned char *) data)[i]);
      //printf("\n");

      if (write(sk_out, data, hdr->tp_snaplen) < hdr->tp_snaplen) {
	perror("write");
	sleep(1);
      } else {
	nr_bytes += hdr->tp_snaplen;
      }

      ++ nr_packets;

      if ((current_time = time(0)) != previous_time) {
	previous_time = current_time;

	fprintf(stderr, "ok: copied %u bytes (= %u packets) from %s to %s\n", nr_bytes, nr_packets, source, destination);
	nr_packets = nr_bytes = 0;
      }

      hdr->tp_status = TP_STATUS_KERNEL;

      if (++ offset == 1024)
	offset = 0;
    }
  }
#endif
  size_t max_size = output_proto == UDP ? 8960 : flatMemorySize;

  while ((size = read(sk_in, flatMemoryAddress, max_size)) != 0) {
    if (size < 0) {
      perror("read");
      sleep(1);
    } else if (write(sk_out, flatMemoryAddress, size) < size) {
      perror("write");
      sleep(1);
    } else {
      nr_bytes += size;
    }

    ++ nr_packets;

    if ((current_time = time(0)) != previous_time) {
      previous_time = current_time;

      if (input_proto == UDP || input_proto == Eth)
	fprintf(stderr, "copied %u bytes (= %u packets) from %s to %s\n", nr_bytes, nr_packets, source, destination);
      else
	fprintf(stderr, "copied %u bytes from %s to %s\n", nr_bytes, source, destination);
      nr_packets = nr_bytes = 0;
    }
  }

  return 0;
}
