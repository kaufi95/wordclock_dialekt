#ifndef DIALEKT_H
#define DIALEKT_H

#include <Arduino.h>
#include <time.h>

namespace dialekt
{
    bool **timeToMatrix(time_t time);
}

#endif