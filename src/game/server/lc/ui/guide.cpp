#include "guide.h"

#include "../expedition/balance.h"
#include "../../core/gamecontext.h"
#include "../../core/gameworld.h"
#include "localize_util.h"
#include "terminal_actions.h"
#include "terminal_menu.h"
#include "../../core/player.h"
#include "../../scrap/scrap_info.h"
#include "../../entities/core/character.h"
#include "../../entities/lc/scrap.h"
#include "../../entities/lc/monster.h"

#include <teeuniverses/components/localization.h>

static void AddInfoLoc(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pKey)
{
	char aBuf[192];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	pMenu->AddInfo(aBuf);
}

static void AddActionLoc(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pCmd, const char *pKey)
{
	char aBuf[128];
	LcLocalizeCopy(aBuf, sizeof(aBuf), pLoc, pLang, pKey);
	pMenu->AddAction(pCmd, aBuf);
}

static void AddStoreDetailLines(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, const char *pCmd)
{
	if(str_comp(pCmd, "buy_time") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("为全队下趟班次延长 2 分钟。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("适合指标吃紧、需要多搜几间房的局面。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("在大厅 F3 终端或 ESC 投票均可购买。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("公司币全队共享，购买后立即扣款。"));
		return;
	}
	if(str_comp(pCmd, "buy_flash") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟出发时配发「停车标志」闪光弹。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("实地 ESC 投票选该物品，理由填 1 使用。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("在脚下产生范围冲击，眩晕附近怪物数秒。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("被围殴或需要抢路撤离时非常实用。"));
		return;
	}
	if(str_comp(pCmd, "buy_armor") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟全员出发时额外获得 3 点护甲。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("护甲可吸收伤害，适合高危险路线。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("与生命保障叠加，提高容错率。"));
		return;
	}
	if(str_comp(pCmd, "buy_health") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟全员出发时额外获得 4 点生命值。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("直接提高血上限，减少被秒杀风险。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("与防护服搭配可显著延长生存时间。"));
		return;
	}
	if(str_comp(pCmd, "buy_aircraft") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟增加 1 架飞行器库存（可多次购买叠加）。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("设施内 F3「飞行器」页部署，靠近自动上机。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("左键朝鼠标射击 | 右键按瞄准方向升降 | 空格下机。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("自动吸附附近废品(不含飞船内)，最多 16 件。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("飞回飞船自动卸货到船中心，计入你的贡献分。"));
		return;
	}
	if(str_comp(pCmd, "buy_shotgun") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发散弹枪及弹药。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("近距离爆发高，适合清走廊与房间。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("弹药有限，优先对付高威胁目标。"));
		return;
	}
	if(str_comp(pCmd, "buy_rifle") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发激光枪及弹药。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("中远程稳定输出，适合开阔区域。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("配合队形推进，避免落单被钩爪拖走。"));
		return;
	}
	if(str_comp(pCmd, "buy_grenade") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发榴弹及弹药。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("可炸开障碍或范围杀伤怪物。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("注意友伤，投掷前确认队友位置。"));
		return;
	}
	if(str_comp(pCmd, "buy_gun") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发手枪及弹药。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("基础远程武器，弹药较多。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("适合作为副武器或新手配装。"));
		return;
	}
	if(str_comp(pCmd, "buy_ninja") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发忍者刀。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("可冲刺攻击，机动性极强。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("适合绕后、穿洞或快速脱离。"));
		return;
	}
	if(str_comp(pCmd, "buy_medkit") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发急救包。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("理由 1 使用：回复生命并解除冻结/感染。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("被孢子蜥感染或倒地时的救命消耗品。"));
		return;
	}
	if(str_comp(pCmd, "buy_whistle") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发驱虫哨。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("理由 1 使用：短距离定向眩晕怪物。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("比闪光弹范围小，但更精准省资源。"));
		return;
	}
	if(str_comp(pCmd, "buy_soda") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发能量汽水。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("理由 1 使用：短期大幅提升移动速度。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("抢时间回收、跑毒或撤离时非常好用。"));
		return;
	}
	if(str_comp(pCmd, "buy_megaphone") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发扩音器。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("理由 1 使用：扫描并广播最近怪物坐标。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("相当于免费扫描，适合探路员。"));
		return;
	}
	if(str_comp(pCmd, "buy_boombox") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发音响。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("理由 1 使用：全图眩晕怪物数秒。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("终极控场道具，适合被围攻时翻盘。"));
		return;
	}
	if(str_comp(pCmd, "buy_remote") == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("下趟配发遥控器。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("理由 1 使用：随机召唤一只怪物。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("高风险高回报，可用于引怪或整蛊队友。"));
		return;
	}
}

static void AddMonsterDetailLines(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang, int Type)
{
	switch(Type)
	{
	case TYPE_PULLHANDLE:
		AddInfoLoc(pMenu, pLoc, pLang, _("会用钩爪把最近队员拖向自己。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("拖住后会周期性增加你的负重，越拖越慢。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("贴近目标后会进入自毁倒计时，但仍需尽快击杀。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：队友集火打断；闪光/哨子控场后拉开距离。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("具备钩爪移动，可能从意想不到的位置出现。"));
		break;
	case TYPE_SATIETY:
		AddInfoLoc(pMenu, pLoc, pLang, _("会主动追逐附近地上的废品。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("碰到废品后会连同废品一起消失（被吃掉）。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("约 60 秒后也会自行消散，威胁不大但会浪费战利品。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：先捡再撤；或用低价值废品引开。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("具备飞行能力，可越过部分地形追击。"));
		break;
	case TYPE_LEEK_BOX:
		AddInfoLoc(pMenu, pLoc, pLang, _("靠近后会感染玩家（韭菜盒子病毒）。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("感染后需尽快回飞船，否则会持续恶化直至任务失败。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("成功感染后会短暂停滞并进入自毁倒计时。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：保持距离远程击杀；备好急救包解控。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("可贴墙移动，在狭窄通道里更难瞄准。"));
		break;
	case TYPE_BUG:
		AddInfoLoc(pMenu, pLoc, pLang, _("平时躲在草丛中完全隐藏。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("玩家靠近时会突然伏击并钩爪攻击。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("贴近后会进入自毁倒计时，草丛区务必小心。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：扫描/扩音器定位；投掷物或队友引怪。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("隐藏状态下无法被普通攻击命中。"));
		break;
	case TYPE_FEAR:
		AddInfoLoc(pMenu, pLoc, pLang, _("被玩家注视时会定身无法行动。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("未被注视时会高速追击，极近距离可秒杀。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：一人盯眼、他人输出；切忌全员同时背对。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("血量归零会立即被消灭，优先集火。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("可贴墙移动，在拐角处尤其危险。"));
		break;
	case TYPE_HUNTER:
		AddInfoLoc(pMenu, pLoc, pLang, _("会主动追踪视野内的玩家。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("保持距离时会用手枪远程射击。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("威胁等级高，应优先集火消灭。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：利用掩体；近战突脸打断射击。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("具备飞行能力，可从上方发起攻击。"));
		break;
	case TYPE_BOMBER:
		AddInfoLoc(pMenu, pLoc, pLang, _("移动较慢，但贴近玩家时会自爆。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("被击杀时也会爆炸，对周围造成范围伤害。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：远程点杀；切勿贴身补刀或围殴。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("爆炸可误伤队友，注意站位。"));
		break;
	case TYPE_LEECH:
		AddInfoLoc(pMenu, pLoc, pLang, _("会用钩爪牵制目标并拉近。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("近距离持续吸取生命值。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("贴近后会进入自毁倒计时，被吸时很难挣脱。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：眩晕/击退后拉开距离集火。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("具备钩爪移动，可能从下方或侧翼出现。"));
		break;
	case TYPE_STALKER:
		AddInfoLoc(pMenu, pLoc, pLang, _("高速地面追击，比猛禽更快但血量更低。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("会直线追最近玩家，在开阔地形尤其危险。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("对策：利用掩体与高低差；优先远程点杀。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("血量较低，集火可快速清除。"));
		break;
	default:
		AddInfoLoc(pMenu, pLoc, pLang, _("未知威胁，保持队形并尽快撤离。"));
		break;
	}
}

static void AddScrapDetailLines(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang, int ScrapID)
{
	const ScrapInfo &Info = pGameServer->ScrapInfo()->m_aScrapInfo[ScrapID];
	char aRange[96];
	int MinValue = Info.m_Value.x;
	int MaxValue = Info.m_Value.y;
	int MinWeight = Info.m_Weight.x;
	int MaxWeight = Info.m_Weight.y;
	LcFormatCopy(aRange, sizeof(aRange), pLoc, pLang, _("价值 {int:minv}~{int:maxv} 元 | 重量 {int:minw}~{int:maxw} 镑"),
		"minv", &MinValue, "maxv", &MaxValue, "minw", &MinWeight, "maxw", &MaxWeight);
	pMenu->AddInfo(aRange);
	{
		char aDesc[192];
		LcLocalizeCopy(aDesc, sizeof(aDesc), pLoc, pLang, LcScrapDesc(ScrapID));
		pMenu->AddInfo(aDesc);
	}
	pMenu->AddInfo("");

	switch(ScrapID)
	{
	case SCRAP_L1_TOOTHPASTE:
		AddInfoLoc(pMenu, pLoc, pLang, _("公司回收的「瓶盖」，实为过期牙膏。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用结果随机：可能回血，也可能扣血。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("便宜轻巧，适合赌运气或凑重量。"));
		break;
	case SCRAP_L1_HAIRBRUSH:
		AddInfoLoc(pMenu, pLoc, pLang, _("一把旧发刷，使用后直接消耗。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("梳一梳提神——具体效果看运气。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("重量低，紧急时刻可扔掉减负。"));
		break;
	case SCRAP_L1_FLASHBANG:
		AddInfoLoc(pMenu, pLoc, pLang, _("停车标志改装的闪光弹。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("在脚下释放冲击波，眩晕周围怪物。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("团队控场核心，留到被围时使用。"));
		break;
	case SCRAP_L1_PICKLES:
		AddInfoLoc(pMenu, pLoc, pLang, _("一罐可疑泡菜，气味能熏退队友。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后随机回血或扣血。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("别在血量低时赌它。"));
		break;
	case SCRAP_L1_FISH:
		AddInfoLoc(pMenu, pLoc, pLang, _("塑料玩具鱼，毫无营养。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后随机回血或扣血。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("常见 L1 废品，价值一般。"));
		break;
	case SCRAP_L1_METALSHEET:
		AddInfoLoc(pMenu, pLoc, pLang, _("大型螺栓/金属板碎片。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后获得随机护甲值。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("无护甲时优先使用，比赌血稳。"));
		break;
	case SCRAP_L1_SODA:
		AddInfoLoc(pMenu, pLoc, pLang, _("高糖能量汽水，喝完腿软之前跑得飞快。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("短期大幅加速，适合撤离或抢时间。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("效果结束后注意别被追上。"));
		break;
	case SCRAP_L1_WHISTLE:
		AddInfoLoc(pMenu, pLoc, pLang, _("驱虫哨，声音尖锐。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("短距离眩晕最近的一只怪物。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("比闪光弹省，但范围小很多。"));
		break;
	case SCRAP_L2_TOY:
		AddInfoLoc(pMenu, pLoc, pLang, _("会说话的机器人玩具，电池漏液。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后随机回血或扣血。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("L2 废品，价值高于 L1。"));
		break;
	case SCRAP_L2_CUBE:
		AddInfoLoc(pMenu, pLoc, pLang, _("魔方，据说能锻炼脑力。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后稳定回复生命值。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("可靠的回血手段，值得保留。"));
		break;
	case SCRAP_L2_SIGN:
		AddInfoLoc(pMenu, pLoc, pLang, _("路牌，可装备为近战武器。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("锤击伤害高于普通拳头。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("缺枪时可当防身工具。"));
		break;
	case SCRAP_L2_PILL:
		AddInfoLoc(pMenu, pLoc, pLang, _("未标记的药瓶，别问是什么。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后随机回血或扣血。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("血量满时不必冒险。"));
		break;
	case SCRAP_L2_OLDPHONE:
		AddInfoLoc(pMenu, pLoc, pLang, _("老式电话，还能拨号。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后向全队广播你的坐标。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("迷路或需要支援时非常有用。"));
		break;
	case SCRAP_L2_REMOTE:
		AddInfoLoc(pMenu, pLoc, pLang, _("电视遥控器，按钮大多已失灵。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后随机召唤一只怪物。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("作死神器，请在安全位置再按。"));
		break;
	case SCRAP_L2_MEDKIT:
		AddInfoLoc(pMenu, pLoc, pLang, _("标准急救包。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("回血并解除冻结/感染等异常。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("团队必备，感染时立刻使用。"));
		break;
	case SCRAP_L2_MEGAPHONE:
		AddInfoLoc(pMenu, pLoc, pLang, _("扩音器，电池勉强够用。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("扫描并标记最近怪物的位置。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("相当于一次免费扫描。"));
		break;
	case SCRAP_L3_MAGIC7BALL:
		AddInfoLoc(pMenu, pLoc, pLang, _("魔法七号球，回答总是「再试一次」。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后获得忍者冲刺能力。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("高价值废品，别轻易放下。"));
		break;
	case SCRAP_L3_SHOTGUN:
		AddInfoLoc(pMenu, pLoc, pLang, _("破损散弹枪，还能打两发。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后获得 2 发散弹弹药。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("遇首领或怪海时极其实用。"));
		break;
	case SCRAP_L3_GOLDBAR:
		AddInfoLoc(pMenu, pLoc, pLang, _("金条，质检员看见会两眼放光。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后直接增加全队公司币。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("见到就捡，别犹豫。"));
		break;
	case SCRAP_L3_LAMP:
		AddInfoLoc(pMenu, pLoc, pLang, _("绚丽台灯，玻璃极易碎。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后获得护甲。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("又值钱又能保命，优先带回飞船。"));
		break;
	case SCRAP_L3_CASHREGISTER:
		AddInfoLoc(pMenu, pLoc, pLang, _("收银机，抽屉卡住了。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后随机吐出公司币。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("带回船上也能卖好价钱。"));
		break;
	case SCRAP_L3_TEETH:
		AddInfoLoc(pMenu, pLoc, pLang, _("一副假牙，咬合力惊人。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用会伤到自己——纯整蛊。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("除非想害队友，否则别按理由 1。"));
		break;
	case SCRAP_L3_LUCKYCAT:
		AddInfoLoc(pMenu, pLoc, pLang, _("招财猫，爪子会动。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后下一件捡到的废品价值翻倍。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("先用猫，再捡 L3 大件，收益最大化。"));
		break;
	case SCRAP_L3_BOOMBOX:
		AddInfoLoc(pMenu, pLoc, pLang, _("老式音响，低音炮仍能用。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("使用后全图怪物眩晕数秒。"));
		AddInfoLoc(pMenu, pLoc, pLang, _("终极控场，一按扭转战局。"));
		break;
	default:
		AddInfoLoc(pMenu, pLoc, pLang, _("锤子拾取，ESC/F3 理由 1 使用，理由空放下。"));
		break;
	}
}

static bool StoreItemInCategory(int ItemIndex, int Category)
{
	switch(Category)
	{
	case 0:
		return ItemIndex == 0 || ItemIndex == 2 || ItemIndex == 3 || ItemIndex == 4;
	case 1:
		return ItemIndex >= 5 && ItemIndex <= 9;
	case 2:
		return ItemIndex == 1 || ItemIndex >= 10;
	default:
		return false;
	}
}

static const int s_aMonsterTypes[] = {
	TYPE_PULLHANDLE, TYPE_SATIETY, TYPE_LEEK_BOX, TYPE_BUG,
	TYPE_FEAR, TYPE_HUNTER, TYPE_BOMBER, TYPE_LEECH, TYPE_STALKER,
};

const char *LcStoreItemDesc(const char *pCmd)
{
	if(!pCmd)
		return "";

	if(str_comp(pCmd, "buy_time") == 0)
		return _("下趟全队班次 +2 分钟");
	if(str_comp(pCmd, "buy_flash") == 0)
		return _("配发停车标志，投票理由1使用，范围眩晕怪物");
	if(str_comp(pCmd, "buy_armor") == 0)
		return _("下趟出发时 +3 护甲");
	if(str_comp(pCmd, "buy_health") == 0)
		return _("下趟出发时 +4 生命值");
	if(str_comp(pCmd, "buy_shotgun") == 0)
		return _("下趟配发散弹枪与弹药");
	if(str_comp(pCmd, "buy_rifle") == 0)
		return _("下趟配发激光枪与弹药");
	if(str_comp(pCmd, "buy_grenade") == 0)
		return _("下趟配发榴弹与弹药");
	if(str_comp(pCmd, "buy_gun") == 0)
		return _("下趟配发手枪与弹药");
	if(str_comp(pCmd, "buy_ninja") == 0)
		return _("下趟配发忍者刀，可冲刺攻击");
	if(str_comp(pCmd, "buy_medkit") == 0)
		return _("配发急救包，理由1使用回血并解控");
	if(str_comp(pCmd, "buy_whistle") == 0)
		return _("配发驱虫哨，理由1使用短距眩晕");
	if(str_comp(pCmd, "buy_soda") == 0)
		return _("配发汽水，理由1使用短期加速");
	if(str_comp(pCmd, "buy_megaphone") == 0)
		return _("配发扩音器，理由1扫描最近怪物");
	if(str_comp(pCmd, "buy_boombox") == 0)
		return _("配发音响，理由1全图眩晕怪物");
	if(str_comp(pCmd, "buy_remote") == 0)
		return _("配发遥控器，理由1随机召唤怪物");
	if(str_comp(pCmd, "buy_aircraft") == 0)
		return _("下趟飞行器库存+1，设施内部署；可吸附废品、回船卸货");
	return "";
}

const char *LcStoreItemDescShort(const char *pCmd)
{
	if(!pCmd)
		return "";

	if(str_comp(pCmd, "buy_time") == 0)
		return _("下趟+2分钟");
	if(str_comp(pCmd, "buy_flash") == 0)
		return _("配发闪光弹");
	if(str_comp(pCmd, "buy_armor") == 0)
		return _("下趟+3甲");
	if(str_comp(pCmd, "buy_health") == 0)
		return _("下趟+4血");
	if(str_comp(pCmd, "buy_shotgun") == 0)
		return _("下趟散弹枪");
	if(str_comp(pCmd, "buy_rifle") == 0)
		return _("下趟激光枪");
	if(str_comp(pCmd, "buy_grenade") == 0)
		return _("下趟榴弹");
	if(str_comp(pCmd, "buy_gun") == 0)
		return _("下趟手枪");
	if(str_comp(pCmd, "buy_ninja") == 0)
		return _("下趟忍者刀");
	if(str_comp(pCmd, "buy_medkit") == 0)
		return _("配发急救包");
	if(str_comp(pCmd, "buy_whistle") == 0)
		return _("配发驱虫哨");
	if(str_comp(pCmd, "buy_soda") == 0)
		return _("配发汽水");
	if(str_comp(pCmd, "buy_megaphone") == 0)
		return _("配发扩音器");
	if(str_comp(pCmd, "buy_boombox") == 0)
		return _("配发音响");
	if(str_comp(pCmd, "buy_remote") == 0)
		return _("配发遥控器");
	if(str_comp(pCmd, "buy_aircraft") == 0)
		return _("下趟飞行器+1");
	return "";
}

const char *LcMonsterDesc(int Type)
{
	switch(Type)
	{
	case TYPE_PULLHANDLE: return _("钩爪拖近玩家并加重；贴近后自毁，血量归零立即死亡");
	case TYPE_SATIETY: return _("追逐废品并吞食；约60秒自行消散，也可被击杀");
	case TYPE_LEEK_BOX: return _("靠近感染玩家；成功后短暂停滞并自毁，需尽快回飞船");
	case TYPE_BUG: return _("草丛隐藏伏击；隐藏时免伤，贴近后自毁，需扫描或引怪");
	case TYPE_FEAR: return _("被注视定身，未注视时追击并近身秒杀；血量归零立即死亡");
	case TYPE_HUNTER: return _("追踪并远程手枪射击；具备飞行，优先集火");
	case TYPE_BOMBER: return _("移动慢，贴近或死亡时爆炸；远程击杀，避免围殴");
	case TYPE_LEECH: return _("钩爪拉近并吸血；贴近后自毁，需控场后集火");
	case TYPE_STALKER: return _("高速地面追击；血量低但极难甩掉，优先远程击杀");
	default: return _("未知威胁，保持队形并尽快撤离");
	}
}

const char *LcMonsterDescShort(int Type)
{
	switch(Type)
	{
	case TYPE_PULLHANDLE: return _("钩爪+加重·自毁");
	case TYPE_SATIETY: return _("吃废品·限时");
	case TYPE_LEEK_BOX: return _("感染·自毁");
	case TYPE_BUG: return _("草丛伏击·免伤");
	case TYPE_FEAR: return _("注视定身·未看秒杀");
	case TYPE_HUNTER: return _("远程射击·飞行");
	case TYPE_BOMBER: return _("死亡爆炸");
	case TYPE_LEECH: return _("钩爪吸血·自毁");
	case TYPE_STALKER: return _("高速追击·低血");
	default: return _("威胁");
	}
}

const char *LcScrapDesc(int ScrapID)
{
	switch(ScrapID)
	{
	case SCRAP_L1_TOOTHPASTE: return _("理由1：随机回血或扣血");
	case SCRAP_L1_HAIRBRUSH: return _("理由1：消耗品，梳头发提神");
	case SCRAP_L1_FLASHBANG: return _("理由1：范围眩晕怪物（闪光弹）");
	case SCRAP_L1_PICKLES: return _("理由1：随机回血或扣血");
	case SCRAP_L1_FISH: return _("理由1：随机回血或扣血");
	case SCRAP_L1_METALSHEET: return _("理由1：获得随机护甲");
	case SCRAP_L1_SODA: return _("理由1：短期移动加速");
	case SCRAP_L1_WHISTLE: return _("理由1：短距离眩晕怪物");
	case SCRAP_L2_TOY: return _("理由1：随机回血或扣血");
	case SCRAP_L2_CUBE: return _("理由1：稳定回复生命");
	case SCRAP_L2_SIGN: return _("理由1：装备路牌，锤击伤害更高");
	case SCRAP_L2_PILL: return _("理由1：随机回血或扣血");
	case SCRAP_L2_OLDPHONE: return _("理由1：向全队广播你的坐标");
	case SCRAP_L2_REMOTE: return _("理由1：随机召唤一只怪物");
	case SCRAP_L2_MEDKIT: return _("理由1：回血并解除冻结");
	case SCRAP_L2_MEGAPHONE: return _("理由1：扫描最近怪物位置");
	case SCRAP_L3_MAGIC7BALL: return _("理由1：获得忍者冲刺能力");
	case SCRAP_L3_SHOTGUN: return _("理由1：获得2发散弹");
	case SCRAP_L3_GOLDBAR: return _("理由1：直接增加公司币");
	case SCRAP_L3_LAMP: return _("理由1：获得护甲");
	case SCRAP_L3_CASHREGISTER: return _("理由1：随机吐出公司币");
	case SCRAP_L3_TEETH: return _("理由1：作死，会伤到自己");
	case SCRAP_L3_LUCKYCAT: return _("理由1：下一件废品价值翻倍");
	case SCRAP_L3_BOOMBOX: return _("理由1：全图眩晕怪物数秒");
	default: return _("理由1使用，理由空放下");
	}
}

const char *LcScrapDescShort(int ScrapID)
{
	switch(ScrapID)
	{
	case SCRAP_L1_TOOTHPASTE: return _("随机回血");
	case SCRAP_L1_HAIRBRUSH: return _("消耗");
	case SCRAP_L1_FLASHBANG: return _("眩晕怪物");
	case SCRAP_L1_PICKLES: return _("随机回血");
	case SCRAP_L1_FISH: return _("随机回血");
	case SCRAP_L1_METALSHEET: return _("加护甲");
	case SCRAP_L1_SODA: return _("加速");
	case SCRAP_L1_WHISTLE: return _("短眩晕");
	case SCRAP_L2_TOY: return _("随机回血");
	case SCRAP_L2_CUBE: return _("回血");
	case SCRAP_L2_SIGN: return _("路牌武器");
	case SCRAP_L2_PILL: return _("随机回血");
	case SCRAP_L2_OLDPHONE: return _("广播坐标");
	case SCRAP_L2_REMOTE: return _("召唤怪物");
	case SCRAP_L2_MEDKIT: return _("回血解控");
	case SCRAP_L2_MEGAPHONE: return _("扫怪物");
	case SCRAP_L3_MAGIC7BALL: return _("忍者冲刺");
	case SCRAP_L3_SHOTGUN: return _("2发散弹");
	case SCRAP_L3_GOLDBAR: return _("加公司币");
	case SCRAP_L3_LAMP: return _("加护甲");
	case SCRAP_L3_CASHREGISTER: return _("吐钱");
	case SCRAP_L3_TEETH: return _("自伤");
	case SCRAP_L3_LUCKYCAT: return _("下件双倍");
	case SCRAP_L3_BOOMBOX: return _("全图眩晕");
	default: return _("可消耗");
	}
}

void LcFillGuideStoreIndex(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang)
{
	AddInfoLoc(pMenu, pLoc, pLang, _("【商店说明】选分类查看详情"));
	pMenu->AddInfo("");
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 9", _("☞ 保障与班次"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 11", _("☞ 下趟武器"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 12", _("☞ 实地消耗品"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 3", _("⏎ 返回图鉴"));
}

void LcFillGuideStoreCategory(CLcTerminalMenu *pMenu, int Category, CLocalization *pLoc, const char *pLang)
{
	int Count = 0;
	const SLcStoreItem *pItems = LcGetStoreItems(&Count);

	switch(Category)
	{
	case 0:
		AddInfoLoc(pMenu, pLoc, pLang, _("【保障与班次】滚轮选条目"));
		break;
	case 1:
		AddInfoLoc(pMenu, pLoc, pLang, _("【下趟武器】滚轮选条目"));
		break;
	default:
		AddInfoLoc(pMenu, pLoc, pLang, _("【实地消耗品】滚轮选条目"));
		break;
	}
	pMenu->AddInfo("");

	for(int i = 0; i < Count; i++)
	{
		if(!StoreItemInCategory(i, Category))
			continue;

		char aCmd[32];
		str_format(aCmd, sizeof(aCmd), "lcm_goto %d", LC_PAGE_GUIDE_STORE_DETAIL_BASE + i);
		dynamic_string aLabel;
		pLoc->Format_L(aLabel, pLang, _("☞ {lstr:name} [{int:cost}币] {lstr:desc}"),
			"name", LcStoreItemName(pItems[i].m_pCmd), "cost", &pItems[i].m_Cost,
			"desc", LcStoreItemDescShort(pItems[i].m_pCmd));
		char aBuf[128];
		str_copy(aBuf, aLabel.buffer(), sizeof(aBuf));
		pMenu->AddAction(aCmd, aBuf);
	}

	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 4", _("⏎ 返回商店目录"));
}

void LcFillGuideStoreDetail(CLcTerminalMenu *pMenu, int ItemIndex, CLocalization *pLoc, const char *pLang)
{
	int Count = 0;
	const SLcStoreItem *pItems = LcGetStoreItems(&Count);
	if(ItemIndex < 0 || ItemIndex >= Count)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("无效条目"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 4", _("⏎ 返回"));
		return;
	}

	const SLcStoreItem &Item = pItems[ItemIndex];
	char aTitle[128];
	LcFormatCopy(aTitle, sizeof(aTitle), pLoc, pLang, _("【{lstr:name}】 {int:cost} 币"),
		"name", LcStoreItemName(Item.m_pCmd), "cost", &Item.m_Cost);
	pMenu->AddInfo(aTitle);
	{
		char aDesc[192];
		LcLocalizeCopy(aDesc, sizeof(aDesc), pLoc, pLang, LcStoreItemDesc(Item.m_pCmd));
		pMenu->AddInfo(aDesc);
	}
	pMenu->AddInfo("");
	AddStoreDetailLines(pMenu, pLoc, pLang, Item.m_pCmd);

	char aBackCmd[32];
	str_format(aBackCmd, sizeof(aBackCmd), "lcm_goto %d",
		ItemIndex == 0 || ItemIndex == 2 || ItemIndex == 3 || ItemIndex == 4 ? LC_PAGE_GUIDE_STORE_SUPPLIES
		: ItemIndex >= 5 && ItemIndex <= 9 ? LC_PAGE_GUIDE_STORE_WEAPONS
		: LC_PAGE_GUIDE_STORE_FIELD);
	AddActionLoc(pMenu, pLoc, pLang, aBackCmd, _("⏎ 返回分类"));
}

void LcFillGuideMonsterIndex(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang)
{
	AddInfoLoc(pMenu, pLoc, pLang, _("【怪物图鉴】滚轮选条目看详情"));
	pMenu->AddInfo("");
	AddInfoLoc(pMenu, pLoc, pLang, _("血量(护甲+生命)归零后会立即消灭；部分怪物有自毁计时"));

	for(int i = 0; i < (int)(sizeof(s_aMonsterTypes) / sizeof(s_aMonsterTypes[0])); i++)
	{
		char aCmd[32];
		str_format(aCmd, sizeof(aCmd), "lcm_goto %d", LC_PAGE_GUIDE_MONSTER_BASE + i);
		dynamic_string aLabel;
		pLoc->Format_L(aLabel, pLang, _("☞ {int:id} {lstr:name} | {lstr:desc}"),
			"id", &i, "name", LcMonsterName(s_aMonsterTypes[i]), "desc", LcMonsterDescShort(s_aMonsterTypes[i]));
		char aBuf[128];
		str_copy(aBuf, aLabel.buffer(), sizeof(aBuf));
		pMenu->AddAction(aCmd, aBuf);
	}

	pMenu->AddInfo("");
	AddInfoLoc(pMenu, pLoc, pLang, _("每5班次可能出现首领"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 3", _("⏎ 返回图鉴"));
}

void LcFillGuideMonsterDetail(CLcTerminalMenu *pMenu, int TypeIndex, CLocalization *pLoc, const char *pLang)
{
	if(TypeIndex < 0 || TypeIndex >= (int)(sizeof(s_aMonsterTypes) / sizeof(s_aMonsterTypes[0])))
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("无效怪物"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 5", _("⏎ 返回"));
		return;
	}

	const int Type = s_aMonsterTypes[TypeIndex];
	char aTitle[96];
	LcFormatCopy(aTitle, sizeof(aTitle), pLoc, pLang, _("【{int:id} {lstr:name}】"),
		"id", &TypeIndex, "name", LcMonsterName(Type));
	pMenu->AddInfo(aTitle);
	{
		char aDesc[192];
		LcLocalizeCopy(aDesc, sizeof(aDesc), pLoc, pLang, LcMonsterDesc(Type));
		pMenu->AddInfo(aDesc);
	}
	pMenu->AddInfo("");
	AddMonsterDetailLines(pMenu, pLoc, pLang, Type);
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 5", _("⏎ 返回怪物列表"));
}

void LcFillGuideScrapIndex(CLcTerminalMenu *pMenu, CLocalization *pLoc, const char *pLang)
{
	AddInfoLoc(pMenu, pLoc, pLang, _("【废品说明】选等级分类"));
	pMenu->AddInfo("");
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 13", _("☞ L1 常见废品 (8种)"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 14", _("☞ L2 稀有废品 (8种)"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 15", _("☞ L3 高价值废品 (8种)"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 3", _("⏎ 返回图鉴"));
}

void LcFillGuideScrapTier(CLcTerminalMenu *pMenu, CGameContext *pGameServer, int Tier, CLocalization *pLoc, const char *pLang)
{
	int Start = 0;
	int End = END_SCRAP_L1;
	switch(Tier)
	{
	case 0:
		AddInfoLoc(pMenu, pLoc, pLang, _("【L1 常见废品】滚轮选条目"));
		Start = 0;
		End = END_SCRAP_L1;
		break;
	case 1:
		AddInfoLoc(pMenu, pLoc, pLang, _("【L2 稀有废品】滚轮选条目"));
		Start = END_SCRAP_L1;
		End = END_SCRAP_L2;
		break;
	default:
		AddInfoLoc(pMenu, pLoc, pLang, _("【L3 高价值废品】滚轮选条目"));
		Start = END_SCRAP_L2;
		End = END_SCRAP_L3;
		break;
	}
	pMenu->AddInfo("");

	for(int i = Start; i < End; i++)
	{
		char aCmd[32];
		str_format(aCmd, sizeof(aCmd), "lcm_goto %d", LC_PAGE_GUIDE_SCRAP_DETAIL_BASE + i);
		dynamic_string aLabel;
		pLoc->Format_L(aLabel, pLang, _("☞ {lstr:name} | {lstr:desc}"),
			"name", pGameServer->ScrapInfo()->GetScrapName(i),
			"desc", LcScrapDescShort(i));
		char aBuf[128];
		str_copy(aBuf, aLabel.buffer(), sizeof(aBuf));
		pMenu->AddAction(aCmd, aBuf);
	}

	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 6", _("⏎ 返回废品目录"));
}

void LcFillGuideScrapDetail(CLcTerminalMenu *pMenu, CGameContext *pGameServer, int ScrapID, CLocalization *pLoc, const char *pLang)
{
	if(ScrapID < 0 || ScrapID >= NUM_SCRAPS)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("无效废品"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 6", _("⏎ 返回"));
		return;
	}

	char aTitle[96];
	LcFormatCopy(aTitle, sizeof(aTitle), pLoc, pLang, _("【{lstr:name}】"),
		"name", pGameServer->ScrapInfo()->GetScrapName(ScrapID));
	pMenu->AddInfo(aTitle);
	AddScrapDetailLines(pMenu, pGameServer, pLoc, pLang, ScrapID);

	char aBackCmd[32];
	int BackPage = LC_PAGE_GUIDE_SCRAP_L1;
	if(ScrapID >= END_SCRAP_L2)
		BackPage = LC_PAGE_GUIDE_SCRAP_L3;
	else if(ScrapID >= END_SCRAP_L1)
		BackPage = LC_PAGE_GUIDE_SCRAP_L2;
	str_format(aBackCmd, sizeof(aBackCmd), "lcm_goto %d", BackPage);
	AddActionLoc(pMenu, pLoc, pLang, aBackCmd, _("⏎ 返回分类列表"));
}

static void LcFillInventorySummary(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang)
{
	int Lb = pP->m_AddedWeight;
	int Value = 0;
	for(int i = 0; i < pP->m_vScraps.size(); i++)
	{
		if(pP->m_vScraps[i])
		{
			Lb += pP->m_vScraps[i]->m_Weight;
			Value += pP->m_vScraps[i]->m_Value;
		}
	}
	pP->m_Weight = Lb;

	int Used = (int)pP->m_vScraps.size();
	int MaxSlots = GC_MAX_SCRAP_SLOTS;
	char aSummary[128];
	LcFormatCopy(aSummary, sizeof(aSummary), pLoc, pLang, _("总重量:{int:lb}镑 价值:{int:value}元 | {int:used}/{int:max} 格"),
		"lb", &Lb, "value", &Value, "used", &Used, "max", &MaxSlots);
	AddInfoLoc(pMenu, pLoc, pLang, pP->GetCharacter() && pP->GetCharacter()->m_InShip
		? _("☪ 个人背包")
		: _("☪ 背包"));
	pMenu->AddInfo(aSummary);
}

void LcFillInventoryIndex(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang)
{
	LcFillInventorySummary(pMenu, pGameServer, pP, pLoc, pLang);
	pMenu->AddInfo("");

	if(pP->m_vScraps.size() == 0)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("背包为空"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		return;
	}

	AddInfoLoc(pMenu, pLoc, pLang, _("请先选择操作方式"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 17", _("☞ 使用物品"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 18", _("☞ 放下物品"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
}

void LcFillInventoryUseList(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang)
{
	LcFillInventorySummary(pMenu, pGameServer, pP, pLoc, pLang);
	AddInfoLoc(pMenu, pLoc, pLang, _("滚轮选物品，左键确认使用"));
	pMenu->AddInfo("");

	int NumItems = 0;
	for(int i = 0; i < pP->m_vScraps.size(); i++)
	{
		if(!pP->m_vScraps[i])
			continue;

		NumItems++;
		char aCmd[32];
		char aLabel[128];
		int SlotNum = i + 1;
		int Value = pP->m_vScraps[i]->m_Value;
		int Weight = pP->m_vScraps[i]->m_Weight;
		str_format(aCmd, sizeof(aCmd), "lcm_scrap use %d", i);
		LcFormatCopy(aLabel, sizeof(aLabel), pLoc, pLang, _("☞ #{int:slot} {lstr:name} {int:value}元/{int:weight}镑"),
			"slot", &SlotNum, "name", pGameServer->ScrapInfo()->GetScrapName(pP->m_vScraps[i]->m_ScrapID),
			"value", &Value, "weight", &Weight);
		pMenu->AddAction(aCmd, aLabel);
	}

	if(NumItems == 0)
		AddInfoLoc(pMenu, pLoc, pLang, _("背包为空"));

	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 8", _("⏎ 返回操作选择"));
}

void LcFillInventoryDropList(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, CLocalization *pLoc, const char *pLang)
{
	LcFillInventorySummary(pMenu, pGameServer, pP, pLoc, pLang);
	AddInfoLoc(pMenu, pLoc, pLang, _("滚轮选物品，左键确认放下"));
	pMenu->AddInfo("");

	int NumItems = 0;
	for(int i = 0; i < pP->m_vScraps.size(); i++)
	{
		if(!pP->m_vScraps[i])
			continue;

		NumItems++;
		char aCmd[32];
		char aLabel[128];
		int SlotNum = i + 1;
		int Value = pP->m_vScraps[i]->m_Value;
		int Weight = pP->m_vScraps[i]->m_Weight;
		str_format(aCmd, sizeof(aCmd), "lcm_scrap drop %d", i);
		LcFormatCopy(aLabel, sizeof(aLabel), pLoc, pLang, _("☞ #{int:slot} {lstr:name} {int:value}元/{int:weight}镑"),
			"slot", &SlotNum, "name", pGameServer->ScrapInfo()->GetScrapName(pP->m_vScraps[i]->m_ScrapID),
			"value", &Value, "weight", &Weight);
		pMenu->AddAction(aCmd, aLabel);
	}

	if(NumItems == 0)
		AddInfoLoc(pMenu, pLoc, pLang, _("背包为空"));

	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 8", _("⏎ 返回操作选择"));
}

void LcFillInventoryDetail(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CPlayer *pP, int Slot, CLocalization *pLoc, const char *pLang)
{
	if(Slot < 0 || Slot >= (int)pP->m_vScraps.size() || !pP->m_vScraps[Slot])
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("该格已空或无效"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 8", _("⏎ 返回背包"));
		return;
	}

	Scrap *pScrap = pP->m_vScraps[Slot];
	char aTitle[128];
	int SlotNum = Slot + 1;
	LcFormatCopy(aTitle, sizeof(aTitle), pLoc, pLang, _("【#{int:slot} {lstr:name}】 {int:value}元 / {int:weight}镑"),
		"slot", &SlotNum, "name", pGameServer->ScrapInfo()->GetScrapName(pScrap->m_ScrapID),
		"value", &pScrap->m_Value, "weight", &pScrap->m_Weight);
	pMenu->AddInfo(aTitle);
	{
		char aDesc[192];
		LcLocalizeCopy(aDesc, sizeof(aDesc), pLoc, pLang, LcScrapDesc(pScrap->m_ScrapID));
		pMenu->AddInfo(aDesc);
		LcLocalizeCopy(aDesc, sizeof(aDesc), pLoc, pLang, LcScrapDescShort(pScrap->m_ScrapID));
		pMenu->AddInfo(aDesc);
	}
	AddInfoLoc(pMenu, pLoc, pLang, _("图鉴中有完整说明"));

	char aUseCmd[32];
	char aDropCmd[32];
	char aUseLabel[96];
	char aDropLabel[96];
	str_format(aUseCmd, sizeof(aUseCmd), "lcm_scrap use %d", Slot);
	str_format(aDropCmd, sizeof(aDropCmd), "lcm_scrap drop %d", Slot);
	LcLocalizeCopy(aUseLabel, sizeof(aUseLabel), pLoc, pLang, _("☞ 使用 (理由1)"));
	LcLocalizeCopy(aDropLabel, sizeof(aDropLabel), pLoc, pLang, _("☞ 放下 (理由空)"));
	pMenu->AddAction(aUseCmd, aUseLabel);
	pMenu->AddAction(aDropCmd, aDropLabel);
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 8", _("⏎ 返回背包列表"));
}

void LcFillShipCargoIndex(CLcTerminalMenu *pMenu, CGameContext *pGameServer, CLocalization *pLoc, const char *pLang)
{
	if(!pGameServer->m_pController || !pGameServer->m_pController->m_pShip)
	{
		AddInfoLoc(pMenu, pLoc, pLang, _("当前不在飞船内"));
		AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
		return;
	}

	pGameServer->m_pController->m_pShip->RecalculateValue();
	const int Num = pGameServer->m_pController->m_pShip->GetNum();
	const int Value = pGameServer->m_pController->m_pShip->GetValue();
	char aSummary[128];
	LcFormatCopy(aSummary, sizeof(aSummary), pLoc, pLang, _("共 {int:num} 件 | 总价值 {int:value} 元"), "num", &Num, "value", &Value);
	AddInfoLoc(pMenu, pLoc, pLang, _("☪ 飞船库存"));
	pMenu->AddInfo(aSummary);
	AddInfoLoc(pMenu, pLoc, pLang, _("已放入飞船的废品（只读）"));
	pMenu->AddInfo("");

	int c = 0;
	for(CScrap *pScrap = (CScrap *)pGameServer->m_World.FindFirst(CGameWorld::ENTTYPE_SCRAP); pScrap; pScrap = (CScrap *)pScrap->TypeNext())
	{
		if(!pScrap->GetInShip())
			continue;
		c++;
		char aLine[128];
		int ScrapValue = pScrap->GetScrapValue();
		int ScrapWeight = pScrap->GetWeight();
		LcFormatCopy(aLine, sizeof(aLine), pLoc, pLang, _("#{int:c} {lstr:name} {int:value}元/{int:weight}镑"),
			"c", &c, "name", pGameServer->ScrapInfo()->GetScrapName(pScrap->GetScrapType()),
			"value", &ScrapValue, "weight", &ScrapWeight);
		pMenu->AddInfo(aLine);
	}

	if(c == 0)
		AddInfoLoc(pMenu, pLoc, pLang, _("船上暂无废品"));
	AddActionLoc(pMenu, pLoc, pLang, "lcm_goto 0", _("⏎ 返回"));
}

static void SendGuideHeader(CGameContext *pGameServer, int ClientID, const char *pTitle)
{
	pGameServer->SendChatTarget(ClientID, _("- - - - - - -"));
	pGameServer->SendChatTarget(ClientID, pTitle);
}

void LcSendStoreGuide(CGameContext *pGameServer, int ClientID)
{
	SendGuideHeader(pGameServer, ClientID, _("【公司商店说明】"));
	pGameServer->SendChatTarget(ClientID, _("F3 终端图鉴可分页查看各商品详情"));
	pGameServer->SendChatTarget(ClientID, _("保障与班次 | 延长班次、护甲、生命、飞行器(可叠加库存)"));
	pGameServer->SendChatTarget(ClientID, _("下趟武器 | 散弹/激光/榴弹/手枪/忍者刀"));
	pGameServer->SendChatTarget(ClientID, _("实地消耗品 | 闪光、急救、哨子、汽水等"));
	pGameServer->SendChatTarget(ClientID, _("购买后下趟出发生效；公司币全队共享"));
	pGameServer->SendChatTarget(ClientID, _("- - - - - - -"));
}

void LcSendMonsterGuide(CGameContext *pGameServer, int ClientID)
{
	SendGuideHeader(pGameServer, ClientID, _("【怪物图鉴】"));
	pGameServer->SendChatTarget(ClientID, _("F3 终端可查看每种怪物的详细对策"));
	pGameServer->SendChatTarget(ClientID, _("血量归零立即死亡；囤积虫约60秒消散，多种怪贴近后会自毁"));
	for(int i = 0; i < (int)(sizeof(s_aMonsterTypes) / sizeof(s_aMonsterTypes[0])); i++)
	{
		pGameServer->SendChatTarget(ClientID, _("{int:id} {lstr:name} | {lstr:desc}"),
			"id", &i, "name", LcMonsterName(s_aMonsterTypes[i]), "desc", LcMonsterDescShort(s_aMonsterTypes[i]));
	}
	pGameServer->SendChatTarget(ClientID, _("每5班次可能出现首领，血量更高"));
	pGameServer->SendChatTarget(ClientID, _("- - - - - - -"));
}

void LcSendScrapGuide(CGameContext *pGameServer, int ClientID)
{
	SendGuideHeader(pGameServer, ClientID, _("【废品说明】"));
	pGameServer->SendChatTarget(ClientID, _("F3 终端按 L1/L2/L3 分页查看各废品详情"));
	pGameServer->SendChatTarget(ClientID, _("锤子拾取；背包超重会减速"));
	pGameServer->SendChatTarget(ClientID, _("ESC/F3：理由1=使用 | 理由空=放下"));
	pGameServer->SendChatTarget(ClientID, _("L1 常见 8 种 | L2 稀有 8 种 | L3 高价值 8 种"));
	pGameServer->SendChatTarget(ClientID, _("- - - - - - -"));
}
