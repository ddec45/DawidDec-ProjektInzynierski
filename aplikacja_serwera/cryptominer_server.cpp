#include "cryptominer_server.hpp"
#include "httpserver.hpp"
using namespace httpserver;
#include "simdjson.h"
using namespace simdjson;

#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#define ERROR_MESSAGE_STRING(message) ""

//std::unique_ptr<std::string> config_file_content_ptr;

ondemand::parser config_file_parser;
std::string config_file_content;
ondemand::document config_file;

std::unordered_map<int, miner_instance_info> miner_instance_info_map;
std::unordered_map<int, miner_application_info> miner_application_info_map;
config_file_content_class config_file_content_object;

std::mutex mtx;

int main(int argc, char** argv) {
    puts("Initializing cryptominer server...");
    //try{
        std::fstream config_file_stream;
        config_file_stream.open("config.json", std::ios_base::in);

        config_file_stream.seekg (0, config_file_stream.end);
        int config_file_length = config_file_stream.tellg();
        config_file_stream.seekg (0, config_file_stream.beg);

        char* buffer = new char[config_file_length];
        config_file_stream.read(buffer, config_file_length);
        config_file_content = buffer;
        delete buffer;
        std::cout << "Config file content:\n" << config_file_content;

        std::string json(config_file_content);
        simdjson::padded_string padded_json(json);
        config_file = config_file_parser.iterate(padded_json);

        config_file_content_object.user_api_key = config_file["user_api_key"].get_string().value();
        config_file_content_object.admin_api_key = config_file["admin_api_key"].get_string().value();
        config_file_content_object.ssl_certificate = config_file["ssl_certificate"].get_string().value();
        config_file_content_object.ssl_key = config_file["ssl_key"].get_string().value();

        config_file_content_object.port = config_file["port"].get_uint64();
        config_file_content_object.max_nr_of_miner_instances = config_file["max_nr_of_miner_instances"].get_uint64();
        config_file_content_object.instance_statistics_length = config_file["instance_statistics_length"].get_uint64();
        config_file_content_object.update_period = config_file["update_period"].get_int64();

        auto miner_app_array = config_file["miner_applications"].get_array();
        int miner_app_array_length = miner_app_array.count_elements();

        for(ondemand::object miner_app: miner_app_array) {
            miner_application_info obj;
            auto sv = miner_app["filename"].get_string().value();
            obj.filename = std::string(sv.begin(), sv.end());
            obj.id = miner_app["id"].get_int64();
            sv = miner_app["name"].get_string();
            obj.name = std::string(sv.begin(), sv.end());
            sv = miner_app["description"].get_string();
            obj.description = std::string(sv.begin(), sv.end());
            int key = obj.id;
            miner_application_info_map.insert({obj.id, obj});
            //config_file_content_object.miner_applications.insert({obj.id, obj});
        }
        //int max_nr_of_miner_instances = config_file["max_nr_of_miner_instances"].get_int64();


        //std::cout << max_nr_of_miner_instances << std::endl;
    // }
    // catch(...){
    //     puts("Incorrect config file!");
    //     exit(-1);
    // }

    webserver ws = create_webserver(config_file_content_object.port)
        .use_ssl()
        .https_mem_key("key.pem")
        .https_mem_cert("cert.pem")
        .debug();
    hello_world_resource hwr;
    json_resource jr;
    miner_application_list_resource malr;
    miner_application_get_resource magr;
    miner_instance_start_resource misr;
    miner_instance_list_resource milr;
    miner_instance_statistics_list_resource mislr;
    miner_instance_statistics_get_resource misgr;
    miner_instance_update_resource miur;
    miner_instance_delete_resource midr;
    send_mining_statistics_resource smsr;
    ws.register_resource("/hello", &hwr);
    ws.register_resource("/json", &jr);
    ws.register_resource("/user/miner/application/list", &malr);
    ws.register_resource("/user/miner/application/{miner_app_id}", &magr);
    ws.register_resource("/user/miner/instance/start/{miner_app_id}", &misr);
    ws.register_resource("/user/miner/instance/list", &milr);
    ws.register_resource("/user/miner/instance/statistics/list", &mislr);
    ws.register_resource("/user/miner/instance/statistics/{miner_instance_id}", &misgr);
    ws.register_resource("/user/miner/instance/update/{miner_instance_id}", &miur);
    ws.register_resource("/user/miner/instance/delete/{miner_instance_id}", &midr);
    ws.register_resource("/admin/mining_statistics/send/{miner_instance_id}", &smsr);
    
    ws.start(false);
    puts("Cryptominer server has started.");

    while(ws.is_running()){
        sleep(5);

        int current_time = time(NULL);
        mtx.lock();
        try{
            for (auto it = miner_instance_info_map.cbegin(); it != miner_instance_info_map.cend();)
            {
                if(current_time > it->second.update_timestamp + config_file_content_object.update_period){
                    ERROR_CHECK((kill(it->second.process_id, SIGKILL) == -1), "kill")
                    std::cout << "Deleted miner instance " << it->second.id << "." << std::endl;
                    miner_instance_info_map.erase(it++);
                }
                else{
                    ++it;
                }
            }
        }
        catch(...){
            //mtx.unlock();
        }
        mtx.unlock();
    }
    
    return 0;
}