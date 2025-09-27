#include "mystring.h"
#include <string.h>       /* TO DO: initial cheat! Remove this line and all library dependency evetually */
#include <stdio.h>

unsigned int mystrlen(const char *s)
{
  int i = 0;
  while (s[i] != ('\n' || '\0')){
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


char *mystrcpy(char *dest, const char *src)
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

int main(){
  char s1[100] = "hello there";
  char s2[100] = "how are you?";

  printf("String Length: %u\n", mystrlen(s1));
  printf("String Comp: %i\n", mystrcmp(s1, s2));
  printf("Copied String: %s\n", mystrcpy(s1, s2));
}
