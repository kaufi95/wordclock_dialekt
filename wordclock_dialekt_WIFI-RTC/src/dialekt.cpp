/*

H E S C E I S C H O S
F Ü N F Z W A N Z I G
V I E R T E L Z E H N
F V O R L N O C H N S
H A L B H Z W O A N S
D R E I V S E C H S E
S I E B N E Z N Ü N E
F Ü N F E O A C H T E
V I E R E N Z E H N E
E L F E I Z W Ö L F E
       . . . .

*/

#include <Arduino.h>
#include <TimeLib.h>
#include <array>

namespace dialekt
{
    void resetMatrix();
    bool showEsIst(uint8_t minutes);
    void turnPixelsOn(uint8_t start, uint8_t end, uint8_t row);

    void hour_one();
    void hour_two();
    void hour_three();
    void hour_four();
    void hour_five();
    void min_five();
    void hour_six();
    void hour_seven();
    void hour_eight();
    void hour_nine();
    void hour_ten();
    void min_ten();
    void hour_eleven();
    void hour_twelve();
    void quarter();
    void twenty();
    void to();
    void after();
    void half();

    std::array<std::array<bool, 11>, 11> matrix = {false};

    // converts time into matrix
    std::array<std::array<bool, 11>, 11> timeToMatrix(time_t time)
    {
        resetMatrix();
        uint8_t hours = hour(time);
        uint8_t minutes = minute(time);

        // show "Es isch" randomized
        if (showEsIst(minutes))
        {
            Serial.print("Es isch ");
            turnPixelsOn(1, 2, 0);
            turnPixelsOn(5, 8, 0);
        }

        // show minutes
        if (minutes >= 5 && minutes < 10)
        {
            min_five();
            Serial.print(" ");
            after();
        }
        else if (minutes >= 10 && minutes < 15)
        {
            min_ten();
            Serial.print(" ");
            after();
        }
        else if (minutes >= 15 && minutes < 20)
        {
            quarter();
            Serial.print(" ");
            after();
        }
        else if (minutes >= 20 && minutes < 25)
        {
            twenty();
            Serial.print(" ");
            after();
        }
        else if (minutes >= 25 && minutes < 30)
        {
            min_five();
            Serial.print(" ");
            to();
            Serial.print(" ");
            half();
        }
        else if (minutes >= 30 && minutes < 35)
        {
            half();
        }
        else if (minutes >= 35 && minutes < 40)
        {
            min_five();
            Serial.print(" ");
            after();
            Serial.print(" ");
            half();
        }
        else if (minutes >= 40 && minutes < 45)
        {
            twenty();
            Serial.print(" ");
            to();
        }
        else if (minutes >= 45 && minutes < 50)
        {
            quarter();
            Serial.print(" ");
            to();
        }
        else if (minutes >= 50 && minutes < 55)
        {
            min_ten();
            Serial.print(" ");
            to();
        }
        else if (minutes >= 55 && minutes < 60)
        {
            min_five();
            Serial.print(" ");
            to();
        }

        Serial.print(" ");

        // convert hours to 12h format
        if (hours >= 12)
        {
            hours -= 12;
        }

        if (minutes >= 25)
        {
            hours++;
        }

        if (hours == 12)
        {
            hours = 0;
        }

        // show hours
        switch (hours)
        {
        case 0:
            // Zwölfe
            hour_twelve();
            break;
        case 1:
            // Oans
            hour_one();
            break;
        case 2:
            // Zwoa
            hour_two();
            break;
        case 3:
            // Drei
            hour_three();
            break;
        case 4:
            // Viere
            hour_four();
            break;
        case 5:
            // Fünfe
            hour_five();
            break;
        case 6:
            // Sechse
            hour_six();
            break;
        case 7:
            // Siebne
            hour_seven();
            break;
        case 8:
            // Achte
            hour_eight();
            break;
        case 9:
            // Nüne
            hour_nine();
            break;
        case 10:
            // Zehne
            hour_ten();
            break;
        case 11:
            // Elfe
            hour_eleven();
            break;
        }

        // pixels for minutes in additional row
        for (byte i = 1; i <= minutes % 5; i++)
        {
            matrix[10][i - 1] = true;
        }

        Serial.print(" + ");
        Serial.print(minutes % 5);
        Serial.print(" min");
        Serial.println();

        return matrix;
    }

    void resetMatrix()
    {
        for (auto &row : matrix)
        {
            row.fill(false);
        }
    }

    // determine if "es isch" is shown
    bool showEsIst(uint8_t minutes)
    {
        bool randomized = random() % 2 == 0;
        bool showEsIst = randomized || minutes % 30 < 5;
        return showEsIst;
    }

    // turn on pixels in matrix
    void turnPixelsOn(uint8_t start, uint8_t end, uint8_t row)
    {
        for (uint8_t i = start; i <= end; i++)
        {
            matrix[row][i] = true;
        }
    }

    // ------------------------------------------------------------
    // numbers as labels

    void hour_one()
    {
        // oans/eins
        Serial.print("oans");
        turnPixelsOn(7, 10, 4);
    }

    void hour_two()
    {
        // zwoa/zwei
        Serial.print("zwoa");
        turnPixelsOn(5, 8, 4);
    }

    void hour_three()
    {
        // drei
        Serial.print("drei");
        turnPixelsOn(0, 3, 5);
    }

    void hour_four()
    {
        // vier/e/vier
        Serial.print("viere");
        turnPixelsOn(0, 4, 8);
    }

    void hour_five()
    {
        // fünfe/fünf
        Serial.print("fünfe");
        turnPixelsOn(0, 4, 7);
    }

    void min_five()
    {
        // fünf/fünf
        Serial.print("fünf");
        turnPixelsOn(0, 3, 1);
    }

    void hour_six()
    {
        // sechse/sechs
        Serial.print("sechse");
        turnPixelsOn(5, 10, 5);
    }

    void hour_seven()
    {
        // siebne/sieben
        Serial.print("siebne");
        turnPixelsOn(0, 5, 6);
    }

    void hour_eight()
    {
        // achte/acht
        Serial.print("achte");
        turnPixelsOn(6, 10, 7);
    }

    void hour_nine()
    {
        // nüne/neun
        Serial.print("nüne");
        turnPixelsOn(7, 10, 6);
    }

    void hour_ten()
    {
        // zehne/zehn
        Serial.print("zehne");
        turnPixelsOn(6, 10, 8);
    }

    void min_ten()
    {
        // zehn/zehn
        Serial.print("zehn");
        turnPixelsOn(7, 10, 2);
    }

    void hour_eleven()
    {
        // elfe/elf
        Serial.print("elfe");
        turnPixelsOn(0, 3, 9);
    }

    void hour_twelve()
    {
        // zwölfe/zwölf
        Serial.print("zwölfe");
        turnPixelsOn(5, 10, 9);
    }

    void quarter()
    {
        // viertel
        Serial.print("viertel");
        turnPixelsOn(0, 6, 2);
    }

    void twenty()
    {
        // zwanzig
        Serial.print("zwanzig");
        turnPixelsOn(4, 10, 1);
    }

    // ------------------------------------------------------------

    void to()
    {
        // vor/vor
        Serial.print("vor");
        turnPixelsOn(1, 3, 3);
    }

    void after()
    {
        // noch/nach
        Serial.print("noch");
        turnPixelsOn(5, 8, 3);
    }

    void half()
    {
        // halb
        Serial.print("halb");
        turnPixelsOn(0, 3, 4);
    }

} // namespace dialekt