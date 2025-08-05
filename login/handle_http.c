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
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
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

void get_login(int client_socket) {

    char buffer[RESERVED];
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes_read] = '\0';
    char method[8], login_info[256], username[128], password[128];
    sscanf(buffer, "%s %s", method, login_info);
    //check whether to create or login at login_info
    int pass = 0;
    int j = 0;
    for (int i = 0; i < strlen(login_info) + 1; i++) {
        if (login_info[i] == '=') {
            for (j = 0; login_info[j + i] != '\0'; j++)
                if (!pass) {
                    if (login_info[i + j + 1] == '&') {
                        username[j] = '\0';
                        pass = 1;
                        break;
                    }
                    username[j] = login_info[i + j + 1];
                } else if (pass) {
                    if (login_info[i + j + 1] == '\0') {
                        password[j + 1] = '\0';
                        break;
                    }
                    password[j] = login_info[i + j + 1];
                }
        }

    }
    printf("method: %s\ntype: %s\npass: %s\nuser: %s\n", method, login_info, password, username);

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
    fclose(file);
    
    get_login(client_socket);


}










