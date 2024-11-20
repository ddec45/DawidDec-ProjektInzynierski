#include "cryptominer_server.hpp"

void request_get_notify(const http_request& req){
    std::cout << "Request: " << req.get_path() << std::endl;
    return;
}

std::shared_ptr<http_response> hello_world_resource::render(const http_request& req) {
    request_get_notify(req);
    return std::shared_ptr<http_response>(new string_response(config_file_content + '\n', 200, "application/json"));
}

std::shared_ptr<http_response> json_resource::render_POST(const http_request& req){
    request_get_notify(req);
    try{
        ondemand::parser parser;
        std::string json(req.get_content());
        simdjson::padded_string padded_json(json);
        ondemand::document tweets = parser.iterate(padded_json);
        std::string s = std::to_string(uint64_t(tweets["search_metadata"]["count"]));
        s += " results.";
        return std::shared_ptr<http_response>(new string_response(s, 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(json_resource)

std::shared_ptr<http_response> miner_application_list_resource::render_GET(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;
        response_content << "[";
        int size = miner_application_info_map.size();
        int i = 0;
        for(auto miner_app : miner_application_info_map){
            response_content <<  "{\"name\":\"" << miner_app.second.name << "\",\"id\":" << miner_app.second.id
                << ",\"description\":\"" << miner_app.second.description << "\"}";
                if(++i < size){
                    response_content << ",";
                }
        }
        response_content << "]";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_application_list_resource)