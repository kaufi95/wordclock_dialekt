/*

H E S C E I S T H L S
F Ü N F Z W A N Z I G
V I E R T E L Z E H N
F V O R L N A C H N S
H A L B H Z W E I N S
D R E I V S E C H S E
S I E B E N Z N E U N
F Ü N F E N A C H T E
V I E R N Z W Ö L F E
E L F Z E H N E U H R
       . . . .

*/

#include <Arduino.h>
#include <TimeLib.h>
#include <array>
#include "matrixUtils.h"

namespace deutsch
{
    void hour_one(bool s);
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
    void uhr();

    std::array<std::array<bool, 11>, 11> *matrix;

    // converts time into matrix
    void timeToPixels(time_t time, std::array<std::array<bool, 11>, 11> &_matrix)
    {
        matrix = &_matrix;
        uint8_t hours = hour(time);
        uint8_t minutes = minute(time);

        // show "Es ist" or "Es isch" randomized
        if (showEsIst(minutes))
        {
            // Es ist
            Serial.print("Es ist ");
            turnPixelsOn(1, 2, 0, *matrix);
            turnPixelsOn(5, 7, 0, *matrix);
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
            if (minutes >= 0 && minutes < 5)
            {
                hour_one(false);
            }
            else
            {
                hour_one(true);
            }
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

        if (minutes >= 0 && minutes < 5)
        {
            uhr();
        }

        turnPixelsOnMinutes(0, minutes % 5, 10, *matrix);

        Serial.print(" + ");
        Serial.print(minutes % 5);
        Serial.print(" min");
        Serial.println();
    }

    // ------------------------------------------------------------
    // numbers as labels

    void hour_one(bool s)
    {
        // oans/eins
        if (s)
        {
            Serial.print("eins");
            turnPixelsOn(7, 10, 4, *matrix);
        }
        else
        {
            Serial.print("ein");
            turnPixelsOn(7, 9, 4, *matrix);
        }
    }

    void hour_two()
    {
        // zwoa/zwei
        Serial.print("zwei");
        turnPixelsOn(5, 8, 4, *matrix);
    }

    void hour_three()
    {
        // drei
        Serial.print("drei");
        turnPixelsOn(0, 3, 5, *matrix);
    }

    void hour_four()
    {
        // vier/e/vier
        Serial.print("vier");
        turnPixelsOn(0, 3, 8, *matrix);
    }

    void hour_five()
    {
        // fünfe/fünf
        Serial.print("fünf");
        turnPixelsOn(0, 3, 7, *matrix);
    }

    void min_five()
    {
        // fünf/fünf
        Serial.print("fünf");
        turnPixelsOn(0, 3, 1, *matrix);
    }

    void hour_six()
    {
        // sechse/sechs
        Serial.print("sechs");
        turnPixelsOn(5, 9, 5, *matrix);
    }

    void hour_seven()
    {
        // siebne/sieben
        Serial.print("sieben");
        turnPixelsOn(0, 5, 6, *matrix);
    }

    void hour_eight()
    {
        // achte/acht
        Serial.print("acht");
        turnPixelsOn(6, 9, 7, *matrix);
    }

    void hour_nine()
    {
        // nüne/neun
        Serial.print("neun");
        turnPixelsOn(7, 10, 6, *matrix);
    }

    void hour_ten()
    {
        // zehne/zehn
        Serial.print("zehn");
        turnPixelsOn(3, 6, 9, *matrix);
    }

    void min_ten()
    {
        // zehn/zehn
        Serial.print("zehn");
        turnPixelsOn(7, 10, 2, *matrix);
    }

    void hour_eleven()
    {
        // elfe/elf
        Serial.print("elf");
        turnPixelsOn(0, 2, 9, *matrix);
    }

    void hour_twelve()
    {
        // zwölfe/zwölf
        Serial.print("zwölf");
        turnPixelsOn(5, 9, 8, *matrix);
    }

    void quarter()
    {
        // viertel
        Serial.print("viertel");
        turnPixelsOn(0, 6, 2, *matrix);
    }

    void twenty()
    {
        // zwanzig
        Serial.print("zwanzig");
        turnPixelsOn(4, 10, 1, *matrix);
    }

    // ------------------------------------------------------------

    void to()
    {
        // vor/vor
        Serial.print("vor");
        turnPixelsOn(1, 3, 3, *matrix);
    }

    void after()
    {
        // noch/nach
        Serial.print("nach");
        turnPixelsOn(5, 8, 3, *matrix);
    }

    void half()
    {
        // halb
        Serial.print("halb");
        turnPixelsOn(0, 3, 4, *matrix);
    }

    void uhr()
    {
        Serial.print(" uhr");
        turnPixelsOn(8, 10, 9, *matrix);
    }

} // namespace deutsch