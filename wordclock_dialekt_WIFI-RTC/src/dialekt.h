#ifndef DIALEKT_H
#define DIALEKT_H

#include <Arduino.h>
#include <TimeLib.h>
#include <array>
#include "matrixUtils.h"

namespace dialekt
{
    void timeToPixels(time_t time, std::array<std::array<bool, 11>, 11> &_matrix);
}

#endif