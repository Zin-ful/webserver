#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define RESERVED 8192

void get_cookie(int client_socket, const char *data, char *username, char *password) {
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
}

void get_account(int client_socket, char *login_info, char *username, char *password) {
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
        return 0;
    }
}
