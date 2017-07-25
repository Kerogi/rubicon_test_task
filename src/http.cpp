#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include "http.hpp"
#include "misc_utils.h"
#include "records.hpp"
#include "html.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/asio/io_service.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
std::map<int, std::string> code_names = {
  {200, "OK"},
  {501, "Not Implemented"},
  {500, "Internal Server Error"},
  {404, "Not Found"},
  {403, "Forbidden"},
  {400, "Bad Request"}
};

namespace pt = boost::property_tree;

bool extract_uri(const std::string& starting_line, std::string& url, std::ostream& responce_body_os, int& responce_code)
{
	if (responce_code != 0) return false;
	if(0 != starting_line.compare(0, 4, "GET ")) {
		responce_code =  400;
		format_html_body(responce_body_os, "Filed to extract URI", responce_code);
		return false;
	}
	size_t url_end = starting_line.find("HTTP/",4,5);
	if( std::string::npos == url_end) {
		responce_code =  400;
		format_html_body(responce_body_os, "Filed to find the end of URI", responce_code);
		return false;
	}
	std::cout <<"length: "<<starting_line.length()<<std::endl;
	std::cout <<"url_end: "<<url_end<<std::endl;


	url = starting_line.substr(4, url_end - 4 - 1 );
	return true;
}

bool  extract_uri_parts(const std::string& uri, std::string& path, query_dict_t& query, std::ostream& responce_body_os, int& responce_code ){
	if (responce_code != 0) return false;
	size_t query_part_start = uri.find_first_of('?');
	size_t fragments_part_start = uri.find_first_of('#');
	
	path = uri.substr(0, std::min(query_part_start, fragments_part_start));

	if(query_part_start != std::string::npos){
		std::string url_query = uri.substr(query_part_start+1, fragments_part_start);
		std::vector<std::string> querys;
		split(querys, url_query, "&");
		for (const auto &query_str : querys)
		{
			std::vector<std::string> key_value;
			split(key_value, query_str, "=");
			if (key_value.size() > 1)
				query.insert(std::make_pair(key_value[0], key_value[1]));
		}
	}

	return true;
}

