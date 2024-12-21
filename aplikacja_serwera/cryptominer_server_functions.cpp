#include "cryptominer_server.hpp"

void request_get_notify(const http_request& req){
    time_t t;
    time(&t);
    std::cout << req.get_method() << " " << req.get_path() << "\t" << ctime(&t) << std::flush;
    return;
}

std::shared_ptr<http_response> hello_world_resource::render(const http_request& req) {
    request_get_notify(req);
    return std::shared_ptr<http_response>(new string_response(config_file_content + '\n', 200, "application/json"));
}

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

std::shared_ptr<http_response> miner_instance_start_resource::render_POST(const http_request& req){
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
        
        std::string input_arguments = std::string(content["input_arguments"].get_string().value());
        //std::cout << input_arguments << std::endl;

        miner_instance_info obj;
        obj.id = ++id_cnt;
        obj.name = miner_app.name;
        obj.miner_app_id = miner_app.id;
        obj.description = miner_app.description;
        obj.statistics = "{\"default\": \"No statistics information has been sent yet.\"}";
        obj.update_info = "";
        obj.status_code = DEFAULT_CODE;
        obj.update_timestamp = time(NULL) + 5;

        pid_t pid = fork();
        if(pid == 0){
            //std::cout << miner_app.filename.data() << std::endl;
            execlp(("./" + miner_app.filename).c_str(), miner_app.filename.c_str(), std::to_string(obj.id).c_str(), input_arguments.c_str(), NULL);
            ERROR_CHECK("execlp")
            exit(-1);
        }
        obj.process_id = pid;

        mtx.lock();
        try{
            miner_instance_info_map.insert({obj.id, obj});
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        std::stringstream response_content;
        response_content <<  "{\"id\":" << obj.id << ",\"name\":\"" << obj.name
                << "\",\"miner_app_id\":" << obj.id  << ",\"description\":\"" << obj.description << "\"}";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 201, "application/json"));
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
                    //std::cout << miner_instance.second.update_info << " " << miner_instance.second.is_checked_for_deletion << std::endl;
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
                response_content << "{\"miner_instance_id\":" << miner_instance.second.id << ",\"stats\":" << miner_instance.second.statistics << "}";
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
        int miner_instance_id = std::stoi(req.get_arg("miner_instance_id"));

        mtx.lock();
        try{
            miner_instance_info miner_instance = miner_instance_info_map.at(miner_instance_id);
            response_content << "{\"miner_instance_id\":" << miner_instance.id << ",\"stats\":" << miner_instance.statistics << "}";
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

std::shared_ptr<http_response> miner_instance_update_resource::render_PUT(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;  
        int miner_instance_id = std::stoi(req.get_arg("miner_instance_id"));

        ondemand::parser parser;
        std::string json(req.get_content());
        simdjson::padded_string padded_json(json);
        ondemand::document content = parser.iterate(padded_json);
        
        auto sv = content["update_info"].get_string().value();
        std::string update_info = std::string(sv);

        std::stringstream message;
        int ret_code;
        mtx.lock();
        try{
            miner_instance_info &miner_instance = miner_instance_info_map.at(miner_instance_id);
            if(miner_instance.status_code == END_CODE){
                message << "Miner instance with id " << miner_instance_id << " is checked for deletion";
                ret_code = 410;
            }
            else{
                miner_instance.update_info = update_info;
                miner_instance.status_code = UPDATE_CODE;
                message << "Update information for miner instance with id " << miner_instance.id << " sent successfully";
                ret_code = 200;
            }
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        response_content <<  "{\"message\":" << message.str() << "\"}";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), ret_code, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_instance_update_resource)

std::shared_ptr<http_response> miner_instance_delete_resource::render_DELETE(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;  
        int miner_instance_id = std::stoi(req.get_arg("miner_instance_id"));

        std::stringstream message;
        mtx.lock();
        try{
            miner_instance_info &miner_instance = miner_instance_info_map.at(miner_instance_id);
            miner_instance.status_code = END_CODE;
            message << "Miner instance with id " << miner_instance.id << " is now checked for deletion";
        }
        catch(...){
            mtx.unlock();
            return std::shared_ptr<http_response>(new string_response("error", 500));
        }
        mtx.unlock();

        response_content <<  "{\"message\":\"" << message.str() << "\"}";
        return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
    }
    catch(...){
        return std::shared_ptr<http_response>(new string_response("error", 400));
    }
}
NOTFOUND_METHOD_INSERTER(miner_instance_delete_resource)

std::shared_ptr<http_response> send_mining_statistics_resource::render_PUT(const http_request& req){
    request_get_notify(req);
    try{
        std::stringstream response_content;  
        int miner_instance_id = std::stoi(req.get_arg("miner_instance_id"));

        ondemand::parser parser;
        std::string json(req.get_content());
        simdjson::padded_string padded_json(json);
        ondemand::document content = parser.iterate(padded_json);
        
        int end_code = content["end_code"].get_bool();
        // auto sv = content["stats"].get_string().value();
        // std::string stats = std::string(sv);
        std::string stats = std::string(content["stats"].raw_json().value());
        // std::cout << stats << std::endl;

        std::stringstream message;
        mtx.lock();
        try{
            if(end_code){
                miner_instance_info_map.erase(miner_instance_id);
                response_content <<  "{\"request_code\":" << DEFAULT_CODE << ",\"update_info\":\"\"}";
                mtx.unlock();
                return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
            }

            miner_instance_info &miner_instance = miner_instance_info_map.at(miner_instance_id);
            if(miner_instance.status_code == END_CODE){
                response_content <<  "{\"request_code\":"<< END_CODE << ",\"update_info\":\"\"}";
                mtx.unlock();
                return std::shared_ptr<http_response>(new string_response(response_content.str(), 200, "application/json"));
            }

            miner_instance.statistics = stats;
            miner_instance.update_timestamp = time(NULL);

            if(miner_instance.status_code == UPDATE_CODE){
                response_content <<  "{\"request_code\":" << UPDATE_CODE << ",\"update_info\":\"" << miner_instance.update_info <<  "\"}";
                miner_instance.status_code = DEFAULT_CODE;
            }
            else{
                response_content <<  "{\"request_code\":" << DEFAULT_CODE << ",\"update_info\":\"\"}";
            }
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
NOTFOUND_METHOD_INSERTER(send_mining_statistics_resource)