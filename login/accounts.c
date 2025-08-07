//EXAMPLE WEBSERVER CODE

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
        printf("connection from: %d\n", new_socket);
        client_request(new_socket);
        close(new_socket);
    }
    return 1;
}

/*
Basic HTTP functions to speed up coding & readability

*/

void send_http_cookie(int client_socket, int age, const char *data) {
    char header[RESERVED];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Set-Cookie: session_token=%s; Path=/; Max-Age=%d; HttpOnly\r\n"
    "\r\n", data, age); 
    write(client_socket, header, strlen(header));
}

void send_http_head(int client_socket) {
    const char *header = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n";
    write(client_socket, header, strlen(header));
}

void send_http_notfound(int client_socket) {
    const char *not_found = "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body style='background-color:black;'><p style='color:white;'>Nice try bucko but that page doesnt exist</p><html><body>";
    write(client_socket, not_found, strlen(not_found));
}

void client_request(int client_socket) {
    char request[RESERVED];
    ssize_t bytes_read = read(client_socket, request, sizeof(request) - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    request[bytes_read] = '\0';
    printf("Client request: %s\n", request);
    char method[8], data[256];
    sscanf(request, "%s %s", method, data);
    printf("Client request method & data: %s & %s\n", method, data);
    if (strcmp(data, "/") == 0 && !strstr(request, "Cookie:")) {
        send_page(client_socket, "login.html");
        return;
    } else if (strstr(request, "Cookie:")) {
        printf("cookie found\n");
        char cookie[512];
        const char *cookie_position = strstr(request, "Cookie: ");
        int cookie_offset = 22;
        for (int i = cookie_offset; cookie_position[i] != '\n' || cookie_position[i] == ' '; i++) {
            printf("%c\n", cookie_position[i]);
            cookie[i - cookie_offset] = cookie_position[i];
        }
        cookie[strlen(cookie)] = '\0';
        printf("cookie %s\n", cookie);
        get_cookie(client_socket, cookie);
        return;
    } else if (strstr(data, "&")) {
        get_account(client_socket, data);
        return;
    }
    char *path = data + 1;
    send_page(client_socket, path);
}

int send_page(int client_socket, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        send_http_notfound(client_socket);
        return 0;
    }
    printf("sending file %s\n", filepath);
    send_http_head(client_socket);
    char page[RESERVED];
    while (fgets(page, sizeof(page), file)) {
        write(client_socket, page, strlen(page));
    }
    fclose(file);
    return 1;
}

/*
Basic file ops
*/

int check_file_exist(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fclose(file);
        return 0;
    } else if (file) {
        fclose(file);
        return 1;
    } else {
        return 0;
    }
}

/*
Account stuffs
*/

void get_cookie(int client_socket, const char *data) {
    char username[512], password[512];
    printf("reading cookie data\n");
    for (int i = 0; i < strlen(data); i++) {
        if (data[i] =='&') {
            username[i] = '\0';
            break;
        }
        username[i] = data[i];
    }
    for (int i = 0; i < strlen(data); i++) {
        if (data[strlen(username) + i] == '\0') {
            password[i] = '\0';
            break;
        }
        password[i] = data[strlen(username) + i + 1];
    }
    printf("%s\n", username);
    printf("%s\n", password);

    int acc_check = verifiy_account(username, password);
    if (acc_check) {
        if (acc_check == 2) {
            send_page(client_socket, "verify.html");
        } else {
            send_page(client_socket, "index.html");
        }
    } else {
        send_page(client_socket, "login_fail.html");
    }

}

void get_account(int client_socket, char *login_info) {
    char username[128], password[128];
    //check whether to create or login at login_info
    int pass = 0;
    int j = 0;
    for (int i = 0; i < strlen(login_info) + 1; i++) {
        if (login_info[i] == '=') {
            for (j = 0; login_info[j + i] != '\0'; j++) {
                if (!pass) {
                    if (login_info[i + j + 1] == '&') {
                        username[j] = '\0';
                        pass = 1;
                        break;
                    }
                    username[j] = login_info[i + j + 1];
                } else if (pass) {
                    if (login_info[i + j + 1] == '\0') {
                        password[j] = '\0';
                        break;
                    }
                    password[j] = login_info[i + j + 1];
                }
            }
        }
    }
    printf("%s\n", login_info);
    if (strstr(login_info, "/create?")) {
        if(create_account(username, password)) {
            send_page(client_socket,"verify.html");
        } else {
            perror("account creation failed");
        }
    } else if (strstr(login_info, "/login?")) {
        int is_verified = verifiy_account(username, password);
        if (!is_verified) {
            send_page(client_socket, "login_fail.html");
        } else if (is_verified == 1) {
            char login_for_cookie[512];
            snprintf(login_for_cookie, sizeof(login_for_cookie), "%s&%s", username, password);
            send_http_cookie(client_socket, 2400, login_for_cookie);
            send_page(client_socket, "index.html");
        } else if (is_verified == 2) {
            send_page(client_socket, "verify.html");
        }
        return;
    }
}

int create_account(const char *username, const char *password) {
    char path[1024];
    printf("creating account..\n");
    snprintf(path, sizeof(path), "users/%s.conf",username);
    printf("%s\n", path);
    FILE *test = fopen(path, "r");
    printf("file opened\n");
    if (test) {
        printf("account exists\n");
        fclose(test);
        return 0;
    }
    printf("account does not exist\n");

    FILE *file = fopen(path, "w");
    printf("file opened\n");

    fprintf(file, "username:%s\npassword:%s\nverified:0", username, password);
    printf("file written\n");

    fclose(file);
    printf("account created\n");
    return 1;
}

int verifiy_account(const char *username, const char *password) {
    char path[1024];
    printf("logging in\n");
    snprintf(path, sizeof(path), "users/%s.conf",username);
    printf("%s\n", path);
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Account does not exist.\n");
        return 0;
    }
    char login_info[RESERVED];
    size_t data = fread(login_info, 1, sizeof(login_info) - 1, file);
    login_info[data] = '\0';
    char copy[sizeof(login_info)];
    strcpy(copy, login_info);
    
    char *token;
    char *vname, *vpass, *verified;
    int i = 0;
        for (token = strtok(copy, "\n"); token != NULL; token = strtok(NULL, "\n")) {
        char *colon = strchr(token, ':');
        if (colon) {
            if (i == 0) vname = colon + 1;
            else if (i == 1) vpass = colon + 1;
            else if (i == 2) verified = colon + 1;
            i++;
        }
    }
    fclose(file);
    if (strstr(username, vname) && strstr(password, vpass)) {
        printf("account exists: %s\n", verified);
        if (!strcmp(verified, "0")) {
            printf("account not verified\n");
            return 2;
        } else if (!strcmp(verified, "1")) {
            printf("user logged in\n");
            return 1;
        } else {
            printf("unknown exception in login\n");
        }
    } else {
        printf("bad password\n");
        printf("%s\n", username);
        printf("%s\n", vname);
        printf("%s\n", password);
        printf("%s\n", vpass);

        return 0;
    }
}






