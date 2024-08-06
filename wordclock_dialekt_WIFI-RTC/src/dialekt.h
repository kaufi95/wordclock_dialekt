#ifndef DIALEKT_H
#define DIALEKT_H

#include <Arduino.h>
#include <TimeLib.h>
#include <array>

namespace dialekt
{
    std::array<std::array<bool, 11>, 11> timeToMatrix(time_t time);
}

#endif