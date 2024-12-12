#include "simdjson.h"
using namespace simdjson;

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

#define ERROR_CHECK(msg, code) if(errno){ \
    int e = errno; fprintf(stderr, "%s: %d %s\n", msg, e, strerror(e)); exit(code);}

int g_end = 0;

void signal_handler(int s_input){
    g_end = 1;
    puts("Signal get.");
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->assign((char*)contents, size * nmemb);
    return size * nmemb;
}

int main(int argc, char** argv){
    if(argc < 2){
        exit(-1);
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* xmrig_filename = "xmrig_miner_adapter_script.sh";
    const char* config_filename = "xmrig_config.json";
    std::string s_xmrig_filename = xmrig_filename;
    std::string s_config_filename = config_filename;

    int miner_id = std::stoi(argv[1]);
    std::string s_miner_id = std::to_string(miner_id);

    //Wczytywanie pliku config
    std::fstream config_file_stream;
    config_file_stream.open(config_filename, std::ios_base::in);

    config_file_stream.seekg (0, config_file_stream.end);
    int config_file_length = config_file_stream.tellg();
    config_file_stream.seekg (0, config_file_stream.beg);

    char* buffer = new char[config_file_length];
    config_file_stream.read(buffer, config_file_length);
    std::string config_file_content = buffer;
    delete buffer;
    //std::cout << "Config file content:\n" << config_file_content;

    //Parsowanie zawartości
    ondemand::parser parser;
    std::string json(config_file_content);
    simdjson::padded_string padded_json(json);
    ondemand::document content = parser.iterate(padded_json);
    
    auto http = content["http"].get_object();
    std::string http_host(http["host"].get_string().value());
    unsigned int http_port = http["port"].get_uint64() + miner_id; // Ważne!
    std::string http_access_token(http["access-token"].get_string().value());
    int http_restricted = http["restricted"].get_bool();

    //Tworzenie wejściowych argumentów i tworzenie wątku koparki
    char** exec_argv;
    exec_argv = (char**)malloc(sizeof(char*)*4);
    std::string s_input = std::string("--config=") + config_filename + " --http-port=" + std::to_string(http_port)
        + " --http-enabled --http-host=" + http_host + " --http-access-token=" + http_access_token + (http_restricted ? " --http-no-restricted" : "");// + " --http-enabled --http-host " + XMRIG_HOST + " --http-port " + XMRIG_PORT + " --http-access-token abc --http-no-restricted -B";
    if(argv[2] != nullptr){
        s_input += std::string(" ") + argv[2];
    }

    exec_argv[0] = (char*)malloc(sizeof(char)*(s_xmrig_filename.size()+1));
    memcpy(exec_argv[0], s_xmrig_filename.c_str(), s_xmrig_filename.size()+1);
    exec_argv[1] = (char*)malloc(sizeof(char)*(s_miner_id.size()+1));
    memcpy(exec_argv[1], s_miner_id.c_str(), s_miner_id.size()+1);
    exec_argv[2] = (char*)malloc(sizeof(char)*(s_input.size()+1));
    memcpy(exec_argv[2], s_input.c_str(), s_input.size()+1);
    exec_argv[4] = NULL;

    pid_t pid = fork();
    if(pid == 0){
        execv(xmrig_filename, exec_argv);
        ERROR_CHECK("execvp", -2)
    }

    // Z jakiegoś powodu nie działa
    // struct sigaction sa;
    // sa.sa_handler = signal_handler;
    // sigaction(SIGINT, &sa, NULL);
    // ERROR_CHECK("sigaction",-3)
    // sigaction(SIGTERM, &sa, NULL);
    // ERROR_CHECK("sigaction",-3)

    for(int i = 0; i < 3; i++){
        free(exec_argv[i]);
    }
    free(exec_argv);

    //Tworzenie uchwytów do wysyłania żądań i pobierania odpowiedzi
    CURL* xmrig_handle = curl_easy_init();
    if(!xmrig_handle){
        exit(-4);
    }
    CURL* cryptominer_server_handle = curl_easy_init();
    if(!cryptominer_server_handle){
        exit(-5);
    }

    std::string readBuffer;

    std::string url = http_host + ":" + std::to_string(http_port) + "/2/summary";
    curl_easy_setopt(xmrig_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(xmrig_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(xmrig_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(xmrig_handle, CURLOPT_WRITEDATA, &readBuffer);

    struct curl_slist *xmrig_list = NULL;
    xmrig_list = curl_slist_append(xmrig_list, "Content-Type: application/json");
    xmrig_list = curl_slist_append(xmrig_list, (std::string("Authorization: Bearer ") + http_access_token).c_str());
    curl_easy_setopt(xmrig_handle, CURLOPT_HTTPHEADER, xmrig_list);
    curl_easy_setopt(xmrig_handle, CURLOPT_HTTPGET, 1L);

    url = "https://localhost:8080/admin/mining_statistics/send/" + std::to_string(miner_id);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_WRITEDATA, &readBuffer);

    struct curl_slist *cryptominer_server_list = NULL;
    cryptominer_server_list = curl_slist_append(cryptominer_server_list, "Content-Type: application/json");
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_HTTPHEADER, cryptominer_server_list);
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_CUSTOMREQUEST, "PUT");

    //Wysyłanie statystyk
    ondemand::parser response_parser1, response_parser2;
    int server_request_code = 0;
    long response_code = 0;
    std::string update_info, request_body;
    sleep(5);
    while(!waitpid(pid, NULL, WNOHANG) && !g_end){
        try{
            CURLcode code  = curl_easy_perform(xmrig_handle);
            curl_easy_getinfo(xmrig_handle, CURLINFO_RESPONSE_CODE, &response_code);
            if(response_code != 200){
                break;
            }
            std::string response_json1 = readBuffer;
            //std::cout << "test: " << response_json1 << std::endl;
            simdjson::padded_string response_padded_json1(response_json1);
            ondemand::document response_content1 = response_parser1.iterate(response_padded_json1);

            //std::string response_content_string = std::string(response_content1.raw_json().value());
            std::string hashrate = std::string(response_content1["hashrate"]["total"].raw_json().value());
            std::stringstream ss;
            ss << "{\"end_code\":false,\"stats\":\"Total hashrate: " << hashrate /*response_content_string*/ << "\"}";
            request_body = ss.str();
            //std::cout << request_body << std::endl;
            curl_easy_setopt(cryptominer_server_handle, CURLOPT_POSTFIELDS, request_body.c_str());

            code = curl_easy_perform(cryptominer_server_handle);
            curl_easy_getinfo(xmrig_handle, CURLINFO_RESPONSE_CODE,&response_code);
            if(response_code != 200){
                break;
            }
            std::string response_json2 = readBuffer;
            //std::cout << response_json2 << std::endl;
            simdjson::padded_string response_padded_json2(response_json2);
            ondemand::document response_content2 = response_parser2.iterate(response_padded_json2);
            server_request_code = response_content2["request_code"].get_uint64();
            if(server_request_code == 2){
                break;
            }
            if(server_request_code == 1){
                // update_info nie jest na razie nigdzie wykorzystywane
                update_info = std::string(response_content2["update_info"].get_string().value());
            }

            sleep(10);
        }
        catch(...){
           break;
        }
    }

    //Wysyłanie żądania z informacją o zakończeniu kopania
    request_body = std::string("{\"end_code\":true,\"stats\":\"\"}");
    curl_easy_setopt(cryptominer_server_handle, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_perform(cryptominer_server_handle);
    kill(pid, SIGINT);
    ERROR_CHECK("kill", -6)
    puts("xmrig_miner_handler end.");
    curl_slist_free_all(xmrig_list);
    curl_slist_free_all(cryptominer_server_list);
    curl_easy_cleanup(xmrig_handle);
    curl_easy_cleanup(cryptominer_server_handle);
    return 0;
}