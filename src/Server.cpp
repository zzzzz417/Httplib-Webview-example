#include"Server.h"
#include"Sitem.h"
#include "nlohmann/json.hpp"
bool loadConfig(std::vector<ScheduleItem>& out)
{
    std::ifstream file("../data/data.json");
    if (!file) return false;

    nlohmann::json j;
    file >> j;

    for (auto& item : j["list"])
    {
        ScheduleItem s;
        s.sID       = item["sID"];
        s.status    = item["status"];
        s.state     = item["state"];
        s.ocName    = item["ocName"];
        s.beginTime = item["beginTime"];
        s.stayTime  = item["stayTime"];
        s.remark    = item["remark"];

        out.push_back(s);
    }
    return true;
}

bool saveConfig(const std::vector<ScheduleItem>& data)
{
    nlohmann::json j;
    nlohmann::json arr = nlohmann::json::array();

    for (auto& s : data) {
        arr.push_back({
            {"sID", s.sID},
            {"status", s.status},
            {"state", s.state},
            {"ocName", s.ocName},
            {"beginTime", s.beginTime},
            {"stayTime", s.stayTime},
            {"remark", s.remark}
        });
    }

    j["list"] = arr;
    std::ofstream file("../data/data.json");
    file << j.dump(4);
    file.close();
    return true;
}

void ApiHandlers::getData(const httplib::Request& request, httplib::Response& response)
{
    std::vector<ScheduleItem> sc;
    loadConfig(sc);
    nlohmann::json j;
    nlohmann::json arr = nlohmann::json::array();
    for (auto& s:sc){
        arr.push_back({
            {"sID", s.sID},
            {"status", s.status},
            {"state", s.state},
            {"ocName", s.ocName},
            {"beginTime", s.beginTime},
            {"stayTime", s.stayTime},
            {"remark", s.remark}
        });
    }
    
    j["list"] = arr;
    response.set_content(j.dump(),"application/json");    
}
void ApiHandlers::upData(const httplib::Request& request, httplib::Response& response){
    nlohmann::json reqJson = nlohmann::json::parse(request.body);
    std::vector<ScheduleItem> sc;
    loadConfig(sc);
    
    int sID = reqJson["sID"];
    for (auto& item : sc) {
        if (item.sID == sID) {
            item.status = reqJson["status"];
            item.state  = reqJson["state"];
            item.remark = reqJson["remark"];
            break;
        }
    }
    
    saveConfig(sc);
    response.set_content("{\"result\":\"ok\"}", "application/json");
}
void ApiHandlers::addData(const httplib::Request& request, httplib::Response& response){


    // 解析前端发来的新日程
    nlohmann::json reqJson = nlohmann::json::parse(request.body);
    std::vector<ScheduleItem> sc;
    loadConfig(sc);
    
    // 创建新日程
    ScheduleItem newItem;
    newItem.sID       = sc.size(); // 自动按序号生成ID
    newItem.status    = reqJson["status"];
    newItem.state     = reqJson["state"];
    newItem.ocName    = reqJson["ocName"];
    newItem.beginTime = reqJson["beginTime"];
    newItem.stayTime  = reqJson["stayTime"];
    newItem.remark    = reqJson["remark"];
    
    // 加入列表并保存
    sc.push_back(newItem);
    saveConfig(sc);
    response.set_content("{\"result\":\"ok\"}", "application/json"); 
}
