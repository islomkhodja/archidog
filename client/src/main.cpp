#include <iostream>

#include <boost/asio/io_service.hpp>

#include "client.h"

int main(int argc, char* argv[])
{

    auto address = "localhost";
    auto port = "9898";
    auto filePath ="../client_files/sevara.txt.rle";

    try {
        boost::asio::io_service ioService;

        boost::asio::ip::tcp::resolver resolver(ioService);
        auto endpointIterator = resolver.resolve({ address, port });
        Client client(ioService, endpointIterator, filePath, "zip");
//        Client client(ioService, endpointIterator, filePath, "unzip");

        ioService.run();
    } catch (std::fstream::failure& e) {
        std::cerr << e.what() << "\n";
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
