#pragma once

enum
{
    PLAYERCLASS_HUMAN = 0,    // 调查员
    PLAYERCLASS_HOARDINGBUG,  // 囤积虫
    PLAYERCLASS_BUNKERSPIDER, // 蜘蛛
    PLAYERCLASS_NUTCRACKER,   // 胡桃夹子
    PLAYERCLASS_THUMPER,      // 半身鱼
    PLAYERCLASS_SNAREFLEA,    // 抱脸虫
    PLAYERCLASS_BRACKEN,      // 小黑

    NUM_PLAYERCLASS,
};

struct Classes
{
    int GetMaxPicks(int Class)
    {
        switch (Class)
        {
        case PLAYERCLASS_HUMAN:
            return 9999;
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

        case PLAYERCLASS_HOARDINGBUG:
            return 4;

        case PLAYERCLASS_BUNKERSPIDER:
            return 8;

        case PLAYERCLASS_NUTCRACKER:
            return 5;

        case PLAYERCLASS_THUMPER:
            return 4;

        case PLAYERCLASS_SNAREFLEA:
            return 3;

        case PLAYERCLASS_BRACKEN:
            return 1;

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