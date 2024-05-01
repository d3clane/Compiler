#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H

/// @file 
/// @brief Contains functions to work with input and output for text input

#include <stdio.h>

/// @brief Contains info about line (her ending and length)
struct LineType
{
    const char* line;       ///< string line
    char lineEnding;        ///< symbol on which line is ending
    size_t lineLength;      ///< line length
};

/// @brief Contains info for working with big texts from the file
struct TextType
{    
    char *text;                 ///< Dynamic array containing text from file
    size_t textSz;              ///< number of elements in text var
    
    LineType* lines;            ///< Dynamic arr with pointers to the lines
    size_t linesCnt;            ///< number of elements in ptrArr
};

/// @brief Destructs text
///
/// @details Free all dynamic arrays
/// @param [out]text structure to destruct
void TextTypeDestructor(TextType* text);

//------------------------------------------------------------------------------------------------

/// @brief opens file with fileName and calls TextTypeCtor(text, fp);
///
/// @param [out]text struct to fill
/// @param [in]fileName file to open
/// @return ReadTextAndParse result
int TextTypeCtor(TextType* text, const char* const fileName);

//------------------------------------------------------------------------------------------------

/// @brief reads text from the inStream and parses it to the strings.
///
/// @details read text, unite all '\n' symbols that goes in a row in one '\n' and parses this on strings
/// @param [out]text struct to fill.
/// @param [in]inFileName file name to open to read from
/// @return 0 if no errors occurred otherwise not 0
/// @attention Creates dynamic arrays in text structure
int TextTypeCtor(TextType* text, FILE* const inStream);

//------------------------------------------------------------------------------------------------

/// @brief reads text from the inStream
///
/// @param [in]inStream stream to read from
/// @return pointer to the dynamic array containing text
/// @attention Creates dynamic array
char* ReadText(FILE* const inStream);

//------------------------------------------------------------------------------------------------

/// @brief prints text
///
/// @param [in]lines array containing pointers to the lines.
/// @param [in]linesCnt number of elements in ptrArr
/// @param [in]outStream file pointer to print output
/// @return 0 if no errors occurred otherwise not 0
int PrintLines(LineType* lines, const size_t linesCnt, FILE* const outStream);

//------------------------------------------------------------------------------------------------

/// @brief prints text to outFile
///
/// @param [in]text text to print
/// @param [in]length number of element in the text to be printed
/// @param [in]outFileName file to open to print out
/// @return number of printed values
size_t PrintText(const char* const text, const size_t length, FILE* const outStream);

//------------------------------------------------------------------------------------------------

/// @brief returns size in bytes of the file with fileName
///
/// @param [in]fileName to get size
/// @return -1 if error occurred otherwise file size in bytes
size_t GetFileSize(const char* const fileName);

//------------------------------------------------------------------------------------------------

/// @brief returns size in bytes of the file in fp
///
/// @param [in]fp file pointer to count bytes in
/// @return 
size_t GetFileSize(FILE* const fp);

//------------------------------------------------------------------------------------------------

/// @brief puts strings to the stdout and stops printing on separator symbol. 
///
/// @details function doesn't puts separator symbol at the end of the string. 
/// @details It also puts '\n' symbol at the end   
///                      
/// @param [in]line Line to print out
/// @param [in]outStream stream to print out
/// @return EOF if errors otherwise ASCII code of the last printed symbol
int PutLine(LineType line, FILE* const outStream);

//------------------------------------------------------------------------------------------------

/// @brief Wipes off file with fileName
/// @param [in]fileName name of the file to wipe
void WipeFile(const char* const fileName);

//------------------------------------------------------------------------------------------------

/// @brief Print separator in file. Can be used for printing separator
/// @param [in]outStream stream to print out separators
static inline void PrintTextsSeparators(FILE* const outStream)
{
    fprintf(outStream, "\n\n\n\n\n\n\n");
    fprintf(outStream, "-------------------------------------------------------------------------");
    fprintf(outStream, "\n\n\n\n\n\n\n");
}

//------------------------------------------------------------------------------------------------

/// @brief Tries to open file. Updates special ErrorInfo structure on error
/// @param [in]fileName file to open
/// @param [in]mode mode to open file
/// @return pointer to opened file (nullptr if can't open);
FILE* TryOpenFile(const char* const fileName, const char* const mode);

//------------------------------------------------------------------------------------------------

#endif // INPUT_OUTPUT_H