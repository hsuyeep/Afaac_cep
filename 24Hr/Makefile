#CFLAGS =	-O2 -std=c99
CFLAGS =	-g -std=c99 -Wall

all::		udp-copy generator analyzer

udp-paste:	udp-paste.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread

udp-copy:	udp-copy.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread

udp-copy-8bit:	udp-copy-8bit.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread

udp-copy-8bit-1sb:udp-copy-8bit-1sb.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread

udp-copy-8bit-dip:udp-copy-8bit-dip.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread

generator:	generator.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread -lm

analyzer:	analyzer.c common.c
		gcc $(CFLAGS) $^ -o $@ -lpthread

readtseries:	readtseries.c
		gcc $(CFLAGS) $^ -o $@ 
