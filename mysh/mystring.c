/**
 * Summary: Contains string manipulation functions.
 * 
 * Current functionality: Implements custom versions of strlen, strcmp, and
 *    strcpy.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * @file: mystring.c
 */

#include "mystring.h"
/*
#include <string.h>
#include <stdio.h>
*/

/**
* @brief computes and returns the length of a NULL TERMINATED string.
*
* @param s a pointer to the first char of the char array
* @return the length of the string, excluding the null terminator
*/
unsigned int mystrlen(const char *s)
{
  int i = 0;
  while (s[i] != ('\0')){
    i++;
  }
  return i;
}

/**
* @brief compares two strings lexicographically. Returns 0 if the two are equal,
*        -1 if the first string is < second second string, 1 if the first
*        string > second string.
*
* @param s1 pointer to one of the strings
* @param s2 pointer to one of the strings
* @return int, the outcome of the compare
*/
int mystrcmp(const char *s1, const char *s2)
{
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

/**
* @brief Copies the src into the destination and returns a pointer to the first char
*        in the destination array.
*
* @param src pointer to the first char of the char array you want to copy
* @param dest pointer to the first char of the char array you want to copy into
* @return returns a pointer to the first char of the destination char array
*/
char *mystrcpy(const char *src, char *dest)
{
  char *orig_dest = dest;
  while(*src){
    *dest = *src;
    src++;
    dest++;
  }
  *dest = '\0';
  return orig_dest;
}

//For testing
/*
int main(){
  char s1[] = "what it do boo?";
  char s2[] = "not much boo wbu?";

  printf("String Length (s1): %u\n", mystrlen(s1));
  printf("String Length (s2): %u\n", mystrlen(s2));
  printf("String Comp (s1, s2): %i\n", mystrcmp(s1, s2));
  printf("String Comp (s1, s2): %i\n", strcmp(s1, s2));
  printf("Copied String (s1, s2): s2 = '%s'\n", mystrcpy(s1, s2));
}
*/