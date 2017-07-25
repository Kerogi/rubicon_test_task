#pragma once
#include <string>
#include <map>
#include <curl/curl.h>

using query_dict_t = std::multimap<std::string,std::string>;
struct url_parts_t {
    std::string path;
    query_dict_t query;
};
url_parts_t extract_uri_parts(const std::string& uri);

std::string unescape( CURL *curl, const std::string& url);
std::string escape( CURL *curl, const std::string& url);

size_t http_responce(std::ostream& os, std::istream& body_is, int code = 200);