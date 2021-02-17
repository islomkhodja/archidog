#include "rle.h"

float compress(std::basic_string<char> fname, std::basic_string<char> cname) {
    std::fstream file;            // original file
    std::fstream compressed;      // compressed file
    char character;          // читаемый символ
    char next_character;     // следующий читаемый символ
    int fsize = 0;           // размер файла
    int frequency = 1;       // уникальность символов
    int write_pos = 0;

    file.open(fname, std::ios::in | std::ios::ate | std::ios::binary);
    compressed.open(cname, std::ios::out | std::ios::trunc | std::ios::binary);
    fsize = file.tellg();

    for(int i = 0; i < fsize; i++)
    {
        file.seekg(i, std::ios::beg);      // safety net
        file.read((char*)&character, sizeof(char)); // получить текущую символ
        next_character = file.peek();

        if(next_character != character)
        {
            compressed.seekp(write_pos, std::ios::beg);
            compressed.write((char*)&frequency, sizeof(char));
            compressed.seekp(write_pos + 1, std::ios::beg);
            compressed.write((char*)&character, sizeof(char));
            write_pos += 2;
            frequency = 0;
        }
        frequency++;
    }

    file.close();
    compressed.close();

    return (write_pos / float(fsize));
}

void decompress(std::basic_string<char> fname, const std::basic_string<char> uname) {
    std::fstream file;
    std::fstream ufile;
    char character;
    int frequency = 0;
    int fsize = 0;
    int write_pos = 0;

    file.open(fname, std::ios::ate | std::ios::in | std::ios::binary);
    ufile.open(uname, std::ios::trunc | std::ios::out | std::ios::binary);
    fsize = file.tellg();

    for(int i = 0; i < fsize; i += 2)
    {
        file.seekg(i, std::ios::beg);
        file.read((char*)&frequency, sizeof(char));
        file.seekg(i + 1, std::ios::beg);
        file.read((char*)&character, sizeof(char));

        for(int j = 0; j < frequency; j++)
        {
            ufile.seekp(write_pos, std::ios::beg);
            ufile.write((char*)&character, sizeof(char));
            write_pos++;
        }
    }
    file.close();
    ufile.close();
}