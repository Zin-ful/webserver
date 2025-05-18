#ifndef STROPS_H
#define STROPS_H

//all char* functions need to be freed after use since we use malloc to allocate the memory needed
char* append(const char *string1, const char *string2); //returns the combination of two strings
char* pstrip(const char *string, const char *item, int amount); //returns the result of the strip
void dstrip(char *string, const char *item, int amount); //edits the supplied string
void psplit(const char *string, const char *delim, char *new_1, char *new_2); //preserves the delimiter
void dsplit(const char *string, const char *delim, char *new_1, char *new_2); //destroys the delimiter
int find(const char *string1, const char *string2); //does not always return 1 so direct if statment calls will fail, instead it returns the first position of the found item or array
int find_all(const char *string1, const char *string2); //returns an int with the total amount of item occurences in the string
int find_pos(const char *string1, const char *string2, char *location); //returns true or false and adds the positions to a char array
int getlen(const char *string); //returns the length of a string without the null-terminator
void simple_replace(char *string, const char *to_replace, const char *replacement) //current alternative to replace/replace all as they dont work
void replace(char *string, const char *to_replace, const char *replacement); //edits the original string
void replace_all(char *string, const char *to_replace, const char *replacement); //literally a for-loop of replace
void replace_pos(char *string, const char to_replace, const char replacement, const char *pos, int increment); //best used with math to dynamically set the increment if needed. Uses an array and incremement to replace specific characters. does not work with string-arrays
void dcopy(const char *string1, char *string2); //edits the 2nd string with the copy
char* pcopy(const char *string1); //returns the copied string
char* pisolate(const char *string, const char *isol);//returns
char* disolate(const char *string1, char *string2, const char *isol); //edits

#endif
