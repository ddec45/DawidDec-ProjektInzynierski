#include "cryptominer_server.hpp"

void request_get_notify(const http_request& req){
    std::cout << req.get_method() << " "<< req.get_path() << std::endl;
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
            response_content <<  "{\"id\":" << miner_app.second.id << ",\"name\":\"" << miner_app.second.name
                << "\",\"description\":\"" << miner_app.second.description << "\"}";
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

std::shared_ptr<http_response> miner_application_get_resource::render_GET(const http_request& req){
    request_get_notify(req);
    try{
        int miner_app_id = std::stoi(req.get_arg("miner_app_id"));
        miner_application_info miner_app = miner_application_info_map.at(miner_app_id);
        std::stringstream response_content;
        response_content <<  "{\"id\":" << miner_app.id << ",\"name\":\"" << miner_app.name
            << "\",\"description\":\"" << miner_app.description << "\"}";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_application_get_resource)

std::shared_ptr<http_response> miner_instance_start_resource::render_PUT(const http_request& req){
    static int id_cnt = 0;

    request_get_notify(req);
    try{
        int miner_app_id = std::stoi(req.get_arg("miner_app_id"));
        miner_application_info miner_app = miner_application_info_map.at(miner_app_id);

        ondemand::parser parser;
        std::string json(req.get_content());
        simdjson::padded_string padded_json(json);
        ondemand::document content = parser.iterate(padded_json);
        //std::cout << json << std::endl;
        
        auto sv = content["input_arguments"].get_string().value();
        std::string input_arguments = std::string(sv);
        //std::cout << input_arguments << std::endl;

        miner_instance_info obj;
        obj.id = ++id_cnt;
        obj.name = miner_app.name;
        obj.miner_app_id = miner_app.id;
        obj.description = miner_app.description;
        obj.statistics = "Put statistics info here.";
        obj.update_timestamp = 0;

        mtx.lock();
        try{
            miner_instance_info_map.insert({obj.id, obj});
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        pid_t pid = fork();
        if(pid == 0){
            //std::cout << miner_app.filename.data() << std::endl;
            ERROR_CHECK((execlp(("./" + miner_app.filename).data(), miner_app.filename.data(), input_arguments.data(), NULL) == -1), "execlp")
            exit(-1);
        }

        std::stringstream response_content;
        response_content <<  "{\"id\":" << obj.id << ",\"name\":\"" << obj.name
                << "\",\"miner_app_id\":" << obj.id  << ",\"description\":\"" << obj.description << "\"}";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_instance_start_resource)

std::shared_ptr<http_response> miner_instance_list_resource::render_GET(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;
        response_content << "[";

        mtx.lock();
        try{
            int size = miner_instance_info_map.size();
            int i = 0;
            for(auto miner_instance : miner_instance_info_map){
                response_content <<  "{\"id\":" << miner_instance.second.id << ",\"name\":\"" << miner_instance.second.name
                    << "\",\"miner_app_id\":\"" << miner_instance.second.miner_app_id
                    << "\",\"description\":\"" << miner_instance.second.description << "\"}";
                    if(++i < size){
                        response_content << ",";
                    }
            }
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        response_content << "]";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_instance_list_resource)

std::shared_ptr<http_response> miner_instance_statistics_list_resource::render_GET(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;
        response_content << "[";
        
        mtx.lock();
        try{
            int size = miner_instance_info_map.size();
            int i = 0;
            for(auto miner_instance : miner_instance_info_map){
                response_content << "{\"miner_instance_id\":" << miner_instance.second.id << ",\"stats\":\"" << miner_instance.second.statistics << "\"}";
                    if(++i < size){
                        response_content << ",";
                    }
            }
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        response_content << "]";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_instance_statistics_list_resource)

std::shared_ptr<http_response> miner_instance_statistics_get_resource::render_GET(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;  
        
        mtx.lock();
        try{
            int miner_instance_id = std::stoi(req.get_arg("miner_instance_id"));
            miner_instance_info miner_instance = miner_instance_info_map.at(miner_instance_id);
            response_content << "{\"miner_instance_id\":" << miner_instance.id << ",\"stats\":\"" << miner_instance.statistics << "\"}";
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_instance_statistics_get_resource)