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

#include <unistd.h>  
#include <sys/errno.h>
#include <cstring>

#define ERROR_CHECK(ret, msg) if(ret){ \
    int e = errno; printf("%s: %s\n", msg, strerror(e)); throw;}

#define NOTFOUND_METHOD_INSERTER(name) std::shared_ptr<http_response> name::render(const http_request& req){ \
    return std::shared_ptr<http_response>(new string_response("Not Found", 404)); \
}


struct miner_instance_info{
    int id;
    std::string name;
    int miner_app_id;
    std::string description;
    std::string statistics;
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


void request_get_notify();

class hello_world_resource : public http_resource {
public:
    std::shared_ptr<http_response> render(const http_request&);
};

class json_resource : public http_resource{
public:
    std::shared_ptr<http_response> render_POST(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_application_list_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_GET(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

class miner_instance_start_resource: public http_resource{
public:
    std::shared_ptr<http_response> render_PUT(const http_request& req);
    std::shared_ptr<http_response> render(const http_request& req);
};

#endif /* CRYPTOMINER_SERVER_H */