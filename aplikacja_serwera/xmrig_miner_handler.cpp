#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>

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
    char** exec_argv;

    std::string s = std::string(argv[1]) + " --http-enabled --http-host 127.0.0.1 --http-port 5678 --http-no-restricted --http-access-token abc -B";
    std::string substr;
    std::stringstream ss1(s);
    int idx = 1;
    while (std::getline(ss1, substr, ' ')){
        idx++;
    }
    idx++;
    exec_argv = (char**)malloc(sizeof(char*)*idx);

    int len = strlen(filename);
    exec_argv[0] = (char*)malloc(sizeof(char)*(len+1));
    memcpy(exec_argv[0], filename, len+1);
    exec_argv[0][len] = '\0';

    std::stringstream ss2(s);
    idx = 1;
    while (std::getline(ss2, substr, ' ')){
        exec_argv[idx] = (char*)malloc(sizeof(char)*(substr.size()+1));
        memcpy(exec_argv[idx], substr.c_str(), substr.size()+1);
        idx++;
    }
    exec_argv[idx] = NULL;

    pid_t pid = fork();  
    if(pid == 0){
        //std::cout << filename << std::endl;
        ERROR_CHECK((execv(filename, exec_argv) == -1), "execvp")
        exit(-1);
    }

    for(int i = 0; i < idx; i++){
        free(exec_argv[i]);
    }
    free(exec_argv);

    while(!waitpid(pid, NULL, WNOHANG)){
        sleep(30);
        ERROR_CHECK((kill(pid, SIGKILL) == -1), "kill")
        //puts("sent kill signal");
        sleep(10);
    }
    return 0;
}