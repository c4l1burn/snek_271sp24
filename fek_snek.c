#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

// Define constants
#define PORT 0xC271 // Hexadecimal for 'CS271'

// Function Declarations
void startServer();
void startClient();
void showHelp();

int main(int argc, char const *argv[]) {
    srand(time(NULL)); // Initialize random number generation

    printf("%s, expects (1) arg, %d provided", argv[0], argc-1);
    if (argc != 2) {
        printf(".\n");
        showHelp();
        return 1;
    } else {
        printf(": \"%s\".\n", argv[1]);
    }

    switch (argv[1][1]) {
        case 's':
            startServer();
            break;
        case 'c':
            startClient();
            break;
        case 'h':
            showHelp();
            break;
        default:
            printf("Invalid option. Use -h for help.\n");
            return 1;
    }

    return 0;
}

// Placeholder for server functionality
void startServer() {
    printf("Server functionality will be implemented here.\n");
}

// Implements basic client-side networking functionality
void startClient() {
    int sock;
    struct sockaddr_in6 address;

    // Creating the socket
    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return;
    }

    address.sin6_family = AF_INET6;
    address.sin6_port = htons(PORT); // Convert to network byte order
    if (inet_pton(AF_INET6, "::1", &address.sin6_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        return;
    }

    // Connecting to the server
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Connection Failed");
        return;
    }

    printf("Client connected to server.\n");

    // Send a test message
    char *message = "Test command";
    write(sock, message, strlen(message));
    printf("Message sent: %s\n", message);

    // Close the socket
    close(sock);
}

// Display help information
void showHelp() {
    printf("HELP: Usage: -s for server, -c for client, -h for help\n");
}
