

// DECOMPOSITION:

// 1. Get snek.c to spin up a server and client that can talk to each other to some extent
// nevermind that actually seems harder. new plan:
// 2. Using the gcc multiple file complier thing (also: make a make for this) have code that calls functions from those other .c files
/*
    Things will basically work like this:
	snek.c is a kind of hermaphroditous program that can either be a server or a client depending on which functions are called.
	A client instance of snek.c (hereafter just "the client" or "the server") sends an input to a server process every (interval). The clock is also client side.
	The client remembers the last input given, and sends that to the server every (interval) until a new direction is given
	(the client also needs logic for how you can turn)

*/

// Including a bunch of the given code here. maybe later ill figure out that this was stupid but we gotta start somewhere

#include <sys/socket.h> // for socket()
#include <arpa/inet.h>  // for add6
#include <stdio.h>      // for printf()
#include <unistd.h>     // for read()
#include <stdlib.h>     // for malloc()
#include <string.h>     // for strlen()
#include <time.h>       // for time()
#include <fcntl.h>      // for fcntl()
#include <pthread.h>    // for pthread_create()

// sockets
#define PORT 0xC271     // get it? CS 271?
#define DOMAIN AF_INET6 // ipv6
#define LOOPBACK "::1"  // "loopback" ipv6 - network locally

// debug
#define VERBOSE 1
#define BUFF 16
#define SIZE   1024

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

struct sockbuff {
    int sock;
    char * buff;
} ;

//typedef struct sockbuff sockbuff;


//To test that your main is ready to support all critical tasks, just configure it to call simple wrapper functions with print statements as stand-ins for later networking and gameplay tasks

// must be able to conditionally call different functions based on command line arguments


//provided main:



void get_sock(int *sock, add6 address, bool is_server)
{
 // Ill probably also want this. what does it do? a mystery!
 // I mean okay I guess i can assume it gets a socket.
}

// note that i probably do actually want cloop and sloop to have some arguments, like passing the socket or w/ever
// client loop. Takes input, processes it into a game, sends that game to the client

void * const_send(void * arg)
// the purpose of this function is to read from a buffer and write to a socket. these two things will be contained in a struct.
// cloop() is whats responsible for actually putting data in that buffer
{
    struct sockbuff * sb = (struct sockbuff * )arg;
    // okaay so how do you read from an input buffer. how does any of that work.
    while (1)
    {
        sleep(1);
        char * buff_contents = sb->buff;
        //printf("Contents of buff: %s \n", buff_contents);
        write(sb->sock, sb->buff, strlen(sb->buff)) ;

        // HOLY BABY JESUS IT FUCKING WORKS
    }
}
int cloop(int sock)
{
    /*
     *  Pthread Cloop logic
     * Spin off a pthread that sets up a socket and sends whatevers in some buffer to it once every second
     * then spin off another pthread that continouously reads input to that same buffer using pointers
     *
     * the cloop will be the thing that does the constant reading?
     */

    // pthread_create( &tid, NULL, &func, (void *) &val ) ;
    // the function that the pthread runs and its arguments are the third and fourth arguments of pthread_create
    // those arguments have to be presented as a void star, for some reason


    // set up constant read from buffer in a pthread

    char * inpt_buff = malloc(sizeof(char));

    struct sockbuff sb;
    struct sockbuff * sb_ptr = malloc(sizeof(sb)) ;
    sb_ptr->buff = inpt_buff;
    sb_ptr->sock = sock;

    pthread_t tid_const_send ;
    pthread_create(&tid_const_send, NULL, &const_send, (void *) sb_ptr);

    /* ------------------------------------- */



    while (1)   // may not even need a thread here? could just do it here.
    {
        scanf("%s", sb_ptr->buff);
        //dude i think this might be enough. thats wild.

    }

    return 0;
}
int client()
{
    /*
     The role of the client() function is primarily to configure and connect a socket
     that is passed as the sole argument to the client gameplay loop function cloop().

    */

    /*
     * The following is me reteaching myself the slides to get my head around what I actually need to do
     * socket(): int socket(int domain, int type, int protocol);
     * the domain is always gonna be AF_INET6, the type is gonna be SOCK_STREAM* (thats an asterix, not a pointer), and the protocol is gonna be 0 bcs theres only one AF_INET6 protocol and its the default
     * so we have: int socket(AF_INET6, SOCK_STREAM, 0);
     * but how do we use that with, like, addresses and stuff?
     * We use a struct, specifically sockaddr
     * Heres how sockaddr_in6 is defined:
     *
     * struct sockaddr_in6 {
           sa_family_t     sin6_family;     / AF_INET6
           in_port_t       sin6_port;       / Port number
           struct in6_addr sin6_addr;       / IPv6 address
       };

        There was other stuff there but it wasnt relevant.

    */
    int sock = socket(DOMAIN, SOCK_STREAM, 0);
    struct sockaddr_in6 addr;
    addr.sin6_family = DOMAIN;
    addr.sin6_port = htons(PORT);
    inet_pton(DOMAIN, LOOPBACK, &addr.sin6_addr);
    // sets the client up to pass pointers (i think) to the loopback address

    // okay so lets do something that will perform 10 write to socket events
    //printf(sock);
    // connect(sock, &address, sizeof(address));
    // the fuckin (struct sockaddr *) thing is a cast and without it it throws a fit. whatever.
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
	{
		perror("Client - conect failed.") ;
		exit(-1) ;
	}

    cloop(sock);
    return 0;
}
int sloop(int conx)
{
    // okay so the point of sloop is to construct the gameplay basically
    char buff[SIZE] ;
    memset(buff, 0, SIZE) ;

    while (1)
    {
        read(conx, buff, SIZE) ;
        printf("%s\n", buff) ;
    }
    return 0;
}


