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
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
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
query_results_t query_peer(CURL *curl, const std::string &url, const std::string &query_string )
{
	query_results_t dst_res {query_string, {} };
	std::shared_ptr<CURL> local_curl;
	if (nullptr == curl) {
		local_curl.reset(curl_easy_init(), [] (CURL* pcurl) { curl_easy_cleanup(pcurl); });
		curl = local_curl.get();
	}
	if (curl)
	{
		CURLcode res;
		std::stringbuf buf;
		std::string new_url = url + "?query=" + escape(curl, query_string);
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
				//seem ok
				//try to process the data
				pt::ptree tree;
				std::istream sbuf(&buf);

				pt::read_xml(sbuf, tree);

				auto result_list_items = tree.get_child("html.body.ul");
				for (const auto &p : result_list_items)
				{
					dst_res.found_records.push_back(std::move(record_from_ptree(p.second)));
				}
			}
		}
	}
	return dst_res;
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
	size_t					job_done_counter=0;
	std::condition_variable cv_job_done;
	std::mutex				m_results;
	bool					notified = false;

	//worker func
	// which simply fetch result from peer
	// and synchronously update the aggregated result, then wakeups the waiting(main) thread
    auto job_func = [&] (const std::string url, const std::string query_string) {
        auto peer_result = query_peer(NULL, url, query_string);
        std::unique_lock<std::mutex> lock(m_results);
        result.found_records.insert(result.found_records.end(), peer_result.found_records.begin(), peer_result.found_records.end());
        notified = true;
        ++job_done_counter;
        cv_job_done.notify_one();
    };


	//enqueue jobs to pool
	for (const auto &url : urls)
	{
	    ++job_start_counter;
	    ios.post(std::bind(job_func, url, query_string));
	}


	if(job_start_counter>0)
	{
		//wait all jobs done
		std::unique_lock<std::mutex> lock(m_results);
		// check that all done
		while(job_start_counter != job_done_counter) { 
			// still has some to do
			while(!notified) 
				cv_job_done.wait(lock); //sleep
			std::cout<<"Got "<<job_done_counter<<" of "<<job_start_counter<<" results"<<std::endl;
			notified = false;
		}
		// stop pool 
		ios.stop();
		for(auto& t:pool)
			t.join(); // join pool
	} 

    return result;
}