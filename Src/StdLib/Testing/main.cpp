#include <stdio.h>
#include <unistd.h>

extern "C" double StdIn();
extern "C" void   StdFOut(double value);
extern "C" void   StdStrOut(const char* str);

int main()
{
    printf("%f", StdIn());
    //StdStrOut("BEBRA SQUAD\n");
}