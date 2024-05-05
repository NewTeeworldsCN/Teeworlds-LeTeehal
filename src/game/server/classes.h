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

    int GetMaxHealth(int Class)
    {
        switch (Class)
        {
        case PLAYERCLASS_HUMAN:
            return 10;

        default:
            return 3;
        }
    }

    void GiveWeapons(class CCharacter *pChr)
    {
        switch (pChr->GetPlayer()->m_Class)
        {
        case PLAYERCLASS_NUTCRACKER:
            pChr->GiveWeapon(WEAPON_HAMMER, -1);
            pChr->GiveWeapon(WEAPON_SHOTGUN, -1);
            break;

        case PLAYERCLASS_BRACKEN:
            pChr->GiveWeapon(WEAPON_HAMMER, -1);
            pChr->GiveWeapon(WEAPON_NINJA, -1);
            break;
        
        default:
            pChr->GiveWeapon(WEAPON_HAMMER, -1);
            break;
        }
    }
};