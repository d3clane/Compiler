#include <assert.h>
#include <string.h>

#include "CommandLineArgsParser.h"

int GetCommandLineArgPos(const int argc, const char* argv[], const char* argName)
{
    assert(argv);
    assert(argName);

    for (int i = 1; i < argc; ++i)
    {
        assert(argv[i]);

        if (strcmp(argv[i], argName) == 0)
            return i;
    }

    return NO_COMMAND_LINE_ARG;
}