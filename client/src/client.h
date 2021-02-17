#ifndef ARCHIVE_SERVER_CLIENT_H
#define ARCHIVE_SERVER_CLIENT_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;
extern string host;
extern string port;

string sendFileWithCommand(tcp::socket& socket, const string& user, const string &file, const string& command);
bool getFileWithCommand(tcp::socket& socket, const string& user, const string &file);
string zip(const string& user, const string &file);
string unzip(const string& user, const string &file);
bool get(const string& user, const string& file);
bool zip_and_get(string& user, string& file);
bool unzip_and_get(string& user, string& file);



#endif //ARCHIVE_SERVER_CLIENT_H
