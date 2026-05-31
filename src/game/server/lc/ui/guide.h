#ifndef GAME_SERVER_LC_GUIDE_H
#define GAME_SERVER_LC_GUIDE_H

class CGameContext;
class CLcTerminalMenu;
class CLocalization;
class CPlayer;

const char *LcStoreItemDesc(const char *pCmd);
const char *LcStoreItemDescShort(const char *pCmd);
const char *LcMonsterDesc(int Type);
const char *LcMonsterDescShort(int Type);
const char *LcScrapDesc(int ScrapID);
const char *LcScrapDescShort(int ScrapID);

void LcFillGuideStoreIndex(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang);
void LcFillGuideStoreCategory(CLcTerminalMenu *pMenu, int Category, CLocalization *pLoc, const char *pLang);
void LcFillGuideStoreDetail(CLcTerminalMenu *pMenu, int ItemIndex, CLocalization *pLoc, const char *pLang);
void LcFillGuideMonsterIndex(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang);
void LcFillGuideMonsterDetail(CLcTerminalMenu *pMenu, int Type, CLocalization *pLoc, const char *pLang);
void LcFillGuideScrapIndex(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang);
void LcFillGuideScrapTier(CLcTerminalMenu *pMenu, CGameContext *pGameServer, int Tier, CLocalization *pLoc, const char *pLang);
void LcFillGuideScrapDetail(CLcTerminalMenu *pMenu, CGameContext *pGameServer, int ScrapID, CLocalization *pLoc, const char *pLang);
void LcFillInventoryIndex(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang);
void LcFillInventoryUseList(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang);
void LcFillInventoryDropList(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang);
void LcFillInventoryDetail(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, int Slot, CLocalization *pLoc, const char *pLang);
void LcFillShipCargoIndex(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang);

void LcSendStoreGuide(CGameContext *pGameServer, int ClientID);
void LcSendMonsterGuide(CGameContext *pGameServer, int ClientID);
void LcSendScrapGuide(CGameContext *pGameServer, int ClientID);

#endif
