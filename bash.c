/**
 * Summary: Contains the main function for a simple shell program.
 * 
 * Current functionality: Prompts the user for input and echoes it back to the
 *      screen.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * file: bash.c
 */

#include <unistd.h>
#define MAX_TOKENS 300

void prompt_echo(); /// Function prototype

int main() {
    
    prompt_echo();
    
    return 0;
}

/**
 * Purpose: This function prompts the user for input, reads the input from
 *          the keyboard, and echos it back to the screen using low-level I/O
 *          system calls.
 */
void prompt_echo() {
    char buffer[MAX_TOKENS], c; 
    int i, j, n;

    while (1) {
        write(1, "$ ", 2);
        i = 0;

        /**reads specified number of characters (1) from the input stream
         *(fd = 0) into c (&c)*/
        n = read(0, &c, 1);
        
        while (n > 0 && c != '\n' && i < MAX_TOKENS - 1) {
            buffer[i++] = c;
            n = read(0, &c, 1);
        }

        //check for input exceeding max char limit
        if (c != '\n') {
            // error message
            write(1, "Error: input exceeds max char limit\n", 37);

            //clear input until newline
            while (n > 0 && c != '\n') {
                n = read(0, &c, 1);
            }

            //skips the rest of the loop and starts over
            continue;
        }

        //check for "exit"
        if (i == 4 && (buffer[0] == 'e' || buffer[0] == 'E') && buffer[1] == 'x'
            && buffer[2] == 'i' && buffer[3] == 't') {
            return;
        }
        
        //echo input back to screen
        write(1, buffer, i);        
        if (i > 0){
            write(1, "\n", 1);
        }
    }
}