#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <curl/curl.h>

#include <unistd.h>  
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <cstring>

#define ERROR_CHECK(ret, msg) if(ret){ \
    int e = errno; printf("%s: %s\n", msg, strerror(e)); }

#define HOST "127.0.0.1"
#define PORT "5678"

int main(int argc, char** argv){
    curl_global_init(CURL_GLOBAL_DEFAULT);

    char* filename = "/home/ddec45/xmrig_folder/xmrig/build/xmrig";
    char** exec_argv;

    std::string s = std::string(argv[1]) + " --http-enabled --http-host " + HOST + " --http-port " + PORT + " --http-no-restricted --http-access-token abc -B";
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

    CURL* xmrig_handle = curl_easy_init();
    if(!xmrig_handle){
        exit(-2);
    }
    CURL* cryptominer_server_handle = curl_easy_init();
    if(!cryptominer_server_handle){
        exit(-3);
    }
    std::string url = std::string("http:/") + HOST + ":" + PORT + "/";
    curl_easy_setopt(xmrig_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(xmrig_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(xmrig_handle, CURLOPT_HTTPGET, 1L);

    CURLcode code  = curl_easy_perform(xmrig_handle);

    url = "https:/127.0.0.1:8080/user/miner/application/list";
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_HTTPGET, 1L);

    struct curl_slist *list = NULL;
    list = curl_slist_append(list, "Content-Type: application/json");
    //list = curl_slist_append(list, "Accept:");
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_HTTPHEADER, list);

    code  = curl_easy_perform(cryptominer_server_handle);

    while(!waitpid(pid, NULL, WNOHANG)){
        sleep(20);
        ERROR_CHECK((kill(pid, SIGKILL) == -1), "kill")
        puts("sent kill signal");
        sleep(5);
    }

    curl_slist_free_all(list); /* free the list */
    curl_easy_cleanup(xmrig_handle);
    curl_easy_cleanup(cryptominer_server_handle);
    return 0;
}