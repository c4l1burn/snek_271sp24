#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 5000
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024
#define GRID_SIZE 20

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point *body;
    size_t length;
    int direction;
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
    game->snake.body = malloc(sizeof(Point) * GRID_SIZE * GRID_SIZE); // maximum size possible
    game->snake.length = 1;
    game->snake.body[0].x = GRID_SIZE / 2;
    game->snake.body[0].y = GRID_SIZE / 2;
    game->snake.direction = 'R';
    game->game_over = 0;
    place_food(game);
}

void update_game(GameState *game, char command) {
    Point next = game->snake.body[0];
    switch (command) {
        case 'w': next.y--; break;
        case 's': next.y++; break;
        case 'a': next.x--; break;
        case 'd': next.x++; break;
    }

    // Check if the snake eats food
    if (next.x == game->food.x && next.y == game->food.y) {
        game->snake.length++;
        place_food(game); // place new food
    } else {
        // Move the body
        for (size_t i = game->snake.length - 1; i > 0; i--) {
            game->snake.body[i] = game->snake.body[i - 1];
        }
    }

    game->snake.body[0] = next;

    // Check for collisions with walls or itself
    if (next.x < 0 || next.y < 0 || next.x >= GRID_SIZE || next.y >= GRID_SIZE) {
        game->game_over = 1;
    }

    for (size_t i = 1; i < game->snake.length; i++) {
        if (game->snake.body[i].x == next.x && game->snake.body[i].y == next.y) {
            game->game_over = 1;
            break;
        }
    }
}

void send_game_state(int client_sock, GameState *game) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Food: (%d, %d) Head: (%d, %d)\n", 
             game->food.x, game->food.y, game->snake.body[0].x, game->snake.body[0].y);
    send(client_sock, buffer, strlen(buffer), 0);
    if (game->game_over) {
        send(client_sock, "Game Over!\n", 11, 0);
    }
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    listen(server_sock, MAX_CLIENTS);
    printf("Server listening on port %d\n", PORT);

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size)) != -1) {
        GameState game;
        init_game(&game);
        char buffer[BUFFER_SIZE];

        while (!game.game_over) {
            ssize_t read_size = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
            if (read_size > 0) {
                buffer[read_size] = '\0';
                update_game(&game, buffer[0]);
                send_game_state(client_sock, &game);
            }
        }

        printf("Client disconnected. Game over.\n");
        close(client_sock);
        free(game.snake.body);
    }

    close(server_sock);
    return 0;
}