#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "StringFuncs.h"

//------------------------------------------------------------------------------------------------

LineType* BuildLinesArr(const char* text, const char separator, size_t* arrSize)
{
    assert(text);
    assert(arrSize);

    size_t linesCnt = CountChars(text, separator) + 1;

    LineType* lines = (LineType*) calloc(linesCnt, sizeof(*lines));

    if (lines == nullptr)
    {
        return nullptr;
    }

    size_t lineIndex = 0;

#pragma GCC diagnostic ignored "-Wtype-limits"
    assert(0 <= lineIndex && lineIndex < linesCnt);
#pragma GCC diagnostic warning "-Wtype-limits"

    lines[lineIndex].line       = text;
    lines[lineIndex].lineEnding = '\n';

    ++lineIndex;

    const char* textIterator = text;
    while (*textIterator)
    {
        if (*textIterator == separator)
        {
#pragma GCC diagnostic ignored "-Wtype-limits"
            assert(0 <= lineIndex && lineIndex < linesCnt);     
#pragma GCC diagnostic warning "-Wtype-limits"   

            assert(lineIndex > 0);
            lines[lineIndex].lineEnding     = '\n';
            lines[lineIndex].line           = textIterator + 1;
            lines[lineIndex - 1].lineLength = (size_t) (lines[lineIndex].line - lines[lineIndex - 1].line);
            ++lineIndex;
        }
        ++textIterator;
    }
    
    *arrSize = lineIndex;

    assert(lineIndex > 0);
    assert(lineIndex - 1 < linesCnt);
    lines[lineIndex - 1].lineLength = 1; // \0 symbol at the end

    return lines;
}

//------------------------------------------------------------------------------------------------

size_t CountChars(const char* str, const char ch)
{
    size_t cnt = 0;

    if (ch == '\0')
        return 1;
    
    while (true) 
    {
        str = strchr(str, ch);

        if (str == nullptr)
            break;
        
        ++str;
        ++cnt;
    }

    return cnt;
}

//------------------------------------------------------------------------------------------------

size_t UniteChars(char *str, const char ch)
{
    assert(str);

    size_t pos = 0;
    size_t shift = 0;

    while (str[pos])
    {
        assert(pos >= shift);
#pragma GCC diagnostic ignored "-Wtype-limits"
        assert(pos >= 0);
#pragma GCC diagnostic warning "-Wtype-limits"
        assert(str);
        str[pos - shift] = str[pos];
        
        bool haveDoneCycle = false;
        while (str[pos] && str[pos] == ch)
        {
            ++pos;
            ++shift;
            haveDoneCycle = true;
        }

        if (haveDoneCycle) //in purpose to place one ch symbol 
            --shift;
        
        if (str[pos] && !haveDoneCycle) ++pos;
    }

    str[pos - shift] = '\0';

    return pos - shift + 1;
}

//------------------------------------------------------------------------------------------------

void Replace(char *str, const char find, const char replace)
{
    assert(str);

    while (*str)
    {  
        assert(str);
        if (*str == find) *str = replace;

        ++str;
    }
}
