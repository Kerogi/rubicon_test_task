#include "mt.hpp"
#include <boost/asio/io_service.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <curl/curl.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "http.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

//callback to proccess incoming bytes for curl
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::stringbuf *pbuf = (std::stringbuf *)userp;
	if (realsize)
		pbuf->sputn((char *)buffer, realsize);
	return realsize;
}

//fetch result from peer server
std::pair<int, query_results_t> query_peer(CURL *curl, const std::string &url, const std::string &query_string )
{
	query_results_t dst_res {query_string, {} };
	std::shared_ptr<CURL> local_curl;
	int response_code = -1;
	if (nullptr == curl) {
		local_curl.reset(curl_easy_init(), [] (CURL* pcurl) { curl_easy_cleanup(pcurl); });
		curl = local_curl.get();
	}
	if (curl)
	{
		CURLcode res;
		std::stringbuf buf;
		std::string new_url = url + "?query=" + escape(curl, query_string) + "&json";
		curl_easy_setopt(curl, CURLOPT_URL, new_url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
		res = curl_easy_perform(curl);
		if (res == CURLE_OK)
		{
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == 200)
			{
				//seem ok
				//try to process the data
			    try{
					boost::property_tree::ptree tree;
					std::istream sbuf(&buf);
					boost::property_tree::read_json(sbuf, tree);

					for(auto& record_kv: tree.get_child("query_results.found_records")) {
						if(record_kv.first == "record")
							dst_res.found_records.push_back(std::move(record_from_ptree(record_kv.second)));
					}
				} catch(boost::property_tree::ptree_error const&  ex) {
					
				}
			}
		}
	}
	return { response_code, dst_res };
}

query_results_t do_the_parallel_query(const std::vector<std::string>& urls, const std::string& query_string)
{
    query_results_t result {query_string, {}};
	//setup ioservice
    boost::asio::io_service ios;
	boost::asio::io_service::work work(ios);

	//setup thread pool
	size_t max_concrent_threads = 4;
	std::vector<std::thread> pool;
	auto thread_func = [&ios] (){ios.run();};
	for(size_t i =0; i < std::min(max_concrent_threads, urls.size()); ++i){
		pool.push_back(std::move(std::thread(thread_func)));
	}

	//setup sync vars
	size_t 					job_start_counter = 0;
	std::vector<int>        job_results;
	std::condition_variable cv_job_done;
	std::mutex				m_results;
	bool					notified = false;

	//worker func
	// which simply fetch result from peer
	// and synchronously update the aggregated result, then wakeups the waiting(main) thread
    auto job_func = [&] (const std::string url, const std::string query_string) {
        auto peer_result = query_peer(NULL, url, query_string);
        std::unique_lock<std::mutex> lock(m_results);
        result.found_records.insert(result.found_records.end(), peer_result.second.found_records.begin(), peer_result.second.found_records.end());
        notified = true;
        job_results.push_back(peer_result.first);
        cv_job_done.notify_one();
    };


	//enqueue jobs to pool
	for (const auto &url : urls)
	{
	    ++job_start_counter;
		std::cout<<"sent "<<job_start_counter<<" of "<<urls.size()<<" requests: "<<url<<", "<<query_string<<std::endl;
	    ios.post(std::bind(job_func, url, query_string));
	}

	size_t rec_count = result.found_records.size();
	if(job_start_counter>0)
	{
		//wait all jobs done
		std::unique_lock<std::mutex> lock(m_results);
		// check that all done
		while(job_start_counter != job_results.size()) { 
			// still has some to do
			while(!notified) 
				cv_job_done.wait(lock); //sleep and unlock, wakeup and lock
			std::cout<<"got "<<job_results.size()<<" of "<<job_start_counter<<" results ["<<job_results.back()<<"] (+"<<result.found_records.size() - rec_count<<" records)"<<std::endl;
			rec_count=result.found_records.size();
			notified = false;
		}
		// stop pool 
		ios.stop();
		for(auto& t:pool)
			t.join(); // join pool
	} 

    return result;
}