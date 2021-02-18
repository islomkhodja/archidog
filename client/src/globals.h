#ifndef ARCHIVE_SERVER_GLOBALS_H
#define ARCHIVE_SERVER_GLOBALS_H

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>
namespace po = boost::program_options;
using namespace std;

typedef unsigned int uint;
typedef unsigned short ushort;

extern string user;
extern string command;
extern string file;
extern string dirClient;
extern string hostIp;
extern string port;

bool parse_args(int argc, char* argv[]);

#endif //ARCHIVE_SERVER_GLOBALS_H
