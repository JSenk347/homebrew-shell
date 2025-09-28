#ifndef MY_STRING_H
#define MY_STRING_H

/**
* @brief computes and returns the length of a NULL TERMINATED string
* @param s a pointer to the first char of the char array
* @return the length of the string, excluding the null terminator
 */
unsigned int mystrlen(const char *s);

/**
* @brief compares two strings lexicographically. Returns 0 if the two are equal, 
* -1 if the first string is < second second string, 1 if the first string >
* second string.
* @param s1 pointer to one of the strings
* @param s2 pointer to one of the strings
* @return int, the outcome of the compare 
*/
int mystrcmp(const char *s1, const char *s2);

/**
* @brief Copies the src into the destination and returns a pointer to the first char
*        in the destination array.
* @param src pointer to the first char of the char array you want to copy
* @param dest pointer to the first char of the char array you want to copy into
* @return returns a pointer to the first char of the destination char array
*/
char *mystrcpy(const char *src, char *dest);

#endif
