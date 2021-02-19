#include <iostream>

#include <boost/filesystem.hpp>
#include "globals.h"
#include "Session.h"
#include "ArchiveServer.h"


ArchiveServer::ArchiveServer(IoService &t_ioService, boost::asio::ip::address ip, short t_port, std::string const &t_workingDirectory)
        : m_socket(t_ioService),
          m_ioContext(t_ioService),
          m_acceptor(t_ioService, boost::asio::ip::tcp::endpoint(ip, t_port)),
          m_workDirectory(t_workingDirectory) {
    std::cout << "Server started\n";

    createWorkDirectory();

    doAccept();
}

void ArchiveServer::doAccept() {
    if (verbose_flag) std::cout << __FUNCTION__ << "() пытаемся подключить клиента к серверу" << std::endl;
    m_acceptor.async_accept(m_socket,
                            [this](boost::system::error_code ec) {
                                if (!ec) {
                                    if (verbose_flag) std::cout << "doAccept(): " <<  "клиент подключился!" << std::endl;
                                    std::make_shared<Session>(std::move(m_socket), m_ioContext, m_workDirectory)->start();
                                }

                                doAccept();
                            });
}

void ArchiveServer::createWorkDirectory() {
    using namespace boost::filesystem;
    auto currentPath = path(m_workDirectory);

    if (!exists(currentPath) && !create_directory(currentPath)) {
        std::cout << "createWorkDirectory(): Не удалось создать рабочую папку: " << m_workDirectory << std::endl;;
    } else {
        if (verbose_flag) std::cout << "createWorkDirectory(): папка создана или уже существует: " << m_workDirectory << std::endl;
    }
    current_path(currentPath);
}

