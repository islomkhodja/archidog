#include <iostream>

#include <boost/filesystem.hpp>
#include "globals.h"
#include "Session.h"
#include "ArchiveServer.h"


ArchiveServer::ArchiveServer(IoContext &ioСontext, const boost::asio::ip::address& ip, const short port, std::string const &workDirectory)
        : socket(ioСontext),
          acceptor(ioСontext, boost::asio::ip::tcp::endpoint(ip, port)),
          workDirectory(workDirectory),
          ioContext(ioСontext) {
    std::cout << "Server started\n";

    createWorkDirectory();

    doAccept();
}

void ArchiveServer::doAccept() {
    if (verbose_flag) std::cout << __FUNCTION__ << "() trying to connect the client to the server" << std::endl;
    acceptor.async_accept(socket,
                            [this](boost::system::error_code ec) {
                                if (!ec) {
                                    if (verbose_flag) std::cout << "doAccept(): " <<  "the client is connected!" << std::endl;
                                    std::make_shared<Session>(std::move(socket), ioContext, workDirectory)->start();
                                }

                                doAccept();
                            });
}

void ArchiveServer::createWorkDirectory() const {
    const auto currentPath = boost::filesystem::path(workDirectory);

    if (!boost::filesystem::exists(currentPath) && !boost::filesystem::create_directory(currentPath)) {
        std::cout << "createWorkDirectory(): Couldn't create working folder: " << workDirectory << std::endl;;
    } else {
        if (verbose_flag) std::cout << "createWorkDirectory(): folder created or already exists: " << workDirectory << std::endl;
    }
    boost::filesystem::current_path(currentPath);
}


