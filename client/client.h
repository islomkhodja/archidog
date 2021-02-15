#pragma once

#include <array>
#include <fstream>

#include <boost/asio.hpp>


class Client
{
public:
    using IoService = boost::asio::io_service;
    using TcpResolver = boost::asio::ip::tcp::resolver;
    using TcpResolverIterator = TcpResolver::iterator;
    using TcpSocket = boost::asio::ip::tcp::socket;

    Client(IoService& t_ioService, TcpResolverIterator t_endpointIterator,
           std::string const& t_path, std::string command);

private:
    void openFile(const std::string &t_path);
    void doConnect();
    void doWriteFile(const boost::system::error_code& t_ec);
    template<class Buffer>
    void writeBuffer(Buffer& t_buffer);


    TcpResolver m_ioService;
    TcpSocket m_socket;
    TcpResolverIterator m_endpointIterator;
    enum { MessageSize = 1024 };
    std::array<char, MessageSize> m_buf;
    boost::asio::streambuf m_request;
    std::ifstream m_sourceFile;
    std::string m_path;
    std::string m_command;
    char in_message[128];
};


template<class Buffer>
void Client::writeBuffer(Buffer& t_buffer)
{
    boost::asio::async_write(m_socket,
                             t_buffer,
                             [this](boost::system::error_code ec, size_t /*length*/)
                             {
                                 doWriteFile(ec);
                             });
}
