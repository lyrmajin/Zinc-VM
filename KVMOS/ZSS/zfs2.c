#include "zfs2.h"
#include "zalloc.h"

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

uint16_t makeFile(char *name, uint16_t size, uint8_t *buffer, uint16_t clusterptr, uint16_t offset, memoryset_t memset, memoryget_t memget)
{
    printf("File Creation was requested!: Address(0x%x){\n", offset);
    memset((strlen(name) + 1), offset); // <-- name lenght
    uint16_t afstrpos = (offset - memget(offset));
    if (name)
        vstrcpy(afstrpos, name, memset);   // <-- name string
    memset((size & 0x00FF), afstrpos - 1); // <-- size low byte
    memset((size & 0xFF00), afstrpos - 2); // <-- size high byte
    uint16_t afdtpos = (afstrpos - size - 2);
    if (buffer)
        vmemcpy(afdtpos, buffer, size, memset);         // <-- file data
    memset((clusterptr & 0x00FF), afdtpos - 1);         // <-- cluster pointer low byte
    memset((clusterptr & 0xFF00) >> 0x08, afdtpos - 2); // <-- cluster pointer high byte
    return PRINTFRETURN("}Success (0x%x)\n", afdtpos);
}

uint8_t makeEntry(uint8_t type, uint16_t address, uint16_t offset, memoryset_t memset, memoryget_t memget)
{
    printf("Entry Creation was requested!: Address(0x%x){\n", offset);
    // 24 is used here as it is the max number of bytes a directory can have. each entry is 3 bytes, so in totaal 8 entries.
    for (uint16_t i = offset; i < (offset + 24); i += 3)
    {
        if (memget(i) == 0)
        {
            memset(type, i);                                              // <-- type
            memset(((address & 0xFF00) >> 0x08), i + 1);                  // <-- address high byte
            memset((address & 0x00FF), i + 2);                            // <-- address low byte
            return PRINTFRETURN("} Success! (%u)\n", ((i - offset) / 3)); // <-- return entry number (0 - 7)
        }
    }
    return 0xFF;
}

void makeDir(char *name, uint16_t rootptr, uint16_t offset, memoryset_t memset, memoryget_t memget)
{
    printf("Dir Creation was requested!: Address(0x%x){\n", offset);
    uint16_t diraddress = makeFile(name, 24, NULL, offset, offset, memset, memget); // <-- create a dir file
    makeEntry(5, rootptr, diraddress, memset, memget);                              // <-- create a cluster entry pointing to the rootdir, at diraddress
    printf("} Success!\n");
}

uint16_t getEntryAddress(uint8_t entryindex, uint16_t offset, memoryget_t memget)
{
    printf("Entry Address was requested!{\n");
    uint16_t address = offset + (entryindex * 3);                                                       // <-- converting the entry number in a absolute address based on offset
    if (memget(address))                                                                                // <-- if the entry isnt empty
        return PRINTFRETURN("}Result (0x%x)\n", ((memget(address + 1) << 0x08) | memget(address + 2))); // <-- return the address
    return PRINTFRETURN("}Result (0x%x)\n", offset);                                                    // <-- return offset to signal entry is empty
}

uint8_t getEntryType(uint8_t entryindex, uint16_t offset, memoryget_t memget)
{
    printf("Entry Type was requested!{\n");
    uint16_t address = offset + (entryindex * 3);             // <-- converting the entry number in a absolute address based on offset
    return PRINTFRETURN("}Result (0x%x)\n", memget(address)); // <-- return entry type
}

uint8_t getFileNameSize(uint16_t offset, memoryget_t memget)
{
    printf("File Name Size was requested!{\n");
    return PRINTFRETURN("}Result (0x%x)\n", memget(offset)); // <-- string size
}

uint16_t getFileName(uint16_t offset, memoryget_t memget)
{
    printf("File Name Ptr was requested!{\n");
    uint16_t size = getFileNameSize(offset, memget); // <-- string size
    return PRINTFRETURN("}Result (0x%x)\n", offset - size);
}

