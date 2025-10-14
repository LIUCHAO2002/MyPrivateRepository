#include "utils.h"
#include <cstdlib>

double randomInRange(double min, double max)
{
    return min + (max - min) * (rand() / (RAND_MAX + 1.0));
}