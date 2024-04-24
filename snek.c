#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define DEFAULT_PORT 5000
#define BUFFER_SIZE 1024
#define GRID_SIZE 20

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point *body;
    size_t length;
} Snake;

typedef struct {
    Snake snake;
    Point food;
    int game_over;
} GameState;

void place_food(GameState *game) {
    int placed = 0;
    while (!placed) {
        game->food.x = rand() % GRID_SIZE;
        game->food.y = rand() % GRID_SIZE;

        placed = 1;
        for (size_t i = 0; i < game->snake.length; i++) {
            if (game->food.x == game->snake.body[i].x && game->food.y == game->snake.body[i].y) {
                placed = 0;
                break;
            }
        }
    }
}

void init_game(GameState *game) {
    game->snake.body = malloc(GRID_SIZE * GRID_SIZE * sizeof(Point));
    if (!game->snake.body) {
        perror("Failed to allocate memory for snake body");
        exit(EXIT_FAILURE);
    }
    game->snake.length = 1;
    game->snake.body[0].x = GRID_SIZE / 2;
    game->snake.body[0].y = GRID_SIZE / 2;
    game->game_over = 0;
    place_food(game);
}
//gcc -o fek_snek fek_snek.c -lpthread

void update_game(GameState *game, char command) {
    Point next = game->snake.body[0];
    switch (command) {
        case 'w': next.y--; break;
        case 's': next.y++; break;
        case 'a': next.x--; break;
        case 'd': next.x++; break;
    }

    if (next.x == game->food.x && next.y == game->food.y) {
        Point* new_body = realloc(game->snake.body, (game->snake.length + 1) * sizeof(Point));
        if (!new_body) {
            perror("Failed to reallocate memory for snake body");
            return; // Continue the game without growing the snake
        }
        game->snake.body = new_body;
        game->snake.body[game->snake.length++] = next;
        place_food(game);
    } else {
        for (size_t i = game->snake.length - 1; i > 0; i--) {
            game->snake.body[i] = game->snake.body[i - 1];
        }
        game->snake.body[0] = next;
    }

    if (next.x < 0 || next.y < 0 || next.x >= GRID_SIZE || next.y >= GRID_SIZE ||
        (game->snake.length > 1 && next.x == game->snake.body[1].x && next.y == game->snake.body[1].y)) {
        game->game_over = 1;
    }
}

void send_game_state(int client_sock, GameState *game) {
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
