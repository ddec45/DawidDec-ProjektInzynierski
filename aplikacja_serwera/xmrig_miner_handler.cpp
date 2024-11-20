#include <string>
#include <memory>
#include <fstream>
#include <unistd.h>  
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <cstring>

#define ERROR_CHECK(ret, msg) if(ret){ \
    int e = errno; printf("%s: %s\n", msg, strerror(e)); }

int main(int argc, char** argv){
    char* filename = "/home/ddec45/xmrig_folder/xmrig/build/xmrig";
    char** exec_argv = argv + 0;

    pid_t pid = fork();  
    if(pid == 0){
        ERROR_CHECK((execvp(filename, exec_argv) == -1), "execvp")
        exit(-1);
    }

    while(!waitpid(pid, NULL, WNOHANG)){
        sleep(30);
        ERROR_CHECK((kill(pid, SIGKILL) == -1), "kill")
        puts("check");
        sleep(10);
    }
    return 0;
}