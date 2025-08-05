#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define RESERVED 8192
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failure");
        exit(EXIT_FAILURE);

    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failure");
            exit(EXIT_FAILURE);
        }
    if (listen(server_fd, 3) < 0) {

        perror("listen failure");
        exit(EXIT_FAILURE);
    }
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept failure");
            exit(EXIT_FAILURE);
        }
        printf("Connection accepted!\n");
        prompt_login(new_socket);
        close(new_socket);
    }
    return 1;
}


void prompt_login(int client_socket) {
    FILE *file = fopen("login.html", "r");
    if (!file) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body style='background-color:black;'><p style='color:white;'>Nice try bucko but that page doesnt exist</p><html><body>";
        write(client_socket, not_found, strlen(not_found));
        return;
    }
    const char *header = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n";
    write(client_socket, header, strlen(header));

    char buffer[RESERVED];
    printf("reached buffer\n");
    while (fgets(buffer, sizeof(buffer), file)) {
        write(client_socket, buffer, strlen(buffer));
    }
    printf("\n\nBUFFER %s\n\n", buffer);
    fclose(file);


}










