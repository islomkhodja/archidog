#ifndef BOOST_ARCHIVE_SERVER_H
#define BOOST_ARCHIVE_SERVER_H

#include <iostream>
#include <array>
#include <fstream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "Session.h"

class ArchiveServer {
public:
    using TcpSocket = boost::asio::ip::tcp::socket;
    using TcpAcceptor = boost::asio::ip::tcp::acceptor;
    using IoService = boost::asio::io_service;

    ArchiveServer(IoService& t_ioService, short t_port, std::string const& t_workDirectory);

private:
    void createWorkDirectory();
    void doAccept();

    TcpSocket m_socket;
    TcpAcceptor m_acceptor;
    std::string m_workDirectory;
};

#endif //BOOST_ARCHIVE_SERVER_H
