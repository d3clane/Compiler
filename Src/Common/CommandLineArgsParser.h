#ifndef COMMAND_LINE_PARSER_H
#define COMMAND_LINE_PARSER_H

#include <stddef.h>

static const int NO_COMMAND_LINE_ARG = -1;
int GetCommandLineArgPos(const int argc, const char* argv[], const char* argName);

#endif