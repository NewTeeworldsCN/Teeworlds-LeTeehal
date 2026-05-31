#ifndef GAME_SERVER_LC_LOCALIZE_UTIL_H
#define GAME_SERVER_LC_LOCALIZE_UTIL_H

class CLocalization;

const char *LcLocalize(CLocalization *pLoc, const char *pLang, const char *pKey);
void LcLocalizeCopy(char *pBuf, int BufSize, CLocalization *pLoc, const char *pLang, const char *pKey);
void LcFormatCopy(char *pBuf, int BufSize, CLocalization *pLoc, const char *pLang, const char *pFmt, ...);

#endif
