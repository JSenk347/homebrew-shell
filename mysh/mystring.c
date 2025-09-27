#include "mystring.h"
#include <string.h>       /* TO DO: initial cheat! Remove this line and all library dependency evetually */


unsigned int mystrlen(const char *s)
{
  return strlen(s);     /* TO DO: replace string library wrappers with non-library code */
}


int mystrcmp(const char *s1, const char *s2)
{
  return strcmp(s1, s2);
}


char *mystrcpy(char *dest, const char *src)
{
  return strcpy(dest, src);
}
