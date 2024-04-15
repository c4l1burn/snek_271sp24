#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
//gcc -o fek_snek /Users/owner/Desktop/fek_snek.c
//./fek_snek -s
//./fek_snek -c

#define PORT 5000          // Port number for the server
#define BUFFER_SIZE 1024   // Buffer size for receiving data
#define GRID_SIZE 20       // Size of the game grid

// Structure for representing a point on the grid
typedef struct {
    int x, y;
} Point;

// Structure for representing a snake
typedef struct {
    Point *body;          // Dynamic array for snake's body
    size_t length;        // Current length of the snake
    int direction;        // Current movement direction of the snake
} Snake;

// Game state structure to hold current game information
typedef struct {
    Snake snake;          // Snake in the game
    Point food;           // Food's position in the game
    int game_over;        // Flag for game over state
} GameState;

// Function to randomly place food on the grid without overlapping the snake
void place_food(GameState *game) {
    int placed = 0;
    while (!placed) {
        // Generate random position for food
        game->food.x = rand() % GRID_SIZE;
        game->food.y = rand() % GRID_SIZE;

        // Check if the food overlaps with the snake's body
        placed = 1;
        for (size_t i = 0; i < game->snake.length; i++) {
            if (game->food.x == game->snake.body[i].x && game->food.y == game->snake.body[i].y) {
                placed = 0;
                break;
            }
        }
    }
}

// Function to initialize the game state with a snake at the center and random food placement
void init_game(GameState *game) {
    game->snake.body = malloc(sizeof(Point) * GRID_SIZE * GRID_SIZE); // Allocate enough space for snake's maximum length
    game->snake.length = 1; // Start with a snake of length 1
    game->snake.body[0].x = GRID_SIZE / 2; // Place the snake in the center of the grid
    game->snake.body[0].y = GRID_SIZE / 2;
    game->snake.direction = 'R'; // Initial direction is right
    game->game_over = 0; // Game is not over at start
    place_food(game); // Place the initial food
}

// Update the game state based on the received command
void update_game(GameState *game, char command) {
    // Determine new position for snake's head based on the command
    Point next = game->snake.body[0];
    switch (command) {
        case 'w': next.y--; break; // Move up
        case 's': next.y++; break; // Move down
        case 'a': next.x--; break; // Move left
        case 'd': next.x++; break; // Move right
    }

    // Check if snake eats food
    if (next.x == game->food.x && next.y == game->food.y) {
        game->snake.length++; // Increase snake length
        place_food(game); // Place new food
    } else {
        // Move the snake's body
        for (size_t i = game->snake.length - 1; i > 0; i--) {
            game->snake.body[i] = game->snake.body[i - 1];
        }
    }
    game->snake.body[0] = next; // Update the head position

    // Check for collisions with the wall or itself
    if (next.x < 0 || next.y < 0 || next.x >= GRID_SIZE || next.y >= GRID_SIZE) {
        game->game_over = 1; // Wall collision
    }

    for (size_t i = 1; i < game->snake.length; i++) {
        if (game->snake.body[i].x == next.x && game->snake.body[i].y == next.y) {
            game->game_over = 1; // Self-collision
            break;
        }
    }
}

// Send the current game state to the client
void send_game_state(int client_sock, GameState *game) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Food: (%d, %d) Head: (%d, %d)\n", 
             game->food.x, game->food.y, game->snake.body[0].x, game->snake.body[0].y);
    send(client_sock, buffer, strlen(buffer), 0); // Send game state
    if (game->game_over) {
        send(client_sock, "Game Over!\n", 11, 0); // Notify client of game over
    }
}

// Function to run the server logic
void run_server() {
    int server_sock, client_sock;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    // Set up the server socket
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

    listen(server_sock, 1); // Listen for one connection
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
}

// Function to run the client logic
void run_client() {
    int sock;
    struct sockaddr_in6 server_addr;
    char buffer[BUFFER_SIZE];

    // Create the client socket
    sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &server_addr.sin6_addr);
    server_addr.sin6_port = htons(PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at ::1:%d\n", PORT);

    // Main loop for client input
    char input;
    printf("Enter 'w', 'a', 's', 'd' to move the snake. Press 'q' to quit.\n");

    while (1) {
        scanf(" %c", &input); // Read one character input from the user
        if (input == 'w' || input == 'a' || input == 's' || input == 'd') {
            send(sock, &input, sizeof(input), 0); // Send valid command to the server
        } else if (input == 'q') {
            printf("Exiting.\n");
            break;
        } else {
            printf("Invalid input. Use only 'w', 'a', 's', 'd' for movement, 'q' to quit.\n");
        }

        ssize_t recv_size = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (recv_size > 0) {
            buffer[recv_size] = '\0';
            printf("%s", buffer); // Display server response
        }
    }

    close(sock);
}

// Main function to decide whether to run as a server or client based on command-line arguments
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s -s for server, -c for client\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-s") == 0) {
        run_server();
    } else if (strcmp(argv[1], "-c") == 0) {
        run_client();
    } else {
        fprintf(stderr, "Invalid option: %s. Use -s for server, -c for client.\n", argv[1]);
        return EXIT_FAILURE;
    }

    return 0;
}
