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
    pthread_mutex_t lock;
} GameState;

GameState game;

void place_food(GameState *game) {
    pthread_mutex_lock(&game->lock);
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
    pthread_mutex_unlock(&game->lock);
}

void init_game(GameState *game) {
    pthread_mutex_init(&game->lock, NULL);
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

void update_game(GameState *game, char command) {
    pthread_mutex_lock(&game->lock);
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
            pthread_mutex_unlock(&game->lock);
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
    pthread_mutex_unlock(&game->lock);
}

void *render_thread(void *arg) {
    while (!game.game_over) {
        pthread_mutex_lock(&game.lock);
        system("clear");
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                if (x == game.snake.body[0].x && y == game.snake.body[0].y)
                    printf("H ");
                else if (x == game.food.x && y == game.food.y)
                    printf("F ");
                else
                    printf(". ");
            }
            printf("\n");
        }
        pthread_mutex_unlock(&game.lock);
        sleep(1);
    }
    return NULL;
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

    srand(time(NULL));
    init_game(&game);
    pthread_t tid;
    pthread_create(&tid, NULL, render_thread, NULL);

    // Assume server network handling here (omitted for simplicity)
    // This would handle client connections and command processing

    pthread_join(tid, NULL);  // Ensure that the rendering thread finishes on game over
    free(game.snake.body);
    pthread_mutex_destroy(&game.lock);
    return 0;
}
