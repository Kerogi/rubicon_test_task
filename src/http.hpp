#pragma once
#include <string>
#include <map>
#include <curl/curl.h>
#include <functional>

extern std::map<int, std::string> code_names;

//the dictionary of query key value pairs (more than 1 value per key is possible)
using query_dict_t = std::multimap<std::string,std::string>;

//basic request handler signature
// consumes a path string and query dict of request
// should write to specified std::ostream a responce body
// return a http responce code
using request_handler_t = std::function<int(const std::string&, const query_dict_t&, std::ostream&)>;

//utility function
std::string unescape( CURL *curl, const std::string& url);
std::string escape( CURL *curl, const std::string& url);

//validate request start line
// if ok returs true
// if not return false and writes formatted (http+html) error message to reply_os stream
bool validate_http_request(const std::string& starting_line, std::string& out_request_path, query_dict_t& out_request_query, std::ostream& reply_os, size_t &reply_size);

//servers request
// if ok returs true, and writes formatted (http+html) response message to reply_os stream
// if not return false and writes formatted (http+html) error message to reply_os stream
bool serve_http_request(const std::string& request_path, const query_dict_t& request_query, std::ostream& reply_os, size_t &reply_size);

//format http responce message based on reply code
// and appends body 
// return size of the response
size_t create_http_responce(std::ostream& reply_os, const std::stringbuf& http_body, int code = 200, bool json = false);