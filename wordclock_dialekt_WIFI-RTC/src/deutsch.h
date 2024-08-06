#ifndef DEUTSCH_H
#define DEUTSCH_H

#include <Arduino.h>
#include <TimeLib.h>

namespace deutsch
{
    bool **timeToMatrix(time_t time);
}

#endif