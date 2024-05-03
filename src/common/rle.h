#ifndef ARCHIVE_SERVER_RLE_H
#define ARCHIVE_SERVER_RLE_H
#include <iostream>
#include <fstream>
#include <string>
#include <memory>

float compress(std::basic_string<char> fname, std::basic_string<char> cname);
void decompress(std::basic_string<char> fname, std::basic_string<char> uname);

#endif //ARCHIVE_SERVER_RLE_H
