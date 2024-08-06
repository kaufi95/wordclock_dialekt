#ifndef DEUTSCH_H
#define DEUTSCH_H

#include <Arduino.h>
#include <TimeLib.h>
#include <array>

namespace deutsch
{
    std::array<std::array<bool, 11>, 11> timeToMatrix(time_t time);
}

#endif