#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <thread>
#include "misc_utils.h"
#include "records.hpp"
#include "html.hpp"
#include "http.hpp"

using boost::asio::ip::tcp;

//global data bases
dest_map_t g_destinations_db;
records_t g_records_db;

using namespace std;

//connectino handle / session worker function
// we got here with already connected socket
void session(tcp::socket sock)
{
	try
	{
		boost::system::error_code ignored_error;
		boost::asio::streambuf sb;

		// read from socket inf
		for (;;)
		{
			//we interested only in first line
			std::size_t line_length = boost::asio::read_until(sock, sb, "\r\n", ignored_error);

			//until we got zero bytes
			if (line_length == 0)
				break;

			boost::asio::streambuf::const_buffers_type bufs = sb.data();
			std::string starting_line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + line_length - 2);
			sb.consume(sb.size()); //clean buffer (up to the end of http request message)

			cout << "\n\n>>>Request: " << starting_line << endl;

			//responce ostream
			std::ostream reply_os(&sb);
			size_t reply_size = 0;

			std::string path;
			query_dict_t query;

			//process http request
			if (validate_http_request(starting_line, path, query, reply_os, reply_size))
			{
				serve_http_request(path, query, reply_os, reply_size);
			}

			if (reply_size == 0)
			{
				// create dummy reponse
				std::stringstream ss;
				ss << "Empty responce";
				reply_size = create_http_responce(reply_os, *ss.rdbuf(), 500);
			}

			cout << "\n<<<Responce size " << reply_size << endl;
			//send
			boost::asio::write(sock, sb.data(), boost::asio::transfer_all(), ignored_error);
			sb.consume(reply_size);
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}

//basic server function which listens on specific port and accepts connection
// then spawn new worker(fork or thread) and handles the connection
void server(boost::asio::io_service &io_service, unsigned short port, bool use_fork = true)
{
	cout << "server startet, serving on port: " << port << ", using " << ((use_fork) ? "fork" : "threads") << " for connection/sessions processing" << endl;
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		tcp::socket sock(io_service);
		acceptor.accept(sock);
		std::string client_ip = sock.remote_endpoint().address().to_string();
		unsigned short client_port = sock.remote_endpoint().port();
		cout << "accepted connection from: " << client_ip << ":" << client_port << endl;
		if (use_fork)
		{
			io_service.notify_fork(boost::asio::io_service::fork_prepare);
			int pid = fork();
			if (pid == 0)
			{
				io_service.notify_fork(boost::asio::io_service::fork_child);
				session(std::move(sock));
			}
			else
			{
				cout << "forked to pid: " << pid << endl;
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
			std::cerr << "Usage: rubicon_test_task <port> [fork?]\n";
			return 1;
		}

		g_destinations_db["A"] = "localhost:12301/restorans";
		g_destinations_db["B"] = "localhost:12302/restorans";
		g_destinations_db["C"] = "localhost:12303/restorans";

		g_records_db["Italian"] = {"Italian", "Italian rest 1"};
		g_records_db["Itolian"] = {"Itolian", "Itolian rest 2"};
		g_records_db["Itulian"] = {"Itulian", "Itulian rest 3"};
		g_records_db["Itilian"] = {"Itilian", "Itilian rest 4"};

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