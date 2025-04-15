#ifndef STROPS_H
#define STROPS_H

char* pstrip(const char *string, const char *item, int amount);
void dstrip(char *string, const char *item, int amount);
void psplit(const char *string, const char *delim, char *new_1, char *new_2);
void dsplit(const char *string, const char *delim, char *new_1, char *new_2);
int find(const char *string1, const char *string2);
int find_all(const char *string1, const char *string2);
int find_pos(const char *string1, const char *string2, char *location);
int getlen(const char *string);
void replace(char *string, const char *to_replace, const char *replacement);
void dcopy(const char *string1, char *string2);
char* pcopy(const char *string1);
char* pisolate(const char *string, const char *isol);
char* disolate(const char *string1, char *string2, const char *isol);

#endif
