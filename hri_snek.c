#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define DEFAULT_PORT 0xC271
#define BUFFER_SIZE 1024
#define GRID_SIZE 20

typedef struct {
    int x, y;
} Point;
// defines a type called point -- points have two ints in them, x and y
// i think this represents literall points on the grid. interesting

typedef struct {
    Point *body;
    size_t length;
} Snake;
// struct for the attributes of the snake.

typedef struct {
    Snake snake;
    Point food;
    int game_over;
} GameState;

void place_food(GameState *game)
{
    int placed = 0; // initialized a "has food been placed" boolean to "No"
    while (!placed)
    {
        game->food.x = rand() % GRID_SIZE;
        game->food.y = rand() % GRID_SIZE;
        // place update food x and y variables -- places some food, duh.
        placed = 1;
        for (size_t i = 0; i < game->snake.length; i++)
        {
            // run length + 1 times
            if (game->food.x == game->snake.body[i].x && game->food.y == game->snake.body[i].y) {
                // if the food got placed inside the snake, set placed to "no" and try again
                placed = 0;
                break;

            }
        }
    }
}

void init_game(GameState *game) {
    game->snake.body = malloc(GRID_SIZE * GRID_SIZE * sizeof(Point));
    // allocates memory for the position of the snakes body
    if (!game->snake.body) {
        perror("Failed to allocate memory for snake body");
        exit(EXIT_FAILURE);
    }
    game->snake.length = 1;                // init length to one. not sure if chatgpt indexed this
    game->snake.body[0].x = GRID_SIZE / 2;
    game->snake.body[0].y = GRID_SIZE / 2; // places snake body[0] in the middle of the board
    game->game_over = 0;                   // sets gameover to "No"
    place_food(game);
}
//gcc -o fek_snek fek_snek.c -lpthread

void update_game(GameState *game, char command)
// takes a game to update and a command to update it with
// the command will be red from the buffer that I set up
{
    Point next = game->snake.body[0];
        // initializes a new point and sets it equal to the first element of the body array
    switch (command)
    {
        case 'w': next.y--; break;
        case 's': next.y++; break;
        case 'a': next.x--; break;
        case 'd': next.x++; break;
    }

    if (next.x == game->food.x && next.y == game->food.y)
    {
        // if new point is inside food
        Point* new_body = realloc(game->snake.body, (game->snake.length + 1) * sizeof(Point));
            // I dont think this makes sense -- i dont know why the size of the block used to store the body element would grow with the size of the body. I think this should actually be a little backwards.
        if (!new_body)
        {
            perror("Failed to reallocate memory for snake body");
            return; // Continue the game without growing the snake
        }
        game->snake.body = new_body; // chatgpt seems confused as to whether body has indexes or not
        game->snake.body[game->snake.length++] = next;
            // snake body element number (length plus one) is now equal to this new point we're rockin with
        place_food(game);
    } else {
        for (size_t i = game->snake.length - 1; i > 0; i--)
            // iterate over the length of snake, shifting all their positions up by one
        {
            game->snake.body[i] = game->snake.body[i - 1];
        }
        game->snake.body[0] = next;
        // update head to next
    }

    if (next.x < 0 || next.y < 0 || next.x >= GRID_SIZE || next.y >= GRID_SIZE ||
        (game->snake.length > 1 && next.x == game->snake.body[1].x && next.y == game->snake.body[1].y))
        // if (x or y are less than 0 or outside the grid) or (the snake ran into itself): game over
        // god that was fucking incomprehensible.
    {
        game->game_over = 1;
    }
}

// honestly i think the above code can probably be transplanted wholesale -- it all looks like it should work fine.


// ...bad. this is assembling the game state (such as it is) before sending it ... to the client?? absolutely not.
void send_game_state(int client_sock, GameState *game) {
    // this though, this is a mess
    char buffer[BUFFER_SIZE];
    int n = snprintf(buffer, sizeof(buffer), "Food: (%d, %d) Head: (%d, %d)\n",
                     game->food.x, game->food.y, game->snake.body[0].x, game->snake.body[0].y);
    if (send(client_sock, buffer, n, 0) == -1) {
        perror("Failed to send game state");
    }
    if (game->game_over) {
        send(client_sock, "Game Over!\n", 11, 0);
    }
}

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc);

    GameState game;
    init_game(&game);
    char buffer[BUFFER_SIZE];

    while (!game.game_over) {
        ssize_t read_size = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            update_game(&game, buffer[0]);
            send_game_state(sock, &game);
        } else if (read_size == 0) {
            puts("Client disconnected");
            break;
        } else {
            perror("Receive failed");
            break;
        }
    }

    close(sock);
    free(game.snake.body);
    return NULL;
}

void run_server(int port) {
    int server_sock, *new_sock;
    struct sockaddr_in server, client;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Could not create socket");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(1);
    }

    listen(server_sock, 3);
    printf("Server listening on port %d...\n", port);

    while (1) {
        socklen_t c = sizeof(struct sockaddr_in);
        new_sock = malloc(sizeof(int));
        if (!new_sock) {
            perror("Memory allocation failed");
            continue;
        }
        *new_sock = accept(server_sock, (struct sockaddr *)&client, &c);
        if (*new_sock < 0) {
            perror("Accept failed");
            free(new_sock);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler, (void*) new_sock) != 0) {
            perror("Could not create thread");
            free(new_sock);
        } else {
            pthread_detach(thread_id);
        }
    }

    close(server_sock);
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port number. Using default %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }

    srand(time(NULL)); // Seed the random number generator once
    run_server(port);
    return 0;
}
