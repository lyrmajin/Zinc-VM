// GLobal dependences
#include "../dependences.h"

// Internal dependences
#include "frontend.h"
#include "../backend/utils.h"
#include <unistd.h>

int allocateToken(TOKENLIST *tokenlist, TOKEN token)
{
    if (tokenlist->listSize > tokenlist->tokensCount)
    { // If the pre allocated size is not full, append token.
        tokenlist->tokens[tokenlist->tokensCount] = token;
        tokenlist->tokensCount++;
    }
    else
    {
        TOKEN *tmp = NULL;
        for (uint8_t i = 0; i < 6; i++)
        {
            if (DoInternalsDebug)
                emitdebug("Attempting to reallocate tokens,(Data Before) tokensCount %u, listSize %u\n", tokenlist->tokensCount, tokenlist->listSize);
            tokenlist->listSize += 10;                                             // adding 10 elements to the array
            tmp = realloc(tokenlist->tokens, sizeof(TOKEN) * tokenlist->listSize); // resizing tokens to be listSize
            if (!tmp)
            { // checking reallocation errors
                emiterr("Unable to reallocate Tokens");
                if (DoInternalsDebug)
                    emitdebug("Unable to reallocate Tokens,(Data Midprocess) tokensCount %u, listSize %u\n", tokenlist->tokensCount, tokenlist->listSize);
                tokenlist->listSize -= 10; // if fail, restore original tokenlist listSize
                sleep(10);
                if (i >= 5)
                {
                    printf("Memory allocation still failing. Keep trying for another minute? [y/N]: ");
                    int c = getchar();
                    if (c == 'n' || c == 'N')
                        exit(-1);
                    else
                        i = 0;
                }
            }
            else
                i = 6;
        }
        tokenlist->tokens = tmp;
        tokenlist->tokens[tokenlist->tokensCount] = token;
        tokenlist->tokensCount++;
    }
    if (DoDebug)
        emitdebug("Emited Token \"%s\"\n", tokenlist->tokens[tokenlist->tokensCount - 1].tokenstr);
    if (DoInternalsDebug)
        emitdebug("Tokens updated successfully,(Data After) tokensCount %u, listSize %u\nToken string \"%s\"\n", tokenlist->tokensCount, tokenlist->listSize, tokenlist->tokens[tokenlist->tokensCount - 1].tokenstr);
    return 0;
}

int tokenize(FILE *INPUT, TOKENLIST *tokenlist)
{
    char *buffer = (char *)malloc(sizeof(char) * bufferSize); // creating a buffer.
    if (!buffer)
    {
        emiterr("unable to allocate buffer, defaulting to 1M");
        bufferSize = (1024 * 1024);
        buffer = (char *)malloc(sizeof(char) * bufferSize);
        if (!buffer)
        {
            emiterr("unable to allocate buffer, defaulting to 1K");
            bufferSize = 1024;
            buffer = (char *)malloc(sizeof(char) * bufferSize);
            if (!buffer)
            {
                emiterr("unable to allocate buffer, defaulting to 10");
                bufferSize = 10;
                buffer = (char *)malloc(sizeof(char) * bufferSize);
                if (!buffer)
                {
                    emiterr("unable to allocate buffer, exiting");
                    exit(-1);
                }
            }
        }
    }
    buffer[0] = '\0'; // setting buffer empty
    char line[64] = {0};
    size_t readBytes = 0;
    size_t j = 0;
    while ((readBytes = fread(buffer, 1, bufferSize, INPUT)) > 0)
    { // reading a buffer section
        for (uint32_t i = 0; i < readBytes; i++)
        {
            // printf("buffer[%u] = %c\nj = %zu\n", i, buffer[i], j); // developers debug
            if (buffer[i] == '\n')
            {
                emittoken(line, &j, tokenlist, INMEDIATE_TOKEN, allocateToken); // token emition process (sets to 0 j and COLUMN. aswell as null terminating line[0])
                LINE++;
            }
            else if (buffer[i] == ';')
            {
                while (buffer[i] != '\n' && i < readBytes)
                    i++;
            }
            else if (isSeparatorChar(buffer[i]))
            {
                emittoken(line, &j, tokenlist, INMEDIATE_TOKEN, allocateToken); // token emition process (sets to 0 j and COLUMN. aswell as null terminating line[0])
                // printf("'%c'\n", buffer[i]); // developers debug
                if (buffer[i] != ' ')
                {
                    line[0] = buffer[i];
                    line[1] = 0;
                    COLUMN--;                                                       // reducing column to avoid missaligment.
                    emittoken(line, &j, tokenlist, SEPARATOR_TOKEN, allocateToken); // token emition process (sets to 0 j and COLUMN. aswell as null terminating line[0])
                }
            }
            else
            {
                COLUMN++;
                if ((j + 1) < sizeof(line))
                {                         // line buffer size
                    if (buffer[i] != ' ') // just a sanity check, is hard for this condition to never be unmeet
                    {
                        line[j] = buffer[i];
                        j++;
                        line[j] = '\0'; // keep string null-terminated
                    }
                }
                else
                {
                    if (DoInternalsDebug)
                        emitdebug("Line Buffer \"%s\" is full, flushing...\n", line);
                    emittoken(line, &j, tokenlist, INMEDIATE_TOKEN, allocateToken); // token emition process (sets to 0 j and COLUMN. aswell as null terminating line[0])
                    if (buffer[i] != ' ')                                           // sanity check to avoid spaces
                        line[j++] = buffer[i];                                      // start new buffer with current character
                    line[j] = '\0';
                }
            }
        }
    }
    if (readBytes < bufferSize) // if eof
    {
        emittoken(line, &j, tokenlist, INMEDIATE_TOKEN, allocateToken); // token emition process (sets to 0 j and COLUMN. aswell as null terminating line[0
    }
    free(buffer);
    return 0;
}