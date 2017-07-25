#pragma once
#include <string>
#include <map>
#include <curl/curl.h>
#include <functional>

extern std::map<int, std::string> code_names;

using query_dict_t = std::multimap<std::string,std::string>;
using request_handler_t = std::function<int(const std::string&, const query_dict_t&, std::ostream&)>;

std::string unescape( CURL *curl, const std::string& url);
std::string escape( CURL *curl, const std::string& url);

bool validate_http_request(const std::string& starting_line, std::string& out_request_path, query_dict_t& out_request_query, std::ostream& reply_os);
bool serve_http_request(const std::string& request_path, const query_dict_t& request_query, std::ostream& reply_os);
size_t create_http_responce(std::ostream& reply_os, const std::stringbuf& http_body, int code = 200);