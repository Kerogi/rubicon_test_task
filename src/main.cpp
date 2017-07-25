#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <map>
#include <curl/curl.h>
#include "misc_utils.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <string>
#include <set>
#include "records.hpp"
#include "html.hpp"
#include "http.hpp"

namespace pt = boost::property_tree;

using boost::asio::ip::tcp;
const int max_length = 1024;

using dest_map_t = std::map<std::string, std::string>;

dest_map_t global_dest_map;
static records_t global_records;

using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  std::stringbuf *pbuf = (std::stringbuf *)userp;
  if (realsize)
    pbuf->sputn((char *)buffer, realsize);
  return realsize;
}

void pull_one_url(CURL *curl, const std::string &url, query_results_t &dst_res)
{
  if (curl)
  {
    CURLcode res;
    std::stringbuf buf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    res = curl_easy_perform(curl);
    cout << "curl_easy_perform finish: " << res << endl;
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
          ;
        }
      }
    }
  }
}

void session(tcp::socket sock)
{
  cout << "session in sock(" << (int)sock.native_handle() << ")" << endl;
  try
  {
    boost::asio::streambuf sb;

    for (;;)
    {

      boost::system::error_code ignored_error;
      std::size_t line_length = boost::asio::read_until(sock, sb, "\r\n", ignored_error);
      if (line_length == 0)
        break;

      boost::asio::streambuf::const_buffers_type bufs = sb.data();
      std::string starting_line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + line_length - 2);
      sb.consume(sb.size()); //clenam buffer (up to the end of http message)

      std::ostream reply_os(&sb);
      if(starting_line <= std::string("GET / HTTP/1.1").length()) {
        auto html_buf = wrap_to_stringbuf(html_not_implemented()) ;
        reply_size = http_responce(reply_os,html_buf, 503);
      }
      
      auto q = extract_query_from_url(url_line);
      auto q_query = q.find("query");
      auto q_dest = q.find("dest");

      size_t reply_size = 0;
      if (q_query == q.end())
      {
        reply_size = format_forbiden(reply_os);
      }
      else
      {
        query_results_t session_records;
        CURL *curl;
        curl = curl_easy_init();
        if (q_dest != q.end())
        {
          std::vector<std::string> destinations;
          split(destinations, q_dest->second, ",");
          cout << LBL(destinations) << endl;
          for (const auto &dest_label : destinations)
          {
            dest_map_t::iterator it = global_dest_map.find(dest_label);
            if (it != global_dest_map.end())
            {
              std::string new_url = it->second + "/rest.html?query=" + q_query->second;
              cout << LBL(new_url) << endl;
              pull_one_url(curl, new_url, session_records);
            }
          }
        }
        else
        {
          session_records = query_records(unescape(curl, q_query->second), global_records);
        }

        if (session_records.found_records.empty())
        {
          reply_size = format_notfound_query(reply_os, unescape(curl, q_query->second));
        }
        else
        {
          reply_size = format_query_results(reply_os, session_records);
        }

        curl_easy_cleanup(curl);
      }
      cout << LBL(reply_size) << endl;
      boost::asio::write(sock, sb.data(), boost::asio::transfer_all(), ignored_error);
      sb.consume(reply_size);
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_service &io_service, unsigned short port, bool use_fork = true)
{
  cout << "server startet, serving on port: " << port << ", using " << ((use_fork) ? "fork" : "threads") << " for mutitasking" << endl;
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    tcp::socket sock(io_service);
    cout << "creating new socket(" << (int)sock.native_handle() << ")" << endl;
    cout << "waiting for connection" << endl;
    acceptor.accept(sock);
    cout << "accepted connection on socket(" << (int)sock.native_handle() << "), spawning new task for session" << endl;
    if (use_fork)
    {
      io_service.notify_fork(boost::asio::io_service::fork_prepare);
      if (fork() == 0)
      {
        io_service.notify_fork(boost::asio::io_service::fork_child);
        session(std::move(sock));
      }
      else
      {
        io_service.notify_fork(boost::asio::io_service::fork_parent);
      }
    }
    else
    {
      std::thread(session, std::move(sock)).detach();
    }
  }
}

int main(int argc, char *argv[])
{
  try
  {
    if (argc <= 1)
    {
      std::cerr << "Usage: blocking_tcp_echo_server <port>\n";
      return 1;
    }

    global_dest_map["A"] = "localhost:12301";
    global_dest_map["B"] = "localhost:12302";
    global_dest_map["C"] = "localhost:12303";

    global_records["Italian"] = {"Italian", "Italian rest 1"};
    global_records["Itolian"] = {"Itolian", "Itolian rest 2"};
    global_records["Itulian"] = {"Itulian", "Itulian rest 3"};
    global_records["Itilian"] = {"Itilian", "Itilian rest 4"};

    boost::asio::io_service io_service;
    cout << "staring server " << argc << endl;

    server(io_service, std::atoi(argv[1]), !argc > 2);
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}