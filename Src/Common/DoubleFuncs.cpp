#include <math.h>
#include <assert.h>

#include "DoubleFuncs.h"

static const double eps = 1e-7;

bool DoubleEqual(double a, double b)
{
    assert(isfinite(a));
    assert(isfinite(b));

    return (a - b) < eps && (b - a) < eps;
}

bool DoubleLess(double a, double b)
{
    assert(isfinite(a));
    assert(isfinite(b));

    return (b - a) > eps;
}
