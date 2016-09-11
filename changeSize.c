#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "linked_list.h"
#include <stdlib.h>


/** 
 * Set a file descriptor to blocking or non-blocking mode
 * using fcntl library function.
 *
 * @param fd The file descriptor
 * @param blocking 0:non-blocking mode, 1:blocking mode
 *
 * @return 1:success, 0:failure.
 **/

int main( int argc, char *argv[] )
{
    int numBytes = 0;
    
    int filedesc = open("/dev/linked_list0", O_RDWR);
    
    if (filedesc < 0) {
		printf("There was an error opening linked_list0\n");
        return -1;
    }
    
    if( argc == 3 || argc == 4 ) {
		if(strcmp(argv[1], "b") == 0) {
			// Get the new buffer size from cli.
			numBytes = atoi(argv[1]);
			if( ioctl(filedesc, LL_SET_MAX_SIZE, &numBytes) == 0){
				printf("Set buffer size = %d.\n", numBytes);
				if( ioctl(filedesc, LL_GET_MAX_SIZE, &numBytes) == 0)
					printf("New buffer size is %d.\n", numBytes);
			}
		}
		else {
			if( argc == 4 ) {
				if( strcmp(argv[3] ,"m") == 0 ) {
					// Get the new buffer size from cli.
					numBytes = atoi(argv[1]);
					if( ioctl(filedesc, LL_SET_PACK_MIN_SIZE, &numBytes) == 0){
						printf("Set min packet size = %d.\n", numBytes);
						if( ioctl(filedesc, LL_GET_PACK_MIN_SIZE, &numBytes) == 0)
							printf("New min packet size is %d.\n", numBytes);
					}
				}
				else {
					numBytes = atoi(argv[1]);
					if( ioctl(filedesc, LL_SET_PACK_MAX_SIZE, &numBytes) == 0){
						printf("Set max packet size = %d.\n", numBytes);
						if( ioctl(filedesc, LL_GET_PACK_MAX_SIZE, &numBytes) == 0)
							printf("New max packet size is %d.\n", numBytes);
					}
				}
			}
			else {
				numBytes = atoi(argv[1]);
				if( ioctl(filedesc, LL_SET_PACK_MAX_SIZE, &numBytes) == 0){
					printf("Set max packet size = %d.\n", numBytes);
					if( ioctl(filedesc, LL_GET_PACK_MAX_SIZE, &numBytes) == 0)
						printf("New max packet size is %d.\n", numBytes);
				}
			}
		}
    }
    else {
        printf("Usage: changeSize k [m|M] n, where:\n");
        printf("- k is the kind: \"b\" for buffer, \"p\" for packet.\n");
        printf("- if packet, \"m\" for minimum, \"M\" maximum .\n");
        printf("- n is the new size of the buffer (in bytes).\n");
        printf("Exiting.\n");
        return 0;
    }
    
    return 0;
}
