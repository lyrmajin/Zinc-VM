#include <stdio.h>
#include <stdlib.h>

unsigned char *memory;

/*
0x00 --> empty
0x01 --> rw directory
0x02 --> ro directory
0x03 --> rw file
0x04 --> ro file
0x05 --> cluster
*/

/*
Data:
[namesize(Null included) (1)] [namestr (namesize)] [filesize (2)] [filedata (filesize)] [clusterpointer (2)]
*/

// deafult test memory
void loadmem(long memsize)
{
    memory[0x00] = 0x00; // <-- cluster pointer high
    memory[0x01] = 0x1E; // <-- cluster pointer low
    memory[0x02] = 0x05; // <-- parent address // <-- file start
    memory[0x03] = 0x00;
    memory[0x04] = 0x1E;
    memory[0x05] = 0x00; // <-- address 1
    memory[0x06] = 0x00;
    memory[0x07] = 0x00;
    memory[0x08] = 0x00; // <-- address 2
    memory[0x09] = 0x00;
    memory[0x0A] = 0x00;
    memory[0x0B] = 0x00; // <-- address 3
    memory[0x0C] = 0x00;
    memory[0x0D] = 0x00;
    memory[0x0E] = 0x00; // <-- address 4
    memory[0x0F] = 0x00;
    memory[0x10] = 0x00;
    memory[0x11] = 0x00; // <-- address 5
    memory[0x12] = 0x00;
    memory[0x13] = 0x00;
    memory[0x14] = 0x00; // <-- address 6
    memory[0x15] = 0x00;
    memory[0x16] = 0x00;
    memory[0x17] = 0x00; // <-- address 7
    memory[0x18] = 0x00;
    memory[0x19] = 0x00; // <-- file end
    memory[0x1A] = 0x00; // <-- file size high
    memory[0x1B] = 0x18; // <-- file size low
    memory[0x1C] = '/'; // <-- name str start
    memory[0x1D] = '\0'; // <-- name str end
    memory[0x1E] = 0x02; // <-- name size

    memory[0x1F] = 0x00; // <-- cluster pointer high
    memory[0x20] = 0x27; // <-- cluster pointer low
    memory[0x21] = (((memsize - 1) & 0xFF00) >> 0x08); // <-- Allocation End High Byte // <-- file start
    memory[0x22] = ((memsize - 1) & 0x00FF); // <-- Allocation End Low Byte
    memory[0x23] = 0x00; // <-- Freed Allocation Count High Byte
    memory[0x24] = 0x00; // <-- Freed Allocation Count Low Byte // <-- file end
    memory[0x25] = 0x00; // <-- file size high
    memory[0x26] = 0x04; // <-- file size low
    memory[0x27] = '\0'; // <-- name str start // <-- name str end
    memory[0x28] = 0x01; // <-- name size
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 0;
    FILE *file = fopen(argv[1], "r");
    fseek(file, 0, SEEK_END);
    long memsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    memory = (unsigned char *)calloc(memsize, 1);
    if (!memory)
        return 1;
    loadmem(memsize);
    file = fopen(argv[argc - 1], "w");
    fwrite(memory, 1, memsize, file);
    fclose(file);
    return 0;
}