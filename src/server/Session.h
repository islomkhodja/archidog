
#ifndef ARCHIVE_SERVER_SESSION_H
#define ARCHIVE_SERVER_SESSION_H

#include <iostream>
#include <array>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

class Session
        : public std::enable_shared_from_this<Session>
{
public:
    using TcpSocket = boost::asio::ip::tcp::socket;

    Session(TcpSocket t_socket, boost::asio::io_context& ioContext, boost::filesystem::path m_workDirectory);

    void start()
    {
        readCommands();
    }

private:
    // main methods
    void readCommands();
    void commandRouter(size_t t_bytesTransferred);
    bool checkUserActive();
    //TODO remove command params from Handlers
    void zipCommandHandler(const std::string &command);
    void unZipCommandHandler(const std::string &command);
    void zipLogic();
    void unZipLogic();
    void getCommandHandler(const std::string &fileName);

    // write methods
    void writeToClient(std::string &message);
    void sendError(const std::string& errorMessage, int line, const std::string& at);
    template<class Buffer>
    void writeBuffer(Buffer &t_buffer, bool once = false);

    // helper methods
    std::string getFileName(std::string const &fileName, std::string type);
    void createFile(std::string const &fileName);
    void sendFile(boost::system::error_code ec);
    bool findFile(const std::string &file_name, boost::filesystem::path &path_found);
    void openFile(const std::string &t_path);
    void remove(std::string &fileName);
    void handleError(std::string const& t_functionName, boost::system::error_code const& t_ec);


    TcpSocket socket;
    boost::asio::io_context& ioContext;
    boost::asio::io_context::strand myStrand;
    boost::asio::streambuf requestBufRead;
    boost::asio::streambuf requestBufSend;
    enum { MaxLength = 40960 };
    std::array<char, MaxLength> readBuf;
    enum { MessageSize = 1024 };
    std::array<char, MessageSize> sendBuf;
    std::ofstream outputFile;
    std::ifstream sourceFile;
    size_t fileSize;
    std::string fileName;
    std::string command;
    std::string user;
    boost::filesystem::path workDirectory;
};

template<class Buffer>
void Session::writeBuffer(Buffer& t_buffer, const bool once)
{
    auto self(shared_from_this());
    boost::asio::async_write(socket,
                             t_buffer,
                             boost::asio::bind_executor(myStrand,[this, self, once](boost::system::error_code ec, size_t length)
                             {
                                 if (!ec && !once) {
                                     if (verbose_flag) std::cout << "writeBuffer(): " << "ok sent" << length << " bytes. If anything, we will send it again" << std::endl;
                                     sendFile(ec);
                                 }
                             }));
}


#endif //ARCHIVE_SERVER_SESSION_H
