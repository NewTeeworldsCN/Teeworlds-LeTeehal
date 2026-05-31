#include "localize_util.h"

#include <stdarg.h>
#include <teeuniverses/components/localization.h>

const char *LcLocalize(CLocalization *pLoc, const char *pLang, const char *pKey)
{
	if(!pKey)
		return "";
	if(!pLoc)
		return pKey;

	const char *pResult = pLoc->Localize(pLang, pKey);
	return pResult ? pResult : pKey;
}

void LcLocalizeCopy(char *pBuf, int BufSize, CLocalization *pLoc, const char *pLang, const char *pKey)
{
	if(!pBuf || BufSize <= 0)
		return;

	str_copy(pBuf, LcLocalize(pLoc, pLang, pKey), BufSize);
}

void LcFormatCopy(char *pBuf, int BufSize, CLocalization *pLoc, const char *pLang, const char *pFmt, ...)
{
	if(!pBuf || BufSize <= 0 || !pFmt)
		return;

	dynamic_string Buffer;
	va_list VarArgs;
	va_start(VarArgs, pFmt);
	if(pLoc)
		pLoc->Format_VL(Buffer, pLang, pFmt, VarArgs);
	else
		Buffer.append(pFmt);
	va_end(VarArgs);

	str_copy(pBuf, Buffer.buffer(), BufSize);
}
