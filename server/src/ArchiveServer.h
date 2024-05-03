#ifndef BOOST_ARCHIVE_SERVER_H
#define BOOST_ARCHIVE_SERVER_H

#include <iostream>
#include <string>
#include <boost/asio.hpp>

/*!
 * \class ArchiveServer
 *
 * \brief Класс архивного сервера. Наш класс может выполнять 2 работы:
 * 1. Если рабочий каталог не создан, будет создан
 * 2. Запускает Сервер и выполняет работу по приему клиентов.
 */
class ArchiveServer {
public:
    using TcpSocket = boost::asio::ip::tcp::socket;
    using TcpAcceptor = boost::asio::ip::tcp::acceptor;
    using IoContext = boost::asio::io_context;

    /*!
     * Конструктор класса, принимает
     *
     * @param ioСontext
     * @param ip
     * @param port
     * @param workDirectory
     *
     *  инициализирует m_acceptor, m_socket
     */
    ArchiveServer(IoContext& ioСontext, const boost::asio::ip::address& ip, short port, std::string const& workDirectory);

private:
    /*!
     * \brief метод который создает каталог если еще не создано
     */
    void createWorkDirectory() const;

    /*! \brief асинхроно выполняет работу по приему клиентов (запускается рекурсивно)
     *
     *  Используем метод async_accept приемника (m_acceptor)
     *  и передадим ему сокет в качестве аргумента.
     *  Если клиент подключился успешно
     *  то создадим для него экземпляр класса Session и сразу же запустим его.
     *  Потом снова повторяем всю эту логику для нового клиента (через рекурсию)
     */
    void doAccept();

    /// сокет
    TcpSocket socket;

    /// acceptor — приемник который принимает клиентские подключения.
    TcpAcceptor acceptor;

    /// рабочий каталог
    std::string workDirectory;

    /*!
     * Boost.Asio использует io_context для общения с сервисом ввода/вывода операционной системы.
     * в классе будем хранить
     */
    IoContext& ioContext;
};

#endif //BOOST_ARCHIVE_SERVER_H
