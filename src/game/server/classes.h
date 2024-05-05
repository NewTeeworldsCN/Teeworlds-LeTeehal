#pragma once

enum
{
    PLAYERCLASS_HUMAN = 0,
    PLAYERCLASS_HOARDINGBUG,
    PLAYERCLASS_BUNKERSPIDER,
    PLAYERCLASS_NUTCRACKER,
    PLAYERCLASS_THUMPER,
    PLAYERCLASS_SNAREFLEA,
    PLAYERCLASS_BRACKEN,

    NUM_PLAYERCLASS,
};

struct Classes
{
    int GetMaxPicks(int Class)
    {
        switch (Class)
        {
        case PLAYERCLASS_HUMAN:
            return 999;
        case PLAYERCLASS_HOARDINGBUG:
            return 1; // Only 1

        default:
            return 0;
        }
    }
};