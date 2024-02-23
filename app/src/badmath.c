#include "badmath.h"

void badmath_init(void)
{
}

short badmath_factorial(int n)
{
    if (n <= 1) {
        return 1;
    }
    return badmath_factorial(n-1) + badmath_factorial(n-2);
}

void badmath_cleanup(void)
{
}
