#include <iostream>

#include <boost/asio/io_service.hpp>

#include "ArchiveServer.h"


int main()
{
    setlocale(LC_ALL, "Russian");
    try {
        boost::asio::io_service ioService;

        ArchiveServer server(ioService, 9898, "../server_files");

        ioService.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}