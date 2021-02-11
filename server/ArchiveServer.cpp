#include <iostream>

#include <boost/asio/read_until.hpp>
#include <boost/filesystem.hpp>

#include "ArchiveServer.h"

Session::Session(TcpSocket t_socket)
        : m_socket(std::move(t_socket)) {
}


void Session::doRead() {
    auto self = shared_from_this();
    async_read_until(m_socket, m_requestBuf_, "\n\n",
                     [this, self](boost::system::error_code ec, size_t bytes) {
                         if (!ec)
                             processRead(bytes);
                         else
                             handleError(__FUNCTION__, ec);
                     });
}


void Session::processRead(size_t t_bytesTransferred) {
    std::cout << __FUNCTION__ << "(" << t_bytesTransferred << ")"
              << ", in_avail = " << m_requestBuf_.in_avail() << ", size = "
              << m_requestBuf_.size() << ", max_size = " << m_requestBuf_.max_size() << "." << std::endl;;

    std::istream requestStream(&m_requestBuf_);
    readData(requestStream);

    auto pos = m_fileName.find_last_of('\\');
    if (pos != std::string::npos)
        m_fileName = m_fileName.substr(pos + 1);

    createFile();

    // write extra bytes to file
    do {
        requestStream.read(m_buf.data(), m_buf.size());
        std::cout << __FUNCTION__ << " write " << requestStream.gcount() << " bytes." << std::endl;;
        m_outputFile.write(m_buf.data(), requestStream.gcount());
    } while (requestStream.gcount() > 0);

    auto self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                             [this, self](boost::system::error_code ec, size_t bytes) {
                                 if (!ec)
                                     doReadFileContent(bytes);
                                 else
                                     handleError(__FUNCTION__, ec);
                             });
}


void Session::readData(std::istream &stream) {
    stream >> m_fileName;
    stream >> m_fileSize;
    stream.read(m_buf.data(), 2);

    std::cout << m_fileName << " size is " << m_fileSize
              << ", tellg = " << stream.tellg() << std::endl;;
}


void Session::createFile() {
    m_outputFile.open(m_fileName, std::ios_base::binary);
    if (!m_outputFile) {
        std::cout << __LINE__ << ": Failed to create: " << m_fileName << std::endl;;
        return;
    }
}


void Session::doReadFileContent(size_t t_bytesTransferred) {
    if (t_bytesTransferred > 0) {
        m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(t_bytesTransferred));

        std::cout << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes" << std::endl;;

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            std::cout << "Received file: " << m_fileName << std::endl;
            return;
        }
    }
    auto self = shared_from_this();
    m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                             [this, self](boost::system::error_code ec, size_t bytes) {
                                 doReadFileContent(bytes);
                             });
}


void Session::handleError(std::string const &t_functionName, boost::system::error_code const &t_ec) {
    std::cout << __FUNCTION__ << " in " << t_functionName << " due to "
              << t_ec << " " << t_ec.message() << std::endl;
}


ArchiveServer::ArchiveServer(IoService &t_ioService, short t_port, std::string const &t_workingDirectory)
        : m_socket(t_ioService),
          m_acceptor(t_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), t_port)),
          m_workDirectory(t_workingDirectory) {
    std::cout << "ArchiveServer started\n";

    createWorkDirectory();

    doAccept();
}

void ArchiveServer::doAccept() {
    m_acceptor.async_accept(m_socket,
                            [this](boost::system::error_code ec) {
                                if (!ec) {
                                    std::make_shared<Session>(std::move(m_socket))->start();
                                }

                                doAccept();
                            });
}

void ArchiveServer::createWorkDirectory() {
    using namespace boost::filesystem;
    auto currentPath = path(m_workDirectory);

    if (!exists(currentPath) && !create_directory(currentPath)) {
        std::cout << "Coudn't create working directory: " << m_workDirectory << std::endl;;
    } else {
        std::cout << "created directory: " << m_workDirectory << std::endl;
    }
    current_path(currentPath);
}