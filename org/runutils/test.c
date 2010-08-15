#include <stdlib.h>
#include <stdio.h>


int main(int argc, char ** argv) {
    int i;
    FILE * f = fopen("/tmp/testoutput", "w+");
    fprintf(f, "argv:\n------\n");
    for (i = 0; i < argc; i ++) {
        fprintf(f, "%d: %s\n", i, argv[i]);
    }
    return argc;
}
