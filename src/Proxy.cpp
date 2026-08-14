#include "Proxy.h"

#include "httplib.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr DWORD kProxyBufferSize = 64 * 1024;

struct WinHttpResponse {
    HINTERNET session = nullptr;
    HINTERNET connection = nullptr;
    HINTERNET request = nullptr;
    bool finished = false;

    ~WinHttpResponse() {
        if (request) WinHttpCloseHandle(request);
        if (connection) WinHttpCloseHandle(connection);
        if (session) WinHttpCloseHandle(session);
    }
};

std::wstring utf8ToWide(std::string_view value) {
    if (value.empty()) return {};
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0
    );
    if (length <= 0) return {};

    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length
    );
    return result;
}

std::string wideToUtf8(std::wstring_view value) {
    if (value.empty()) return {};
    const int length = WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr
    );
    if (length <= 0) return {};

    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr
    );
    return result;
}

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool isRequestHeaderFiltered(const std::string& lowerName) {
    return lowerName == "host" ||
           lowerName == "connection" ||
           lowerName == "content-length" ||
           lowerName == "transfer-encoding" ||
           lowerName == "keep-alive" ||
           lowerName == "proxy-authenticate" ||
           lowerName == "proxy-authorization" ||
           lowerName == "proxy-connection" ||
           lowerName == "te" ||
           lowerName == "trailer" ||
           lowerName == "upgrade" ||
           lowerName == "expect" ||
           lowerName == "accept-encoding";
}

bool isResponseHeaderFiltered(const std::string& lowerName) {
    return lowerName == "connection" ||
           lowerName == "content-length" ||
           lowerName == "content-type" ||
           lowerName == "transfer-encoding" ||
           lowerName == "keep-alive" ||
           lowerName == "proxy-authenticate" ||
           lowerName == "proxy-authorization" ||
           lowerName == "proxy-connection" ||
           lowerName == "te" ||
           lowerName == "trailer" ||
           lowerName == "upgrade" ||
           lowerName == "cross-origin-embedder-policy" ||
           lowerName == "cross-origin-opener-policy";
}

std::wstring buildForwardHeaders(const httplib::Request& request) {
    std::wstring result;
    for (const auto& [name, value] : request.headers) {
        if (isRequestHeaderFiltered(lowerAscii(name))) continue;
        if (name.find_first_of("\r\n") != std::string::npos ||
            value.find_first_of("\r\n") != std::string::npos) {
            continue;
        }
        const auto wideName = utf8ToWide(name);
        const auto wideValue = utf8ToWide(value);
        if (wideName.empty()) continue;
        result.append(wideName).append(L": ").append(wideValue).append(L"\r\n");
    }
    return result;
}

std::wstring queryHeader(HINTERNET request, DWORD query) {
    DWORD bytes = 0;
    WinHttpQueryHeaders(request, query, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &bytes, nullptr);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0) return {};

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryHeaders(
            request, query, WINHTTP_HEADER_NAME_BY_INDEX, value.data(), &bytes, nullptr
        )) {
        return {};
    }
    while (!value.empty() && value.back() == L'\0') value.pop_back();
    return value;
}

void copyResponseHeaders(HINTERNET upstream, httplib::Response& response) {
    const auto rawHeaders = queryHeader(upstream, WINHTTP_QUERY_RAW_HEADERS_CRLF);
    std::wistringstream lines(rawHeaders);
    std::wstring line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        const auto separator = line.find(L':');
        if (separator == std::wstring::npos) continue;

        auto name = wideToUtf8(std::wstring_view(line).substr(0, separator));
        auto value = wideToUtf8(std::wstring_view(line).substr(separator + 1));
        const auto firstValueCharacter = value.find_first_not_of(" \t");
        value = firstValueCharacter == std::string::npos
            ? std::string()
            : value.substr(firstValueCharacter);

        if (name.empty() || isResponseHeaderFiltered(lowerAscii(name))) continue;
        response.set_header(name, value);
    }
}

void sendProxyError(httplib::Response& response, DWORD error) {
    response.status = 502;
    response.set_content(
        "{\"code\":502,\"message\":\"Desktop proxy could not reach the CAD backend\","
        "\"data\":{\"winHttpError\":" + std::to_string(error) + "}}",
        "application/json; charset=utf-8"
    );
}

