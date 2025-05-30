#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Structure to map file extensions to MIME types
struct {
    char *ext;
    char *filetype;
} extensions[] = {
    {"gif", "image/gif"},
    {"jpg", "image/jpg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"html", "text/html"},
    {"txt", "text/plain"},
    {"json", "application/json"},
    {NULL, NULL}
};

// Function to get MIME type from file extension
char* get_mime_type(char* filename) {
    char* ext = strrchr(filename, '.');
    if (ext == NULL) return "text/plain";
    ext++; // Skip the dot

    for (int i = 0; extensions[i].ext != NULL; i++) {
        if (strcmp(ext, extensions[i].ext) == 0) {
            return extensions[i].filetype;
        }
    }
    return "text/plain";
}

// Function to handle HTTP requests
void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    FILE* file;
    int bytes_read;

    // Read the request
    bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Error reading from socket");
        return;
    }
    buffer[bytes_read] = '\0';

    printf("\n=== New Request ===\n");
    printf("Raw request:\n%s\n", buffer);

    // Parse the request to get the file path
    if (sscanf(buffer, "GET %s HTTP/1.1", path) != 1) {
        printf("Invalid request format\n");
        // Send 400 Bad Request
        sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        if (write(client_socket, response, strlen(response)) < 0) {
            perror("Error writing response");
        }
        return;
    }

    printf("Requested path: %s\n", path);
    
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }

    // Remove leading slash
    memmove(path, path + 1, strlen(path));

    printf("Looking for file: %s\n", path);

    // Try to open the file
    file = fopen(path, "rb");
    if (file == NULL) {
        printf("File not found: %s\n", path);
        // Send 404 Not Found
        sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        if (write(client_socket, response, strlen(response)) < 0) {
            perror("Error writing response");
        }
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Send 200 OK with appropriate headers
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", 
            get_mime_type(path), file_size);
    
    if (write(client_socket, response, strlen(response)) < 0) {
        perror("Error writing headers");
        fclose(file);
        return;
    }

    printf("Sending file: %s (size: %ld bytes)\n", path, file_size);

    // Send file contents
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (write(client_socket, buffer, bytes_read) < 0) {
            perror("Error writing file contents");
            break;
        }
    }

    fclose(file);
    printf("=== Request Complete ===\n\n");
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options - only use SO_REUSEADDR
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d...\n", PORT);

    while (1) {
        // Accept connection
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("\n=== New Connection ===\n");
        printf("Client connected from: %s:%d\n", 
               inet_ntoa(address.sin_addr), 
               ntohs(address.sin_port));

        // Handle the request
        handle_request(client_socket);
        
        // Close the connection
        if (close(client_socket) < 0) {
            perror("Error closing socket");
        }
        printf("Connection closed\n");
    }

    return 0;
}
