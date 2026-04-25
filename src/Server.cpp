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
void ApiHandlers::upData(const httplib::Request& request, httplib::Response& response) {
    try {
        nlohmann::json reqJson = nlohmann::json::parse(request.body);
        std::vector<ScheduleItem> sc;
        loadConfig(sc);
        
        int sID = reqJson["sID"].get<int>();
        bool found = false;

        for (auto& item : sc) {
            if (item.sID == sID) {
                // 1. 基础状态更新
                item.status    = reqJson["status"];
                item.state     = reqJson["state"];
                item.remark    = reqJson.value("remark", ""); // 使用 value 防止字段缺失报错

                // 2. 核心内容更新 (新增部分)
                // 只有当编辑弹窗修改了这些内容，前端才会传过来
                if (reqJson.contains("ocName"))    item.ocName    = reqJson["ocName"];
                if (reqJson.contains("beginTime")) item.beginTime = reqJson["beginTime"];
                if (reqJson.contains("stayTime"))  item.stayTime  = reqJson["stayTime"];
                
                found = true;
                break;
            }
        }
        
        if (found) {
            saveConfig(sc);
            response.set_content("{\"result\":\"ok\"}", "application/json");
        } else {
            response.status = 404;
            response.set_content("{\"result\":\"error\", \"message\":\"Task not found\"}", "application/json");
        }
    } catch (const std::exception& e) {
        response.status = 400;
        response.set_content("{\"result\":\"error\", \"message\":\"Invalid JSON\"}", "application/json");
    }
}
void ApiHandlers::addData(const httplib::Request& request, httplib::Response& response){


    // 解析前端发来的新日程
    nlohmann::json reqJson = nlohmann::json::parse(request.body);
    std::vector<ScheduleItem> sc;
    loadConfig(sc);
    
    int newID = 1;
    if (!sc.empty()) {
        int maxID = 0;
        for (const auto& item : sc) {
            if (item.sID > maxID) {
                maxID = item.sID;
            }
        }
        newID = maxID + 1;
    }
    // 创建新日程
    ScheduleItem newItem;
    newItem.sID       = newID; // 自动按序号生成ID
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
void ApiHandlers::delData(const httplib::Request& request, httplib::Response& response){
try {
        // 1. 解析请求体中的 JSON
        nlohmann::json reqJson = nlohmann::json::parse(request.body);
        
        if (!reqJson.contains("sID")) {
            response.status = 400;
            response.set_content("{\"result\":\"error\", \"message\":\"Missing sID in body\"}", "application/json");
            return;
        }

        int targetSID = reqJson["sID"];
        
        // 2. 加载数据
        std::vector<ScheduleItem> sc;
        loadConfig(sc);
        
        // 3. 执行删除逻辑
        auto it = std::remove_if(sc.begin(), sc.end(), [targetSID](const ScheduleItem& item) {
            return item.sID == targetSID;
        });

        if (it != sc.end()) {
            sc.erase(it, sc.end());
            saveConfig(sc); // 持久化
            response.set_content("{\"result\":\"ok\"}", "application/json");
        } else {
            response.status = 404;
            response.set_content("{\"result\":\"error\", \"message\":\"Task not found\"}", "application/json");
        }

    } catch (const std::exception& e) {
        response.status = 400;
        response.set_content("{\"result\":\"error\", \"message\":\"Invalid JSON format\"}", "application/json");
    }    
}