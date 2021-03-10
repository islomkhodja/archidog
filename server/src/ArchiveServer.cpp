#include <iostream>

#include <boost/filesystem.hpp>
#include "globals.h"
#include "Session.h"
#include "ArchiveServer.h"


ArchiveServer::ArchiveServer(IoContext &t_ioСontext, boost::asio::ip::address ip, short t_port, std::string const &t_workingDirectory)
        : m_socket(t_ioСontext),
          m_ioContext(t_ioСontext),
          m_acceptor(t_ioСontext, boost::asio::ip::tcp::endpoint(ip, t_port)),
          m_workDirectory(t_workingDirectory) {
    std::cout << "Server started\n";

    createWorkDirectory();

    doAccept();
}

void ArchiveServer::doAccept() {
    if (verbose_flag) std::cout << __FUNCTION__ << "() trying to connect the client to the server" << std::endl;
    m_acceptor.async_accept(m_socket,
                            [this](boost::system::error_code ec) {
                                if (!ec) {
                                    if (verbose_flag) std::cout << "doAccept(): " <<  "the client is connected!" << std::endl;
                                    std::make_shared<Session>(std::move(m_socket), m_ioContext, m_workDirectory)->start();
                                }

                                doAccept();
                            });
}

void ArchiveServer::createWorkDirectory() {
    using namespace boost::filesystem;
    auto currentPath = path(m_workDirectory);

    if (!exists(currentPath) && !create_directory(currentPath)) {
        std::cout << "createWorkDirectory(): Couldn't create working folder: " << m_workDirectory << std::endl;;
    } else {
        if (verbose_flag) std::cout << "createWorkDirectory(): folder created or already exists: " << m_workDirectory << std::endl;
    }
    current_path(currentPath);
}

