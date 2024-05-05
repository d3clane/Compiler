#include <stdio.h>
#include <unistd.h>

extern "C" double StdIn();
extern "C" void   StdFOut(double value);

int main()
{
    StdFOut(StdIn());
}