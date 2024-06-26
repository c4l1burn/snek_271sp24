

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

#define SNAK '@'
#define SNEK 'O' // snake head
#define SNOD 'X' // for "snake bod"

#define REDO 'r'
#define QUIT 'q'

#define FORE 'w'
#define BACK 's'
#define LEFT 'a'
#define RITE 'd'

// shorter names for addresses and casts
typedef struct sockaddr_in6 *add6;
typedef struct sockaddr *add4;

// Sockbuff: dumb trick to get more data into a pthread
struct sockbuff {
    int sock;
    char * buff;
} ;



// function for a pthread to run to constantly shunt command line input into a buffer
void * const_send(void * arg)
// the purpose of this function is to read from a buffer and write to a socket. these two things will be contained in a struct.
// cloop() is whats responsible for actually putting data in that buffer
{
    struct sockbuff * sb = (struct sockbuff * )arg;
    //char * init;
    char init = 'w';
    // okaay so how do you read from an input buffer. how does any of that work.
    write(sb->sock, &init, strlen(&init)) ;
    while (1)
    {
        sleep(1);
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


 /*
  * BEGIN CLIENT-SIDE GAMEPLAY STUFF
  */

// Point: represents points on the game grid for various things like food, new snake parts and such
// this will be used for indexing into the grid
typedef struct {
    int x, y;
} Point;

// Snake: represents the PC's avatar and its attributes
typedef struct {
    Point * body;  // an array of points which make up the body of the snake
    Point snend;   // "snend" for "snake end"
    size_t length; // integer which defines the length of the snake
} Snake;

// GameState: Holds a snake, one location for food, and gameover faux-boolean
typedef struct {
    Snake snake;
    Point food;
    int game_over;
} GameState;

// updates the food value of the given game
// called at initialization and whenever previous food is collected
void place_food(GameState *game)
{
    int placed = 0; // initialized a "has food been placed" boolean to "No"
    while (!placed)
    {
        game->food.x = rand() % WIDE;
        game->food.y = rand() % HIGH;
        // update food x and y variables -- places some food, duh.
        placed = 1;
        for (size_t i = 0; i < game->snake.length; i++)
        // run length + 1 times
        {
            if (game->food.x == game->snake.body[i].x && game->food.y == game->snake.body[i].y)
            // if the food got placed inside the snake, set placed to "no" and try again
            {
                placed = 0;
                break;
            }
        }
    }
}


// Initializes the game; places the snake bod, puts down the first food, that kinda thing
void init_game(GameState *game)
{
    game->snake.body = malloc(HIGH * WIDE * sizeof(Point));
    // allocates memory for the position of the snakes body
    if (!game->snake.body)
    {
        perror("Failed to allocate memory for snake body");
        exit(EXIT_FAILURE);
    }
    game->snake.length = 4;           // init length to one
    game->snake.body[0].x = WIDE / 2;
    game->snake.body[0].y = HIGH / 2; // initializes the x and y to be values associated with WIDE and HIGH respectively
    game->game_over = 0;              // sets gameover to "No"
    place_food(game);
}

// takes a game to update and a command to update it with
void update_gamestate(GameState *game, char command)
//  command will be read from the buffer that I set up
{
    Point next = game->snake.body[0];
    // next point = current position of the head
    switch (command)                    // updates next point according to input
    {
        case 'w': next.y++; break;
        case 's': next.y--; break;
        case 'a': next.x--; break;
        case 'd': next.x++; break;
        case 'q': printf("\nGoodbye!\n"); exit(EXIT_SUCCESS);
    }


    //just a variables for me to not have to look at all the struct notation. as a treat.
    int snakelen = game->snake.length;

    if (next.x == game->food.x && next.y == game->food.y)
    {


        for (int i = snakelen; i > 0; i--)
        {
            if (i == snakelen)
            {
                game->snake.length ++;
                game->snake.body[i+1] = game->snake.body[i];
            }
            game->snake.body[i] = game->snake.body[i - 1];
        }
        game->snake.body[0] = next;
        place_food(game);
    } else {
        // if the player did anything besides collect food this frame
        // iterate over the length of snake, shifting all their positions up by one
        for (int i = snakelen; i > 0; i--)
        {
            if (i == snakelen) { game->snake.snend = game->snake.body[i]; } // updates the point "snend" (snake end) to be the trailing space behind the snake
            game->snake.body[i] = game->snake.body[i - 1];
        }

        game->snake.body[0] = next;
        // update head to next
    }

    if (next.x < 0 || next.y < 0 || next.x >= WIDE || next.y >= HIGH ||
        (game->snake.length > 1 && next.x == game->snake.body[1].x && next.y == game->snake.body[1].y))
        // if (x or y are less than 0 or outside the grid) or (the snake ran into itself): game over
        // god that was fucking incomprehensible.
        // also it doesnt work but genuinely i dont care at this point
    {
        game->game_over = 1;
    }
}

char ** create_grid()
{
    char ** grid = (char **)malloc(sizeof(char *) * WIDE); // sort of the grid x
    for (int i = 0; i < WIDE; i++)
    {
        grid[i] = (char *)malloc(sizeof(char) * HIGH);
        for (int j = 0; j < HIGH; j++)
        {
            grid[i][j] = '.';
        }
    }
    return grid;
}

void draw_snake(GameState *game, char ** grid)
{
    int snakelen = game->snake.length;
    Point * snegments = game->snake.body;

    int snend_x = game->snake.snend.x;
    int snend_y = game->snake.snend.y;  // im sure this is all self explanatory
    grid[snend_x][snend_y] = '.';

    for (int i = 0; i<=snakelen; i++)
    {
        int snex = snegments[i].x;
        int sney = snegments[i].y;      // yeah this rules
        grid[snex][sney] = SNEK;
        if (i>0)
        {
            grid[snex][sney] = SNOD;
        }
    }



}

// INPUT: a preconfigured connection
// OUTPUT: a rendered game
int sloop(int conx)
{
    char buff[SIZE] ; // the buffer which is constantly getting written to by cloop()
    memset(buff, 'd', SIZE) ;

    GameState game;
    init_game(&game);
    char ** grid = create_grid();
    while (1)
    {
        // Reads what was put in the buffer, uses that to update the gamestate
        read(conx, buff, SIZE) ;
        update_gamestate(&game,buff[0]);

        // convenience vars
        int headx = game.snake.body[0].x;
        int heady = game.snake.body[0].y;

        int foodx = game.food.x;
        int foody = game.food.y;
        draw_snake(&game, grid);
        grid[foodx][foody] = SNAK;


        for (int y = HIGH - 1 ; y >= 0; y--)
        {
            for (int x = 0; x < WIDE; x++)
            {
                printf("%c", grid[x][y]);
            }
            printf("\n");
        }
        printf("\n");
        // debug prints
        //printf("Food: (%d, %d) Head: (%d, %d) Length: (%d)\n",foodx, foody, headx, heady, game.snake.length);
        //printf("\n");
    }    return 0;
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



