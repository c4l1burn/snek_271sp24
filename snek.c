
// Including a bunch of the given code here. maybe later ill figure out that this was stupid but we gotta start somewhere

#include <sys/socket.h> // for socket()
#include <arpa/inet.h>  // for add6
#include <stdio.h>      // for printf()
#include <unistd.h>     // for read()
#include <stdlib.h>     // for malloc()
#include <string.h>     // for strlen()
#include <time.h>       // for time()
#include <fcntl.h>      // for fcntl()

// sockets
#define PORT 0xC271     // get it? CS 271?
#define DOMAIN AF_INET6 // ipv6
#define LOOPBACK "::1"  // "loopback" ipv6 - network locally

// debug
#define VERBOSE 1
#define BUFF 16

// booleans
#define TRUE 1
#define FALS 0
typedef int bool;

// gameplay
#define HIGH 23
#define WIDE 80

#define SNAK '&'
#define SNEK 'O' // what.

#define REDO 'r'
#define QUIT 'q'

#define FORE 'w'
#define BACK 's'
#define LEFT 'a'
#define RITE 'd'

// shorter names for addresses and casts
typedef struct sockaddr_in6 *add6;
typedef struct sockaddr *add4;


//To test that your main is ready to support all critical tasks, just configure it to call simple wrapper functions with print statements as stand-ins for later networking and gameplay tasks

// must be able to conditionally call different functions based on command line arguments


//provided main:

// So this main is using command line arguments. how do those work? extremely unclear. you
int main(int argc, char const *argv[])
{
    printf("%s, expects (1) arg, %d provided", argv[0], argc-1);
    if (argc == 2)
    {
        printf(": \"%s\".\n", argv[1]);
    } else {
        printf(".\n");
        return 1;
    }

    if (argv[1][1] == 's')
    {
        printf("Starting server...\n");
    }

    if (argv[1][1] == 'c')
    {
        printf("Starting client...\n");
    }

    if (argv[1][1] == 'h')
    {
        printf("HELP: Usage:  -s for server, -c for client\n");
    }

    return 0;
}
