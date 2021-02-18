#include <iostream>
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include "globals.h"
#include "ArchiveServer.h"


int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");
    if (!parse_args(argc, argv)) {
        std::cout << "set all options" << std::endl;
        return 0;
    };

    try {
        boost::asio::io_context ioContext;
        boost::asio::executor_work_guard<decltype(ioContext.get_executor())> work { ioContext.get_executor() };
        boost::thread_group threadPool;
        for (int i = 0; i < threadsNum; i++) {
            threadPool.create_thread(
                    boost::bind(&boost::asio::io_service::run, &ioContext)
            );
        }

        ArchiveServer server(ioContext, ip, port, dirClient);

        threadPool.join_all();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}