#include "mystring.h"
/*
#include <string.h>      
#include <stdio.h>
*/

unsigned int mystrlen(const char *s)
{
  int i = 0;
  while (s[i] != '\0'){
    i++;
  }
  return i;     
}
int mystrcmp(const char *s1, const char *s2)
{
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}
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