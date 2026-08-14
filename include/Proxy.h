#ifndef CAD2APP_PROXY_H
#define CAD2APP_PROXY_H

#include <string>

namespace httplib {
class Server;
}

struct ApiProxyConfig {
    std::wstring host;
    unsigned short port;
    bool secure;
};

void registerApiProxy(httplib::Server& server, const ApiProxyConfig& config);

#endif
