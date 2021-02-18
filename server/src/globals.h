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

extern string dirClient;
extern boost::asio::ip::address ip;
extern ushort port;
extern uint maxNFiles;
extern uint reqsPerMin;
extern uint threadsNum;
extern bool verbose_flag;
struct UserSettings;
extern std::map<std::string, UserSettings> UserMap;


struct UserSettings {
    uint userReqCount;
    time_t lastActiveTime;
    uint userMaxFile;
};


bool parse_args(int argc, char* argv[]);

#endif //ARCHIVE_SERVER_GLOBALS_H
