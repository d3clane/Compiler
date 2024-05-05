#include <stdio.h>
#include <unistd.h>

extern "C" double StdIn();

int main()
{
    printf("%lf\n", StdIn());
}