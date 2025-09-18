#include <unistd.h>
#define CMAX 300

int main(int argc, char *argv[]) {
    char buffer[CMAX], c; 
    int i, n;

    while (1) {
        write(1, "$ ", 2);
        i = 0;
        n = read(0, &c, 1);
        while (n > 0 && c != '\n' && i < CMAX - 1) {
            buffer[i++] = c;
            n = read(0, &c, 1);
        }
        write(1, buffer, i);        
        write(1, "\n", 1);  
    }
}