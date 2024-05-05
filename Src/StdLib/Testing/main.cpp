#include <stdio.h>
#include <unistd.h>

double readFunc()
{
    char array[1] = "";
    
    double result  = 0;
    double multiplier1 = 10;
    double multiplier2 = 1;
    double divider = 1;

    while (read(0, array, 1) == 1)
    {
        if (array[0] == '.')
        {
            multiplier1 = 1;
            divider = 10;
        }
        else if ('0' <= array[0] && array[0] <= '9')
        {
            result = (result * multiplier1) + (array[0] - '0') * multiplier2;
        }
        else
            break;

        multiplier2 /= divider;
    }

    return result;
}

extern "C" double StdIn();

int main()
{
    printf("%lf\n", StdIn());
}