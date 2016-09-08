/*
* The linked_list header
*/
#include <linux/ioctl.h>

#define EXPORT_SYMTAB

#define DEVICE_NAME "linked_list"
#define DEVICE_MAX_NUMBER 256
#define MAX_STREAM_SIZE 64
#define PACKET_SIZE 8

/* IOCTL related macros */
#define LL_MAJOR 					250
#define LL_SET_PACKET_MODE 			_IO(DHARMA_MAJOR, 0)
#define LL_SET_STREAM_MODE 			_IO(DHARMA_MAJOR, 1)
#define LL_SET_BLOCKING 			_IO(DHARMA_MAJOR, 2)
#define LL_SET_NONBLOCKING 			_IO(DHARMA_MAJOR, 3)
#define LL_GET_MAX_SIZE				_IO(DHARMA_MAJOR, 4)
#define LL_SET_MAX_SIZE				_IO(DHARMA_MAJOR, 5)
#define LL_GET_PACK_SIZE			_IO(DHARMA_MAJOR, 6)
#define LL_SET_PACK_SIZE			_IO(DHARMA_MAJOR, 7)

/* Buffer to store data */
typedef struct Packet{
	char *buffer;
	int bufferSize;
	int readPos;
	struct Packet *next;
} Packet;
