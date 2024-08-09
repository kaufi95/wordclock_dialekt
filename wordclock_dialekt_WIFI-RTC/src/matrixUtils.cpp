#include <Arduino.h>
#include <array>

// determine if "es isch" is shown
bool showEsIst(uint8_t minutes)
{
    bool randomized = random() % 2 == 0;
    return randomized || minutes % 30 < 5;
}

// reset matrix
void resetMatrix(std::array<std::array<bool, 11>, 11> matrix)
{
    for (auto &row : matrix)
    {
        row.fill(false);
    }
}

// turn on pixels in matrix
void turnPixelsOn(uint8_t start, uint8_t end, uint8_t row, std::array<std::array<bool, 11>, 11> matrix)
{
    for (uint8_t i = start; i <= end; i++)
    {
        matrix[row][i] = true;
    }
}

// pixels for minutes in additional row
void turnPixelsOnMinutes(uint8_t start, uint8_t end, uint8_t row, std::array<std::array<bool, 11>, 11> matrix)
{
    for (uint8_t i = start; i < end; i++)
    {
        matrix[row][i] = true;
    }
}