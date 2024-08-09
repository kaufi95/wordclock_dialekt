#ifndef MATRIXUTILS_H
#define MATRIXUTILS_H

#include <Arduino.h>
#include <array>

bool showEsIst(uint8_t minutes);
void resetMatrix(std::array<std::array<bool, 11>, 11> &matrix);
void turnPixelsOn(uint8_t start, uint8_t end, uint8_t row, std::array<std::array<bool, 11>, 11> &matrix);
void turnPixelsOnMinutes(uint8_t start, uint8_t end, uint8_t row, std::array<std::array<bool, 11>, 11> &matrix);

#endif