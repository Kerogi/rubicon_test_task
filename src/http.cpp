#include <vector>
#include "http.hpp"
#include "misc_utils.h"

uri_parts_t extract_uri_parts(const std::string &uri)
{
    if (starting_line.length() <= std::string("GET / HTTP/1.1").length())
        return query_dict_t();
    auto cmp_res = starting_line.compare(0, 4, "GET ");
    if (cmp_res != 0)
        return uri_parts_t();

    size_t query_part_start = starting_line.find_first_of('?', 4);
    size_t url_end = starting_line.find("HTTP/", 4);

    if (query_part_start >= url_end)
        return uri_parts_t();
    std::string url_query(starting_line.substr(query_part_start + 1, url_end - query_part_start - 2));

    query_dict_t ret;
    std::vector<std::string> querys;
    split(querys, url_query, "&");
    for (auto &query_str : querys)
    {
        std::vector<std::string> query;
        split(query, query_str, "=");
        if (query.size() > 1)
            ret.insert(std::make_pair(query[0], query[1]));
    }
    return ret;
}

std::string unescape(CURL *curl, const std::string &url)
{
    std::string ret;
    if (curl)
    {
        int new_length;
        char *new_string = curl_easy_unescape(curl, url.c_str(), url.length(), &new_length);
        if (new_length > 0)
        {
            ret.assign(new_string, new_length);
        }
        curl_free(new_string);
    }
    return ret;
}

std::string escape(CURL *curl, const std::string &url)
{
    std::string ret;
    if (curl)
    {
        char *new_string = curl_easy_escape(curl, url.c_str(), url.length());
        ret.assign(new_string);
        curl_free(new_string);
    }
    return ret;
}


size_t http_responce(std::ostream& os, std::istream& body_is, int code = 200) {
    std::stringstream ss_http;
    ss_http << "HTTP/1.1 " << code << "\r\n";
    ss_http << "Content-Length: " << http_body.rdbuf().str().length() << "\r\n";
    ss_http << "Connection: close \r\n";
    ss_http << "Content-Type: text/html \r\n";
    ss_http << "\r\n";
    ss_http << http_body.rdbuf().str();
    os << ss_http.str();
    return ss_http.str().length();
}