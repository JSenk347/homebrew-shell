/**
 * Purpose: This program reads user input from the keyboard and echos it back
 *          to the screen.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#include <unistd.h>
#define MAX_TOKENS 300

int main() {
    char buffer[MAX_TOKENS], c; 
    int i, j, n;

    while (1) {
        write(1, "$ ", 2);
        i = 0;
        n = read(0, &c, 1); //reads specified number of characters (1) from the input stream (fd = 0) into c (&c)
        while (n > 0 && c != '\n' && i < MAX_TOKENS - 1) {
            buffer[i++] = c;
            n = read(0, &c, 1);
        }

        //check for "exit"
        if (i == 4 && (buffer[0] == 'e' || buffer[0] == 'E') && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't'){
            return 0;
        }
        
        write(1, buffer, i);        
        if (i > 0){write(1, "\n", 1);}
    }
}