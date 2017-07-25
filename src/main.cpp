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

#include <string>
#include <set>
#include "records.hpp"
#include "html.hpp"
#include "http.hpp"



using boost::asio::ip::tcp;
const int max_length = 1024;


dest_map_t global_dest_map;
records_t global_records;

using namespace std;




void session(tcp::socket sock)
{
	cout << "session in sock(" << (int)sock.native_handle() << ")" << endl;
	try
	{

		boost::system::error_code ignored_error;

		for (;;)
		{
			boost::asio::streambuf sb;
			std::size_t line_length = boost::asio::read_until(sock, sb, "\r\n", ignored_error);
			if (line_length == 0)
				break;

			boost::asio::streambuf::const_buffers_type bufs = sb.data();
			std::string starting_line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + line_length - 2);
			sb.consume(sb.size()); //clenan buffer (up to the end of http message)
			cout<<"\n\n>>>Request |"<<starting_line.length()<<"| [\n"<<starting_line<<"\n>>>Request]\n\n"<<endl;
			std::ostream   reply_os(&sb);
			std::streampos reply_before = reply_os.tellp();

			std::string 	path;
			query_dict_t 	query;

			if(validate_http_request(starting_line, path, query, reply_os)) 
			{
				serve_http_request(path, query, reply_os);
			}

			int reply_size = reply_os.tellp() - reply_before;
			if( reply_size == 0 ){
				std::stringstream ss;
				ss <<"Empty responce";
				reply_size = create_http_responce(reply_os, *ss.rdbuf(), 500);
			}

			cout<<"\n\n<<<Reply |"<<reply_size<<"| [\n"<<std::string(boost::asio::buffers_begin(sb.data()), boost::asio::buffers_begin(sb.data())+ reply_size)<<"\n<<<Reply]\n\n"<<endl;
			boost::asio::write(sock, sb.data(), boost::asio::transfer_all(), ignored_error);
			sb.consume(reply_size);
		}

	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}

void server(boost::asio::io_service &io_service, unsigned short port, bool use_fork = true)
{
	cout << "server startet, serving on port: " << port << ", using " << ((use_fork) ? "fork" : "threads") << " for mutitasking" << endl;
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		tcp::socket sock(io_service);
		//cout << "creating new socket(" << (int)sock.native_handle() << ")" << endl;
		//cout << "waiting for connection" << endl;
		acceptor.accept(sock);
		//cout << "accepted connection on socket(" << (int)sock.native_handle() << "), spawning new task for session" << endl;
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
		} else {
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

		global_dest_map["A"] = "localhost:12301/restorans";
		global_dest_map["B"] = "localhost:12302/restorans";
		global_dest_map["C"] = "localhost:12303/restorans";

		global_records["Italian"] = {"Italian", "Italian rest 1"};
		global_records["Itolian"] = {"Itolian", "Itolian rest 2"};
		global_records["Itulian"] = {"Itulian", "Itulian rest 3"};
		global_records["Itilian"] = {"Itilian", "Itilian rest 4"};

		boost::asio::io_service io_service;
		cout << "staring server " << argc << endl;

		server(io_service, std::atoi(argv[1]), !(argc > 2));
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}