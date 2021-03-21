#include "rle_test.h"
#include <iostream>
#include <fstream>
#include "../common/src/rle.h"

using namespace std;

string test_file_name = "sevara.test.txt";
string compressed_file_name = "sevara.test.txt.sev";
string decompressed_file_name = "decompressed.sevara.test.txt";

void openTmpFile() {
    std::ofstream outfile;
    char a[] = {'S', 'E', 'V', 'A', 'R', 'A'};

    outfile.open(test_file_name, std::ios_base::app);//std::ios_base::app
    for (int i = 0; i < sizeof(a) / sizeof(char); i++) {
        for (int j = 0; j < 255; j++) {
            outfile << a[i];
        }

        outfile << "" << endl;
    }

    outfile.close();

    cout << "1. file " << test_file_name << " successfully created" << endl;
}

bool compare_files(const std::string &filename1, const std::string &filename2) {
    std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
    std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end
    const std::ifstream::pos_type fileSize = file1.tellg();

    if (fileSize != file2.tellg()) {
        return false; //different file size
    }

    file1.seekg(0); //rewind
    file2.seekg(0); //rewind

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    return std::equal(begin1, std::istreambuf_iterator<char>(), begin2); //Second argument is end-of-range iterator
}

void remove(std::string &fileName) {
    const char *c = fileName.c_str();
    if (std::remove(c) != 0)
        cout << "remove(): Error deleting file: " << fileName << endl;
    else
        cout << "file " << fileName << " successfully deleted" << endl;
}

void rle_test_main() {
    // open tmp file for test
    openTmpFile();

    // compress it

    compress(test_file_name, compressed_file_name);
    cout << "2. file " << test_file_name << " successfully compressed. Compressed file name is " << compressed_file_name << endl;

    // decompress it
    decompress(compressed_file_name, decompressed_file_name);
    cout << "3. file " << compressed_file_name << " successfully decompressed. Decompressed file name is " << decompressed_file_name << endl;

    // compare it
    cout << "4. Comparing two files, " << test_file_name << ", with " << decompressed_file_name << endl;
    bool check = compare_files(test_file_name, decompressed_file_name);
    if (check) {
        cout << "Files " << test_file_name << ", with " << decompressed_file_name << " are identical" << endl;
    } else {
        cout << "Files " << test_file_name << ", with " << decompressed_file_name << " are not identical" << endl;
    }

    // after all, delete them all
    remove(test_file_name);
    remove(compressed_file_name);
    remove(decompressed_file_name);
}




