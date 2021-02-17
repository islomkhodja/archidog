#include <string>
#include <iostream>

#include <boost/filesystem/path.hpp>

#include "old_client.h"


Client::Client(
        IoService& t_ioService,
        TcpResolverIterator t_endpointIterator,
        std::string const& t_path, std::string command
)
        :
        m_ioService(t_ioService),
        m_socket(t_ioService),
        m_endpointIterator(t_endpointIterator),
        m_path(t_path),
        m_command(command)
{
    doConnect();

}


void Client::openFile(std::string const& t_path)
{
    m_sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (m_sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path);

    // get length of file
    m_sourceFile.seekg(0, m_sourceFile.end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, m_sourceFile.beg);

    // m_request нужен для запроса
    // m_request передаем в requestStream с типом ostream
    // и через него управляем
    std::ostream requestStream(&m_request);
    boost::filesystem::path p(t_path);
    //requestStream << p.filename().string() << "\n" << fileSize << "\n\n";
    requestStream << m_command << " " << p.filename().string() << " " << fileSize << "\n\n";
    std::cout << "Request size: " << m_request.size() << std::endl;
}


void Client::doConnect()
{
    boost::asio::async_connect(m_socket, m_endpointIterator,
                               [this](boost::system::error_code ec, const TcpResolverIterator&)
                               {
                                   if (!ec) {
                                       openFile(m_path);
                                       writeBuffer(m_request);
                                   } else {
                                       std::cout << "Coudn't connect to host. Please run server "
                                                    "or check network connection.\n";
                                       std::cout << "Error: " << ec.message();
                                   }
                               });
}


void Client::doWriteFile(const boost::system::error_code& t_ec)
{
    if (!t_ec) {
        if (m_sourceFile) {
            m_sourceFile.read(m_buf.data(), m_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            std::stringstream ss;
            ss << "Send " << m_sourceFile.gcount() << " bytes, total: "
               << m_sourceFile.tellg() << " bytes";
            std::cout << ss.str() << std::endl;

            auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        } else {
            std::cout << "конец!" << std:: endl;
            size_t bytes = m_socket.read_some( boost::asio::buffer(in_message, 128) );
            std::istringstream ss(in_message);
            std::string ok;
            ss >> ok;
            std::string name;
            ss >> name;
            std::cout << "message: " << ok << " " << name << std::endl;
        }
    } else {
        std::cout << "Error: " << t_ec.message();
    }
}
