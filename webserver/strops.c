#include <stdio.h>
#include <stdlib.h>

char* pstrip(const char *string, const char *item, int amount) { //preserves the original variable
    int found = 0;
    int i = 0;
    char *msg = malloc(sizeof(string) + 1);
    int len = 0;

    while (string[i] != '\0') {
        msg[i] = string[i];
        i++;
        len++;
    }
    msg[len] = '\0';

    for (int i = 0; i < len;) {
        if (string[i] == *item) {
            for (int j = i; j < len; j++) {
                msg[j] = msg[j + getlen(item)];
            }
            len--;
            found++;
            if (found == amount) break;
        } else {
            i++;
        }
    }
    return msg;
}


void dstrip(char *string, const char *item, int amount) { //destructive to the supplied variable
    int found = 0;
    int i = 0;
    int len = 0;
    while (string[i] != '\0') {
        len++;
        i++;
    }
    i = 0;
    while (string[i] != '\0') {
        if (string[i] == *item) {
            for (int j = i; j < len; j++) {
                string[j] = string[j + getlen(item)];
            }
            found++;
            len--;
            if (found == amount) break;
        } else {
            i++;
        }
    }
    string[len] = '\0';
}

void psplit(const char *string, const char *delim, char *new_1, char *new_2) { //preserves the delimeter
    int i = 0;
    int k = 0;
    int len = 0;
    while (string[i] != '\0') {
        if (string[i] == *delim) {
            for (int j = 0; j != i; j++) {
                new_1[k] = string[j];
                k++;
            }
            new_1[k] = '\0';
            k = 0;
            for (int j = i; string[j] != 0; j++) {
                new_2[k] = string[j];
                k++;
            }
            new_2[k] = '\0';
            len = 0;
            break;
        }
        i++;
    }

}

void dsplit(const char *string, const char *delim, char *new_1, char *new_2) { //removes the delimeter
    int i = 0;
    int k = 0;
    int len = 0;
    while (string[i] != '\0') {
        if (string[i] == *delim) {
            for (int j = 0; j != i; j++) {
                new_1[k] = string[j];
                k++;
            }
            new_1[k] = '\0';
            k = 0;
            for (int j = i + 1; string[j] != 0; j++) {
                new_2[k] = string[j];
                k++;
            }
            new_2[k] = '\0';
            len = 0;
            break;
        }
        i++;
    }

}


int find(const char *string1, const char *string2) { //idk
    int j = 0;
    for (int i = 0; string1[i] != '\0'; i++) {
        while (string1[i] == string2[j]) {
            i++;
            j++;
            if (j == getlen(string2)) return i - j;
        }
        j = 0;
    }
    return 0;
}

int find_all(const char *string1, const char *string2) {
    int k = 0;
    int l = 0;
    int cache;
    for (int i = 0; string1[i] != '\0'; i++) {
        cache = i;
        if (string1[i] == string2[0]) {
            for (int j = 0; string2[j] != '\0'; j++) {
                while (string1[i] == string2[j]) {
                    i++;
                    j++;
                    k++;
                    if (k == getlen(string2)) {
                        l++;
                    }
                }
            }
            printf("k %d\n", k);
            i = cache;
        }
        k = 0;
    }
    return l;
}

int find_pos(const char *string1, const char *string2, char *location) { //includes null terminator so you can loop thru the array easily
    int j = 0;
    int k = 0;
    for (int i = 0; string1[i] != '\0'; i++) {
        while (string1[i] == string2[j]) {
            location[k] = i;
            i++;
            j++;
            k++;
            if (j == getlen(string2)) {
                location[k] = '\0';
                return 1;
            }
        }
        j = 0;
        k = 0;
    }
    return 0;
}

int getlen(const char *string) { // does not include null terminator
    int i = 0;
    while (string[i] != '\0') {
        i++;
    }
    return i;
}

void replace(char *string, const char *to_replace, const char *replacement) {
    char pos[sizeof(string) + 1];
    char cache[sizeof(string) + 1];
    int i = 0;
    while (string[i] != '\0') {
        cache[i] = string[i];
        i++;
    }
    if (find_pos(string, to_replace, pos)) {
        for (i = 0; pos[i] != '\0'; i++) {
            string[pos[i]] = replacement[i];
            if (i == getlen(replacement)) {
                i = pos[i];
                while (cache[i] != '\0') {
                    string[i] = cache[i + getlen(to_replace) - 1];
                    i++;
                }
                string[i] = '\0';
                break;
            }
        }
    }
}

void dcopy(const char *string1, char *string2) {
    for (int i = 0; string1[i] != '\0'; i++) {
        string2[i] = string1[i];
    }
}


char* pcopy(const char *string1) {
    char *string2 = malloc(sizeof(string1) + 1);
    for (int i = 0; string1[i] != '\0'; i++) {
        string2[i] = string1[i];
    }
    return string2;
}


char* pisolate(const char *string, const char *isol) {
    char pos[sizeof(string) + 1];
    find_pos(string, isol, pos);
    char *res = malloc(sizeof(string) + 1);
    for (int i = 0; pos[i] != '\0'; i++) {
        res[i] = string[pos[i]];
      }
    return res;
}


char* disolate(const char *string1, char *string2, const char *isol) {
    char pos[sizeof(string1) + 1];
    find_pos(string1, isol, pos);
    for (int i = 0; pos[i] != '\0'; i++) {
        string2[i] = string1[pos[i]];
      }
}


