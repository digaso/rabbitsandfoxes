#include <stdio.h>
#include <stdlib.h>
#include "rabbitsandfoxes.h"

int main(int argc, char **argv) {

    int sequential = 0, threads = 1;

    if (argc > 1) {
        threads = atoi(argv[1]);

        if (threads <= 0) {
            sequential = 1;
        }
    }

    if (!sequential) {
        runParallelSimulation(threads, stdin, stdout);
    } else {
        runSequentialSimulation(stdin, stdout);
    }

    return 0;
}
