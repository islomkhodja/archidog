#include "globals.h"
#include "client.h"

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");
    if (!parse_args(argc, argv)) {
        std::cout << "set all options" << std::endl;
        return 0;
    };

    try {
        string& username = user;
        string& fileName = file;

        if (command == "get") {
            get(user, file);
            return 0;
        }

        if (command == "zip") {
            zip(user, file);
            return 0;
        }

        if (command == "unzip") {
            unzip(user, file);
            return 0;
        }

        if (command == "zip-and-get") {
            zip_and_get(user, file);
            return 0;
        }

        if (command == "unzip-and-get") {
            unzip_and_get(user, file);
            return 0;
        }

    } catch (exception& e) {
        cout << "some error happened" << endl;
        cout << e.what() << endl;
    }
}

