#include <iostream>
#include <string.h>
#include <string>
#include <fstream>

void COMPILE(char *str, uint16_t offset, uint8_t mode){
    std::ifstream file(str, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0);
    uint8_t* byte = (uint8_t*)malloc(sizeof(uint8_t) * size);
    file.read(reinterpret_cast<char*>(byte), size);
    std::ofstream fl(strcat(str, ".zmd"));
    for(uint16_t i = 0; i < size; i ++){
        if(!mode)
            fl << "SET:[0x" << std::hex << std::uppercase << (uint16_t)(i + offset) << "]:[0x" << std::hex << std::uppercase << (uint16_t)byte[i]<< "]\n";
        else if (mode == 1)
            fl << "MEMORY[0x" << std::hex << std::uppercase << (uint16_t)(i + offset) << "] = 0x" << std::hex << std::uppercase << (uint16_t)byte[i]<< ";\n";
    }
    file.close();
    fl.close();
}

int main(int argc, char **argv){
    COMPILE(argv[1], std::stoi(argv[2], NULL, 16), std::stoi(argv[3], NULL, 10));
    return 0;
}