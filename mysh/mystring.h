#ifndef MY_STRING_H
#define MY_STRING_H

/**
* Computes and returns the length of a NULL TERMINATED string
* @param s a pointer to the first char of the char array
* @return the length of the string, excluding the null terminator
 */
unsigned int mystrlen(const char *s);

/**
* Compares two strings. Returns 0 if the two are equal ..., -1 if
* the first string is < second second string, 1 if the first string >
* second string, and 0 if the strings are equal
* 
*
*/
int mystrcmp(const char *s1, const char *s2);
char *mystrcpy(const char *src, char *dest);

#endif
