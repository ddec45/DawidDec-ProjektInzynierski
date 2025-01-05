#ifndef CRYPTOMINER_SERVER_H 
#define CRYPTOMINER_SERVER_H 

#include "httpserver.hpp"
using namespace httpserver;
#include "simdjson.h"
using namespace simdjson;

#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

#include <unistd.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <cstring>
#include <ctime>

#define ERROR_CHECK(msg) if(errno){ \
    int e = errno; fprintf(stderr, "%s: %s\n", msg, strerror(e)); /*throw;*/}

#define NOTFOUND_METHOD_INSERTER(name) std::shared_ptr<http_response> name::render(const http_request& req){ \
    return std::shared_ptr<http_response>(new string_response("{\"message\":\"Resource not found.\"}", 404)); \
}

struct config_file_content_class{
    std::string user_api_key;
	std::string admin_api_key;
	std::string ssl_certificate;
	std::string ssl_key;

    unsigned int port;
	unsigned int max_nr_of_miner_instances;
	unsigned int instance_statistics_length;
	//std::unordered_map<int, miner_application_info> miner_applications;
	int update_period;
};

#define DEFAULT_CODE 0
#define UPDATE_CODE 1
#define END_CODE 2
struct miner_instance_info{
    int id;
    std::string name;
    int miner_app_id;
    std::string description;
    std::string statistics;
    std::string update_info;
    int status_code;
    int end_code;

    pid_t process_id;
    int update_timestamp;
};

struct miner_application_info{
	std::string filename;
	int id;
	std::string name;
	std::string description;
};

extern ondemand::parser config_file_parser;
extern std::string config_file_content;
extern ondemand::document config_file;

extern std::unordered_map<int, miner_instance_info> miner_instance_info_map;
extern std::unordered_map<int, miner_application_info> miner_application_info_map;
extern config_file_content_class config_file_content_object;

extern std::mutex mtx;

void request_get_notify(const http_request& req);
int check_api_key(std::string key_from_request, std::string key_from_server);

#define CHECK_API_KEY(key_from_request, key_from_server) if(check_api_key(key_from_request, key_from_server)){ \
    return std::shared_ptr<http_response>(new string_response("{\"message\":\"Incorrect api key.\"}", 401, "application/json")); \
}

class hello_world_resource : public http_resource {
public:
    std::shared_ptr<http_response> render(const http_request&);
};

class miner_application_list_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_GET(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_application_get_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_GET(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_start_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_POST(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_list_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_GET(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_statistics_list_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_GET(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_statistics_get_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_GET(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_update_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_PUT(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_delete_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_DELETE(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class send_mining_statistics_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_PUT(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

#endif /* CRYPTOMINER_SERVER_H */