uint16_t getFileDataSize(uint16_t offset, memoryget_t memget)
{
    printf("File Data Size was requested!{\n");
    uint16_t strsize = getFileNameSize(offset, memget); // <-- string size
    return PRINTFRETURN("}Result (0x%x)\n", ((memget(offset - strsize - 2) << 0x08) | memget(offset - strsize - 1)));
}

uint16_t getFileSize(uint16_t offset, memoryget_t memget)
{
    printf("File Size was requested!{\n");
    uint16_t strsize = getFileNameSize(offset, memget); // <-- string size
    uint16_t dtsize = getFileDataSize(offset, memget);  // <-- string size

    return PRINTFRETURN("}Result (0x%x)\n", 1 + strsize + 2 + dtsize + 2); // <-- (namelen (1)) + (strsize) + (size (2)) + (dtsize) + (clusterptr (2))
}

uint16_t getFileDataAddress(uint16_t offset, memoryget_t memget)
{
    printf("File Address was requested!{\n");
    uint16_t strsize = getFileNameSize(offset, memget); // <-- string size
    uint16_t dtsize = getFileDataSize(offset, memget);  // <-- string size

    return PRINTFRETURN("}Result (0x%x)\n", offset - (strsize + 2 + dtsize)); // <-- offset - (The offset to the data start, from offset)
}

uint16_t getFileClusterPtr(uint16_t offset, memoryget_t memget)
{
    printf("File ClusterPtr was requested!{\n");
    // --> (offset) - (namelen (1)) - (strsize) - (size (2)) - (dtsize)
    return PRINTFRETURN("}Result (0x%x)\n", (memget(getFileDataAddress(offset, memget) - 2) << 0x08) | memget(getFileDataAddress(offset, memget) - 1));
}

uint16_t getClusterDataSize(uint16_t offset, memoryget_t memget)
{
    printf("Cluster Data Size was requested!{\n");
    return PRINTFRETURN("}Result (0x%x)\n", (memget(offset - 1) << 0x08) | memget(offset));
}

uint16_t getClusterSize(uint16_t offset, memoryget_t memget)
{
    printf("Cluster Size was requested!{\n");
    uint16_t dtsize = getClusterDataSize(offset, memget);

    return PRINTFRETURN("}Result (0x%x)\n", offset - (4 + dtsize)); // <-- offset - (The offset to the data start, from offset)
}

uint16_t getClusterDataAddress(uint16_t offset, memoryget_t memget)
{
    printf("Cluster Data Address was requested!{\n");
    uint16_t dtsize = getClusterDataSize(offset, memget);

    return PRINTFRETURN("}Result (0x%x)\n", offset - (2 + dtsize)); // <-- offset - (The offset to the data start, from offset)
}

uint16_t getClusterClusterPtr(uint16_t offset, memoryget_t memget)
{
    printf("Cluster ClusterPtr was requested!{\n");
    // --> (offset) - (namelen (1)) - (strsize) - (size (2)) - (dtsize)
    return PRINTFRETURN("}Result (0x%x)\n", (memget(getClusterDataAddress(offset, memget) - 2) << 0x08) | memget(getClusterDataAddress(offset, memget) - 1));
}

void readClusterData(uint8_t *data, uint16_t size, uint16_t offset, memoryget_t memget)
{
    printf("Cluster Data (Read) was requested!{\n");
    uint16_t datasize = getClusterDataSize(offset, memget);
    if (size > datasize)
        size = datasize; // clamp

    memcpyv(data, getClusterDataAddress(offset, memget), size, memget);
    printf("} Success!\n");
}

void writeClusterData(uint8_t *data, uint16_t size, uint16_t offset, memoryset_t memset, memoryget_t memget)
{
    printf("Cluster Data (Write) was requested!{\n");
    uint16_t datasize = getClusterDataSize(offset, memget);
    if (size > datasize)
        size = datasize; // clamp

    vmemcpy(getClusterDataAddress(offset, memget), data, size, memset);
    printf("} Success!\n");
}