bool prepareUpstreamRequest(
    const httplib::Request& request,
    const ApiProxyConfig& config,
    const std::shared_ptr<WinHttpResponse>& upstream,
    DWORD& error
) {
    upstream->session = WinHttpOpen(
        L"InnoCADDesktop/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!upstream->session) {
        error = GetLastError();
        return false;
    }

    WinHttpSetTimeouts(upstream->session, 15'000, 30'000, 60'000, 10 * 60'000);

    upstream->connection = WinHttpConnect(
        upstream->session,
        config.host.c_str(),
        static_cast<INTERNET_PORT>(config.port),
        0
    );
    if (!upstream->connection) {
        error = GetLastError();
        return false;
    }

    const auto method = utf8ToWide(request.method);
    const auto target = utf8ToWide(request.target.empty() ? request.path : request.target);
    upstream->request = WinHttpOpenRequest(
        upstream->connection,
        method.c_str(),
        target.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        config.secure ? WINHTTP_FLAG_SECURE : 0
    );
    if (!upstream->request) {
        error = GetLastError();
        return false;
    }

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(
            upstream->request,
            WINHTTP_OPTION_REDIRECT_POLICY,
            &redirectPolicy,
            sizeof(redirectPolicy)
        )) {
        error = GetLastError();
        return false;
    }

    if (request.body.size() > std::numeric_limits<DWORD>::max()) {
        error = ERROR_FILE_TOO_LARGE;
        return false;
    }

    const auto headers = buildForwardHeaders(request);
    const DWORD bodyLength = static_cast<DWORD>(request.body.size());
    void* body = bodyLength == 0
        ? WINHTTP_NO_REQUEST_DATA
        : const_cast<char*>(request.body.data());

    if (!WinHttpSendRequest(
            upstream->request,
            headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
            headers.empty() ? 0 : static_cast<DWORD>(-1L),
            body,
            bodyLength,
            bodyLength,
            0
        )) {
        error = GetLastError();
        return false;
    }

    if (!WinHttpReceiveResponse(upstream->request, nullptr)) {
        error = GetLastError();
        return false;
    }
    return true;
}

void proxyRequest(
    const httplib::Request& request,
    httplib::Response& response,
    const ApiProxyConfig& config
) {
    auto upstream = std::make_shared<WinHttpResponse>();
    DWORD error = ERROR_SUCCESS;
    if (!prepareUpstreamRequest(request, config, upstream, error)) {
        sendProxyError(response, error);
        return;
    }

    DWORD status = 502;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(
            upstream->request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX
        )) {
        sendProxyError(response, GetLastError());
        return;
    }

    response.status = static_cast<int>(status);
    copyResponseHeaders(upstream->request, response);

    auto contentType = wideToUtf8(queryHeader(upstream->request, WINHTTP_QUERY_CONTENT_TYPE));
    if (contentType.empty()) contentType = "application/octet-stream";

    if (request.method == "HEAD") {
        response.set_content("", contentType);
        return;
    }

    response.set_chunked_content_provider(
        contentType,
        [upstream](size_t, httplib::DataSink& sink) {
            if (upstream->finished) {
                sink.done();
                return true;
            }
            if (sink.is_writable && !sink.is_writable()) return false;

            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(upstream->request, &available)) return false;
            if (available == 0) {
                upstream->finished = true;
                sink.done();
                return true;
            }

            std::vector<char> buffer(std::min(available, kProxyBufferSize));
            DWORD bytesRead = 0;
            if (!WinHttpReadData(
                    upstream->request,
                    buffer.data(),
                    static_cast<DWORD>(buffer.size()),
                    &bytesRead
                )) {
                return false;
            }
            if (bytesRead == 0) {
                upstream->finished = true;
                sink.done();
                return true;
            }
            return sink.write(buffer.data(), bytesRead);
        },
        [upstream](bool) {}
    );
}

} // namespace

void registerApiProxy(httplib::Server& server, const ApiProxyConfig& config) {
    const std::string pattern = R"(/api(?:/.*)?)";
    const auto handler = [config](const httplib::Request& request, httplib::Response& response) {
        proxyRequest(request, response, config);
    };

    server.Get(pattern, handler);
    server.Post(pattern, handler);
    server.Put(pattern, handler);
    server.Patch(pattern, handler);
    server.Delete(pattern, handler);
    server.Options(pattern, handler);
}
