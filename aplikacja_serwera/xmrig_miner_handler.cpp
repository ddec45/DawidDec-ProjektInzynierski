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

#define XMRIG_HOST "127.0.0.1"
#define XMRIG_PORT "5678"
#define XMRIG_TOKEN "abc"

int main(int argc, char** argv){
    curl_global_init(CURL_GLOBAL_DEFAULT);

    char* filename = "/home/ddec45/xmrig_folder/xmrig/build/xmrig";
    std::string s_filename = filename;
    char** exec_argv;

    std::string s = std::string(argv[1]) + " --http-enabled --http-host " + XMRIG_HOST + " --http-port " + XMRIG_PORT + " --http-access-token abc --http-no-restricted -B";
    std::string substr;
    std::stringstream ss1(s);
    int idx = 1;
    while (std::getline(ss1, substr, ' ')){
        idx++;
    }
    idx++;
    exec_argv = (char**)malloc(sizeof(char*)*idx);

    // int len = strlen(filename);
    // exec_argv[0] = (char*)malloc(sizeof(char)*(len+1));
    // memcpy(exec_argv[0], filename, len+1);
    exec_argv[0] = (char*)malloc(sizeof(char)*(s_filename.size()+1));
    memcpy(exec_argv[0], s_filename.c_str(), s_filename.size()+1);

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
    std::string url = std::string(XMRIG_HOST) + ":" + XMRIG_PORT + "/2/summary";
    curl_easy_setopt(xmrig_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(xmrig_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(xmrig_handle, CURLOPT_HTTPGET, 1L);

    struct curl_slist *xmrig_list = NULL;
    xmrig_list = curl_slist_append(xmrig_list, "Content-Type: application/json");
    xmrig_list = curl_slist_append(xmrig_list, (std::string("Authorization: Bearer ") + XMRIG_TOKEN).c_str());
    curl_easy_setopt(xmrig_handle, CURLOPT_HTTPHEADER, xmrig_list);

    CURLcode code  = curl_easy_perform(xmrig_handle);

    url = "https:/127.0.0.1:8080/user/miner/application/list";
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(cryptominer_server_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_HTTPGET, 1L);

    struct curl_slist *cryptominer_server_list = NULL;
    cryptominer_server_list = curl_slist_append(cryptominer_server_list, "Content-Type: application/json");
    //cryptominer_server_list = curl_slist_append(list, "Accept:");
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_HTTPHEADER, cryptominer_server_list);

    //code  = curl_easy_perform(cryptominer_server_handle);

    int response_code = 0;
    while(!waitpid(pid, NULL, WNOHANG)){
        sleep(5);
        ERROR_CHECK((kill(pid, SIGKILL) == -1), "kill")
        puts("sent kill signal.");
        sleep(5);
    }

    puts("xmrig handler end.");
    curl_slist_free_all(cryptominer_server_list); /* free the list */
    curl_easy_cleanup(xmrig_handle);
    curl_easy_cleanup(cryptominer_server_handle);
    return 0;
}