void readFileData(uint8_t *data, uint16_t size, uint16_t offset, memoryget_t memget)
{
    printf("File Data (Read) was requested!{\n");
    uint16_t clusteroffset = getFileName(offset, memget) - 1;   // this gets the cluster address of the file entry
    uint16_t ptr = getClusterClusterPtr(clusteroffset, memget); // this is the cluster ptr. if is equal to the clusteroffset. we at end of cluster chain
    uint16_t datasize = getClusterDataSize(clusteroffset, memget);
    do
    {
        if (size > datasize)
        {
            size -= datasize;
            memcpyv(data, getClusterDataAddress(ptr, memget), datasize, memget);
            goto CHECK;
        }
        memcpyv(data, getClusterDataAddress(ptr, memget), size, memget);
    CHECK:
        data += datasize;

        if (ptr != clusteroffset)
        { // check if this is end of cluster
            clusteroffset = ptr;
            ptr = getClusterClusterPtr(clusteroffset, memget);
            datasize = getClusterDataSize(clusteroffset, memget);
        }
        else
            break;

    } while (1);

    printf("} Success!\n");
}

void writeFileData(uint8_t *data, uint16_t size, uint16_t offset, memoryset_t memset, memoryget_t memget)
{
    printf("File Data (Write) was requested!{\n");
    uint16_t clusteroffset = getFileName(offset, memget) - 1;   // this gets the cluster address of the file entry
    uint16_t ptr = getClusterClusterPtr(clusteroffset, memget); // this is the cluster ptr. if is equal to the clusteroffset. we at end of cluster chain
    uint16_t datasize = getClusterDataSize(clusteroffset, memget);
    do
    {
        if (size > datasize)
        {
            size -= datasize;
            vmemcpy(getClusterDataAddress(ptr, memget), data, datasize, memset);
            goto CHECK;
        }
        vmemcpy(getClusterDataAddress(ptr, memget), data, size, memset);
    CHECK:
        ptr += datasize;

        if (ptr != clusteroffset)
        { // check if this is end of cluster
            clusteroffset = ptr;
            ptr = getClusterClusterPtr(clusteroffset, memget);
            datasize = getClusterDataSize(clusteroffset, memget);
        }
        else
            break;

    } while (1);
    printf("} Success!\n");
}

// 22

/*
void mkdir(char *dirName, uint16_t currentDirAddress, uint16_t *hof, uint8_t *memory)
{
    uint16_t currentDirDataAddress = getFileDataAddress(currentDirAddress, memory);
    makeEntry(1, *hof, currentDirDataAddress, memory);
    makeDir(dirName, currentDirAddress, *hof, memory);

    *hof -= getFileSize(*hof, memory);
}

void touch(char *Name, uint16_t currentDirAddress, uint16_t *hof, uint8_t *memory)
{
    uint16_t currentDirDataAddress = getFileDataAddress(currentDirAddress, memory);
    makeEntry(3, *hof, currentDirDataAddress, memory);
    makeFile(Name, 0, NULL, *hof, *hof, memory);

    *hof -= getFileSize(*hof, memory);
}

void start(uint16_t *hof, uint16_t *fsc, uint16_t *cda, uint8_t *buffer, uint8_t *memory)
{
    if (!buffer)
        return 1;
    readFileData(buffer, 4, *cda + 0x27, memory);
    *hof = ((buffer[0] << 0x8) | buffer[1]);
    *fsc = ((buffer[2] << 0x8) | buffer[3]);

    *cda += 0x1D;

}
void end(uint16_t *hof, uint16_t *fsc, uint16_t *cda, uint8_t *buffer, uint8_t *memory)
{

    buffer[0] = ((*hof >> 0x08) & 0xFF);
    buffer[1] = (*hof & 0xFF);
    buffer[2] = ((*fsc >> 0x08) & 0xFF);
    buffer[3] = (*fsc & 0xFF);

    writeFileData(buffer, 4, *cda + 0x27, memory);

    return 0;
}*/