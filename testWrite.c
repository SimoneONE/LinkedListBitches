#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "linked_list.h"

/** 
 * Set a file descriptor to blocking or non-blocking mode
 * using fcntl library function.
 *
 * @param fd The file descriptor
 * @param blocking 0:non-blocking mode, 1:blocking mode
 *
 * @return 1:success, 0:failure.
 **/

int main(int argc, char *argv[])
{
    char* string;
    int numBytes = 0;

    if( argc == 3 ) {
        
        numBytes = atoi(argv[1]);
        
        switch(numBytes){
            case 1:
                string = "1";
                break;
            case 2:
                string = "12";
                break;
            case 3:
                string = "123";
                break;
            case 4:
                string = "1234";
                break;
            case 5:
                string = "12345";
                break;
            case 6:
                string = "123456";
                break;
            case 7:
                string = "1234567";
                break;
            case 8:
                string = "12345678";
                break;
            case 9:
                string = "123456789";
                break;
            case 10:
                string = "1234567890";
                break;
            case 16:
                string = "aaaaaaaaaaaaaaaa";
                break;
            case 20:
                string = "aaaaaaaaaaaaaaaaaaaa";
                break;
            default:
                printf("Unexpected value. Expected values are 1,2,3,4,5,6,7,8,9,10,16 and 20.\n");
                return 0;
                break;
        }
    }
    else {
        printf("Usage: writeTest n m, where:\n");
        printf("- n is the number of bytes you want to write.\n");
        printf("- m is the mode: \"nb\" for non blocking, \"b\" for blocking.\n");
        printf("Exiting.\n");
        return 0;
    }
    
    int filedesc = open("/dev/linked_list0", O_RDWR);
 
    if (filedesc < 0) {
		printf("There was an error opening linked_list0\n");
        return -1;
    }

	
	int wrote = 0;
	if(strcmp(argv[2], "nb") == 0) {
		ioctl(filedesc,LL_SET_NONBLOCKING,0);
		printf("Testing write and stream, non-blocking read...\n");
	}
	else
		printf("Testing write and stream, blocking read...\n");
		
    if ((wrote = write(filedesc, string, numBytes)) < 0) {
        printf("There was an error writing to linked_list0; wrote: %d\n", wrote);
        return -1;
    }
    
	close(filedesc);
 
    return 0;
}
