#ifndef ARCHIVE_SERVER_RLE_TEST_H
#define ARCHIVE_SERVER_RLE_TEST_H
#include <iostream>

void openTmpFile();
bool compare_files(const std::string &filename1, const std::string &filename2);
void remove(std::string &fileName);
void rle_test_main();


#endif //ARCHIVE_SERVER_RLE_TEST_H
