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

// к этим переменным
extern string dirClient; // прикрепляется путь к рабочей папке
extern boost::asio::ip::address ip; // айпи
extern ushort port; // порт
extern uint maxNFiles; // максимальное количество файлов
extern uint reqsPerMin; // максимальное количество запросов в минуту
extern uint threadsNum; // количество потоков
extern bool verbose_flag; // verbose
struct UserSettings;

// структура данных где будем хранить пользователские данные в виде ключ-значения
extern std::map<std::string, UserSettings> UserMap;

// Тип, в которой хранятся пользовательские данные
struct UserSettings {
    uint userReqCount;
    time_t lastActiveTime;
    uint userMaxFile;
};

// фукция которая читает аргументы
bool parse_args(int argc, char* argv[]);

#endif //ARCHIVE_SERVER_GLOBALS_H