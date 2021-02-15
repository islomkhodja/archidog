#include <iostream>

#include <boost/filesystem.hpp>

#include "Session.h"
#include "ArchiveServer.h"


ArchiveServer::ArchiveServer(IoService &t_ioService, short t_port, std::string const &t_workingDirectory)
        : m_socket(t_ioService),
          m_acceptor(t_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), t_port)),
          m_workDirectory(t_workingDirectory) {
    std::cout << "Server started\n";

    createWorkDirectory();

    doAccept();
}

void ArchiveServer::doAccept() {
    std::cout << __FUNCTION__ << "пытаемся подключит клиента к серверу" << std::endl;
    m_acceptor.async_accept(m_socket,
                            [this](boost::system::error_code ec) {
                                if (!ec) {
                                    std::cout << "async_accept::" <<  "клиент подключился!" << std::endl;
                                    std::make_shared<Session>(std::move(m_socket))->start();
                                }

                                doAccept();
                            });
}

void ArchiveServer::createWorkDirectory() {
    using namespace boost::filesystem;
    auto currentPath = path(m_workDirectory);

    if (!exists(currentPath) && !create_directory(currentPath)) {
        std::cout << "Не удалось создать рабочую папку: " << m_workDirectory << std::endl;;
    } else {
        std::cout << "папка создан или уже существует: " << m_workDirectory << std::endl;
    }
    current_path(currentPath);
}

