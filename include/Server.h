#ifndef SERVER_H
#define SERVER_H
#include "httplib.h"
namespace ApiHandlers {
    void getData(const httplib::Request& request, httplib::Response& response);
    void upData(const httplib::Request& request, httplib::Response& response);
    void addData(const httplib::Request& request, httplib::Response& response);
}
#endif