int server()
{
    int sock = socket(DOMAIN, SOCK_STREAM, 0);
    struct sockaddr_in6 addr;
    addr.sin6_family = DOMAIN;
    addr.sin6_port = htons(PORT);
    addr.sin6_addr = in6addr_any;
    // sets the server up to recieve any address, including addresses originating from our own machine eg. loopback.

	socklen_t s = sizeof(struct sockaddr_in6) ;

    // this just stuffs 0 into the buffer
    // im *pretty* sure that these if statements are actually doing the things and just also putting some exit control in there
    if (bind(sock, (struct sockaddr *)&addr, s))
	{
		perror("Server - bindin failed.\n") ;
		exit(-1) ;
	}
	if (listen(sock, 1))
	{
		perror("Server - listen failed.\n") ;
		exit(-1) ;
	}
	int conx = accept(sock, (struct sockaddr *)&addr, &s) ;
    //In this case conx, an int, is what the server reads from (treats as a file)
	if (conx == -1)
	{
		perror("Server - accept failed.\n") ;
		exit(-1) ;
	}

	printf("Somehow nothing fucked up! Server connected (i think?)\n");

    sloop(conx);
    return 0;
}



int serv_start()
{
        printf("Starting server...\n");
        server();
        return 0;
}

int client_start()
{
        printf("Starting client...\n");
        client();

        return 0;
}

// argc is argument count. argv is "argument vector", whatever the fuck. argv is how you do the command line arguments basically
int main(int argc, char const *argv[])
{
    //int sock = socket(DOMAIN, SOCK_STREAM, 0);
    // creates a socket
    //struct sockaddr_in6 addr;
    // defines the structure (called addr) that lets us use addresses with that socket

    //addr.sin6_family = DOMAIN;
    // the family of address is IPv6
    //addr.sin6_port = htons(PORT);
    // the port of address is (host to network short -- the internet forces us to use big endian no matter what)(port -- in this case port 49777)

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
        serv_start();
    }

    if (argv[1][1] == 'c')
    {
        client_start();

    }

    if (argv[1][1] == 'h')
    {
        printf("HELP: Usage:  -s for server, -c for client\n");
    }
    return 0;
}



