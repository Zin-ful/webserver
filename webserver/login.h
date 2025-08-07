#ifndef LOGIN_H
#define LOGIN_H

void get_cookie(int client_socket, const char *data, char *username, char *password);
void get_account(int client_socket, char *login_info, char *username, char *password);
int create_account(const char *username, const char *password);
int verifiy_account(const char *username, const char *password);

#endif