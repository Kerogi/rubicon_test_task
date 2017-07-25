#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <thread>
#include "misc_utils.h"
#include "records.hpp"
#include "html.hpp"
#include "http.hpp"
#include "settings.hpp"

using boost::asio::ip::tcp;

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
void server(boost::asio::io_service &io_service, unsigned short port, bool use_fork = false)
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
		if (argc < 3)
		{
			std::cerr << "Usage: rubicon_test_task <port> <data.xml> [peer.xml] [-fork]" << endl;
			return 1;
		}

		cout << "loadind data" << endl;
		std::string data_filename(argv[2]);
		if(!load_records(data_filename, g_records_db)) {
			std::cerr << "Failed to load peers list from: "<<data_filename<< endl;
			return 1;
		}
		cout << "loaded "<<g_records_db.size()<<" data records" << endl;
		if(argc > 3) {
			cout << "loadind peer list" << endl;
			std::string peers_filename(argv[3]);
			if(!load_destination(peers_filename, g_destinations_db)) {
				std::cerr << "Failed to load peers list from: "<<peers_filename<< endl;
				std::cerr << "Multiproxy feature will not work properly"<< endl;
			}
			cout << "loaded "<<g_destinations_db.size()<<" peer" << endl;
		}

		boost::asio::io_service io_service;
		cout << "staring server " << endl;
		server(io_service, std::atoi(argv[1]), (argc > 4));
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}