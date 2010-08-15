#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>
#include <fcntl.h>
#include <string.h>


int main(int argc, char ** argv)
{
    int fds_map[3] = {-1, -1, -1};
    int i, params;
    extern char **environ;
    char ** new_argv;

    if (argc < 5) {
        fprintf(stderr, "Usage: %s stdin# stdout# stderr# program [argv0 ... ]\n",
                argv[0]);
        return -1;
    }
    
    fds_map[0] = atoi(argv[1]);
    fds_map[1] = atoi(argv[2]);
    fds_map[2] = atoi(argv[3]);

    for (i = 0; i < 3; i++) {
        if (dup2(fds_map[i], i) == -1) {
            fprintf(stderr, "Error in dup2\n");
            return -1;
        }
    }
    for (i = 3; i < sysconf(_SC_OPEN_MAX); i++) {
        params = fcntl(i, F_GETFD, 0);
        if (!(fcntl(i, F_GETFD, 0) & FD_CLOEXEC)) {
            if (close(i) != 0) {
                fprintf(stderr, "Error in close\n");
                return -1;
            }
        }
    }

    new_argv = (char **)malloc(sizeof(char *) * (argc - 3));
    for (i = 4; i < argc; i++) {
        new_argv[i - 4] = argv[i];
    }
    new_argv[argc - 4] = NULL;

    if (execve(argv[4], new_argv, environ) == -1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    return -1;
}
