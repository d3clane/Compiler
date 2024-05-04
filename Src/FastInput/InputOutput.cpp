
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "Common/Colors.h"
#include "InputOutput.h"
#include "StringFuncs.h"

//------------------------------------------------------------------------------------------------


int TextTypeCtor(TextType* text, const char* const fileName)
{
    FILE* inStream = TryOpenFile(fileName, "rb");

    if (inStream == nullptr)
    {
        return -1;
    }

    int retVal = TextTypeCtor(text, inStream);

    fclose(inStream);

    return retVal;
}

//------------------------------------------------------------------------------------------------

int TextTypeCtor(TextType* text, FILE* const inStream)
{
    assert(text);
    assert(inStream);
    
    // reading:

    text->text = ReadText(inStream);

    if (text->text == nullptr)
        return -1;

    // preparing text:

    const size_t newTextSize = UniteChars(text->text, '\n');

    char* const tmp = (char*) realloc(text->text, newTextSize * sizeof(*(text->text)));
    
    if (tmp) 
        text->text = tmp;
    else 
        return -1;

    text->textSz = newTextSize - 1;

    // parsing on lines:

    text->lines = BuildLinesArr(text->text, '\n', &(text->linesCnt));

    return text->lines? 0 : -1;
}

//------------------------------------------------------------------------------------------------

char* ReadText(FILE* const inStream)
{
    assert(inStream);

    size_t fileSize = GetFileSize(inStream);
    
    if (fileSize == 0)
        return nullptr;

    const size_t bufSize = fileSize + 1;

    char* text = (char*) calloc(bufSize, sizeof(*text)); 

    if (text == nullptr)
    {
        return text;
    }

    size_t nRead = fread(text, sizeof(char), fileSize, inStream);

    assert(nRead == fileSize);

    assert(bufSize > 0);
    text[bufSize - 1] = '\0';

    return text;
}

//------------------------------------------------------------------------------------------------

int PrintLines(LineType* lines, const size_t linesCnt, FILE* const outStream)
{
    assert(lines);
    assert(linesCnt > 0);
    assert(outStream);

    for (size_t i = 0; i < linesCnt; ++i)
    {
        assert(i < linesCnt);

        int printingError = PutLine(lines[i], outStream);

        if (printingError == EOF)
            return printingError;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------

size_t PrintText(const char* const text, const size_t length, FILE* const outStream)
{
    assert(text);
    assert(length > 0);
    assert(outStream);
    
    size_t nPrintedVals = fwrite(text, sizeof(*text), length, outStream);

    return nPrintedVals;
}

//------------------------------------------------------------------------------------------------

size_t GetFileSize(const char *const fileName)
{
    assert(fileName);

    struct stat fileStats = {0};

    int statError = stat(fileName, &fileStats);

    if (statError)
    {
        return 0;
    }
    
    return (size_t) fileStats.st_size;
}

//------------------------------------------------------------------------------------------------

size_t GetFileSize(FILE* const fp)
{
    assert(fp);
    
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        return 0;
    }

    long fileSz = ftell(fp);

    if (fileSz == -1)
    {
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        return 0;
    }

    return (size_t) fileSz;
}

//------------------------------------------------------------------------------------------------

int PutLine(LineType line, FILE* const outStream)
{
    assert(line.line);
    assert(outStream);

    const char* lineIterator = line.line;

    while (*lineIterator != line.lineEnding && *lineIterator != '\0')
    {
        int printingError = putc(*lineIterator, outStream);

        if (printingError == EOF)
            return printingError;
        
        ++lineIterator;
    }

    return putc('\n', outStream);
}

//------------------------------------------------------------------------------------------------

void WipeFile(const char* const fileName) 
{
    assert(fileName);

    FILE *fp = fopen(fileName, "w");

    if (fp == nullptr)
        return;

    fclose(fp);
}

//------------------------------------------------------------------------------------------------

void TextTypeDestructor(TextType* const text)
{
    if (text == nullptr)
        return;

    assert(text);
    free(text->text);
    text->text = nullptr;

    assert(text);
    free(text->lines);
    text->lines = nullptr;

    assert(text);
    text->linesCnt = 0;
}

//------------------------------------------------------------------------------------------------

FILE* TryOpenFile(const char* const fileName, const char* const mode)
{
    assert(fileName);
    assert(mode);

    FILE* fp = fopen(fileName, mode);

    return fp;
}

//------------------------------------------------------------------------------------------------