bool validate_http_request(const std::string& starting_line, std::string& out_request_path, query_dict_t& out_request_query, std::ostream& reply_os)
{
	int responce_code = 0;
	bool res = true;
	std::stringbuf responce_body_buff;
	std::ostream   responce_body_os(&responce_body_buff);

	//some basic validations
	if(starting_line.length() <= std::string("GET / HTTP/1.1").length()) {
		responce_code = 501;
		format_html_body(responce_body_os, "Root or empty default requests is not supported", responce_code);
		res = false;
	} 
	if(res && 0 != starting_line.compare(0, 4, "GET ")){
		responce_code = 501;
		format_html_body(responce_body_os, "Method other than GET is not supported", responce_code);
		res =  false;
	} 
	//if we still ok try to extravk some stuf from requests start line
	if(res) {
		std::string url;
		res = extract_uri(starting_line, url, responce_body_os, responce_code) &&
			extract_uri_parts(url, out_request_path, out_request_query, responce_body_os, responce_code);
	} else {
		//not ok, create http responce with html error message body from prev functions
		create_http_responce(reply_os, responce_body_buff, responce_code);
	}
	return res;
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

// fuction which format basic http responce headers  according to responce code
// and then append responce body passed as parameter
// retunrs length of created responce message
size_t create_http_responce(std::ostream& reply_os, const std::stringbuf& http_body, int code) {
	std::stringstream ss_http;
	ss_http << "HTTP/1.1 " << code <<" "<<code_names[code]<< "\r\n";
	ss_http << "Content-Length: " << http_body.str().length() << "\r\n";
	ss_http << "Connection: close \r\n";
	ss_http << "Content-Type: text/html \r\n";
	ss_http << "\r\n";
	ss_http << http_body.str();
	reply_os << ss_http.str();
	return ss_http.str().length();
}


//server's simple search query 
int serve_query_request(const std::string& request_path, const query_dict_t& request_query, std::ostream& responce_body_os) {
	auto query_kv = request_query.find("query");
	if (query_kv == request_query.end())
	{
		format_html_body(responce_body_os, "Could not find query part in reques", 400);
		return 400;
	} 

	std::shared_ptr<CURL> curl(curl_easy_init(), [] (CURL* pcurl) { curl_easy_cleanup(pcurl); });
	std::string query_string = unescape(curl.get(), query_kv->second);
	auto session_records = query_records(query_string, global_records);

	format_html_query_results_body(responce_body_os, session_records);
	return 200;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::stringbuf *pbuf = (std::stringbuf *)userp;
	if (realsize)
		pbuf->sputn((char *)buffer, realsize);
	return realsize;
}

query_results_t pull_one_url(CURL *curl, const std::string &url, const std::string &query )
{
	query_results_t dst_res {query, {} };
	std::shared_ptr<CURL> local_curl;
	if (nullptr == curl) {
		local_curl.reset(curl_easy_init(), [] (CURL* pcurl) { curl_easy_cleanup(pcurl); });
		curl = local_curl.get();
	}
	if (curl)
	{
		CURLcode res;
		std::stringbuf buf;
		std::string new_url = url + "?query=" + escape(curl, query);
		curl_easy_setopt(curl, CURLOPT_URL, new_url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
		res = curl_easy_perform(curl);
		if (res == CURLE_OK)
		{
			long response_code;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == 200)
			{
				std::cout << buf.str() << std::endl;

				// Create empty property tree object
				pt::ptree tree;
				std::istream sbuf(&buf);
				// Parse the XML into the property tree.
				pt::read_xml(sbuf, tree);

				auto result_list_items = tree.get_child("html.body.ul");
				std::cout << LBL(result_list_items.size()) << std::endl;
				for (const auto &p : result_list_items)
				{
					dst_res.found_records.push_back(std::move(record_from_ptree(p.second)));
				}
			}
		}
	}
	return dst_res;
}

extern dest_map_t global_dest_map;

//server's prosy request
int serve_proxy_request(const std::string& request_path, const query_dict_t& request_query, std::ostream& responce_body_os) {
	auto query_kv = request_query.find("query");
	if (query_kv == request_query.end())
	{
		format_html_body(responce_body_os, "Could not find query part in reques", 400);
		return 400;
	} 
	auto dest_kv = request_query.find("dest");
	if (dest_kv == request_query.end())
	{
		format_html_body(responce_body_os, "Could not find destinations part in request", 400);
		return 400;
	} 

	std::shared_ptr<CURL> curl(curl_easy_init(), [] (CURL* pcurl) { curl_easy_cleanup(pcurl); });
	std::string query_string = unescape(curl.get(), query_kv->second);
	std::string destinations_string = unescape(curl.get(), dest_kv->second);
	query_results_t session_records;

	std::vector<std::string> destinations;
	split(destinations, destinations_string, ",");

	if(destinations.empty()) {
		format_html_body(responce_body_os, "Empty destination parameter", 400);
		return 400;
	}
    boost::asio::io_service ios;
	std::vector<std::thread> pool;
	boost::asio::io_service::work work(ios);
	auto thread_func = [&ios] (){ios.run();};
	for(size_t i =0; i < std::min(size_t(4),destinations.size()); ++i){
		pool.push_back(std::move(std::thread(thread_func)));
	}

	size_t 					found_destinations = 0;
	size_t					job_done_counter=0;
	std::condition_variable cond_all_done;
	std::mutex				m_results;
	bool					notified = false;
	bool					done = false;
	for (const auto &dest_label : destinations)
	{
		dest_map_t::iterator dest_it = global_dest_map.find(dest_label);
		if (dest_it != global_dest_map.end())
		{
			++found_destinations;
			ios.post([&] () {
				auto peer_result = pull_one_url(NULL, dest_it->second, query_string);
				std::unique_lock<std::mutex> lock(m_results);
				session_records.found_records.insert(session_records.found_records.end(), peer_result.found_records.begin(), peer_result.found_records.end());
				notified = true;
				++job_done_counter;
				cond_all_done.notify_one();
			});
		}
	}

	if(found_destinations>0)
	{
		std::unique_lock<std::mutex> lock(m_results);
		while(found_destinations != job_done_counter){
			while(!notified) 
				cond_all_done.wait(lock);
			std::cout<<"Got "<<job_done_counter<<" of "<<found_destinations<<" results"<<std::endl;
			notified = false;
		}
		ios.stop();
		for(auto& t:pool)
			t.join();
	} else {
		format_html_body(responce_body_os, "All of specfied destintaions is unreachable", 400);
		return 400;
	}
	 
	format_html_query_results_body(responce_body_os, session_records);
	return 200;
}

// basic http request handler
bool serve_http_request(const std::string& request_path, const query_dict_t& request_query, std::ostream& reply_os) {

	std::cout<<"request_path: "<<request_path<<std::endl;
	std::cout<<"request_query: "<<request_query<<std::endl;
	static std::map<std::string, request_handler_t> request_handlers = {
		{"/restorans", serve_query_request},
		{"/multiproxy", serve_proxy_request},
	};

	auto handler = request_handlers.find(request_path);
	std::stringbuf responce_body_buff;
	std::ostream   responce_body_os(&responce_body_buff);
	int responce_code = 0;

	if(handler != request_handlers.end()) {
		responce_code = handler->second(request_path, request_query, responce_body_os);
	} else  {
		format_html_body(responce_body_os, "Cound not find what to do with '<b>"+request_path+"</b>'", 404);
		responce_code = 404;
	}
	create_http_responce(reply_os, responce_body_buff, responce_code);
	return responce_code == 200;
}