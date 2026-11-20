#include "edgehelper.h"

#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/client/components/camera.h>
#include <game/client/components/entity/hud/hud_layout.h>
#include <game/client/components/controls.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>

namespace SEdgeHelperProperties
{
	static constexpr float ms_Padding = 3.0f;
	static constexpr float ms_Rounding = 3.0f;

	static constexpr float ms_ItemSpacing = 2.0f;

	static constexpr float ms_CubeSize = 24.0f;
	static constexpr float ms_ArrowsSize = 18.0f;
	static constexpr float ms_WallWidth = 3.0f;
	static constexpr float ms_CircleRadius = 8.0f;
	static constexpr float ms_CircleThickness = 2.0f;

	static ColorRGBA WindowColorDark() { return ColorRGBA(0.2f, 0.2f, 0.2f, 0.9f); }
	static ColorRGBA WindowColorMedium() { return ColorRGBA(0.35f, 0.35f, 0.35f, 0.9f); }

	static ColorRGBA ActionActiveButtonColor() { return ColorRGBA(0.53f, 0.78f, 0.53f, 0.8f); }
	static ColorRGBA ActionWhiteButtonColor() { return ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f); }
};

CEdgeHelper::CEdgeHelper()
{
	CEdgeHelper::OnReset();
}

void CEdgeHelper::OnConsoleInit()
{
	Console()->Register("ec_toggle_edge_info", "", CFGFLAG_CLIENT, ConToggleEdgeHelper, this, "Toggle edge info");
}

void CEdgeHelper::ConToggleEdgeHelper(IConsole::IResult *pResult, void *pUserData)
{
	CEdgeHelper *pSelf = (CEdgeHelper *)pUserData;
	pSelf->SetActive(!pSelf->IsActive());
}

void CEdgeHelper::SetActive(bool Active)
{
	if(m_Active == Active)
		return;

	m_Active = Active;
	if(m_Active)
	{
		if(!g_Config.m_EcEdgeInfoJump && !g_Config.m_EcEdgeInfoCords)
		{
			GameClient()->Echo("Enable any edgeinfo function");
			OnReset();
		}
	}
	else
	{
		OnReset();
	}
}

void CEdgeHelper::OnReset()
{
	SetActive(false);
}

void CEdgeHelper::OnRelease()
{
	SetActive(false);
}

void CEdgeHelper::OnRender()
{
	if(!IsActive())
		return;

	RenderEdgeHelper();
}

void CEdgeHelper::RenderEdgeHelper()
{
	CUIRect Base, EdgeInfo, JumpInfo;

	float MHeight = 300.0f;
	float MWidth = MHeight * (Graphics()->ScreenAspect());

	Base.h = MHeight / (g_Config.m_EcEdgeInfoJump && g_Config.m_EcEdgeInfoCords ? 6 : 12);
	// DOClient: width follows the content instead of a fixed fraction of the screen
	const float ArrowsWidth = 2.0f * SEdgeHelperProperties::ms_ArrowsSize + 16.0f;
	const float JumpRowWidth = ArrowsWidth + TextRender()->TextWidth(12.0f, "888 | 888 | 888", -1, -1.0f);
	Base.w = std::max(96.0f, std::min(MWidth - 20.0f, JumpRowWidth));
	// DOClient: centred along the bottom edge, clear of the chat
	Base.x = (MWidth - Base.w) * (g_Config.m_EcEdgeInfoPosX / 100.0f);
	Base.y = MHeight - Base.h - 34.0f;

	Graphics()->MapScreenToSize(MWidth, MHeight);

	Base.Draw(CHudLayout::BackgroundColor(), IGraphics::CORNER_ALL, 5.0f);
	Base.Margin(SEdgeHelperProperties::ms_Padding, &Base);
	const int ClientId = GameClient()->m_Snap.m_SpecInfo.m_Active ? GameClient()->m_Snap.m_SpecInfo.m_SpectatorId : GameClient()->m_Snap.m_LocalClientId;
	m_PosX = GetPositionEdgeHelper(ClientId, g_Config.m_ClDummy);

	if(g_Config.m_EcEdgeInfoCords && g_Config.m_EcEdgeInfoJump)
		Base.HSplitMid(&EdgeInfo, &JumpInfo);
	if(g_Config.m_EcEdgeInfoCords)
		RenderEdgeHelperEdgeInfo(g_Config.m_EcEdgeInfoJump ? &EdgeInfo : &Base);
	if(g_Config.m_EcEdgeInfoJump)
		RenderEdgeHelperJumpInfo(g_Config.m_EcEdgeInfoCords ? &JumpInfo : &Base);

	// restore the hud mapping (300 units high) rather than the previous one, otherwise the
	// elements drawn after us use a stale scale after an aspect ratio change
	Graphics()->MapScreenToSize(300.0f * Graphics()->ScreenAspect(), 300.0f);
}

float CEdgeHelper::GetPositionEdgeHelper(int ClientId, int Conn)
{
	float ValuePos;
	if(ClientId == SPEC_FREEVIEW)
	{
		ValuePos = GameClient()->m_Camera.m_Center.x / 32.0f;
	}
	else if(GameClient()->m_aClients[ClientId].m_SpecCharPresent)
	{
		ValuePos = GameClient()->m_aClients[ClientId].m_SpecChar.x / 32.0f;
	}
	else
	{
		const CNetObj_Character *pPrevChar = &GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev;
		const CNetObj_Character *pCurChar = &GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur;
		const float IntraTick = Client()->IntraGameTick(Conn);
		ValuePos = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pCurChar->m_X, pCurChar->m_Y), IntraTick).x / 32.0f;
	}

	ValuePos = std::round(ValuePos * 100.0f) / 100.0f;
	float Temp = std::round(ValuePos * 100.0f); // temp = 3263.0f
	int Result = static_cast<int>(Temp) % 100; // result = 3263 % 100 = 63
	return Result;
}

void CEdgeHelper::RenderEdgeHelperEdgeInfo(CUIRect *pBase)
{
	CUIRect LeftZone, RightZone, CenterZone;
	pBase->HSplitBottom(SEdgeHelperProperties::ms_ItemSpacing, pBase, nullptr);
	float ActionSpacing = (pBase->w - (2 * (SEdgeHelperProperties::ms_WallWidth + SEdgeHelperProperties::ms_CircleRadius + SEdgeHelperProperties::ms_CubeSize))) / 4;
	pBase->VSplitLeft(SEdgeHelperProperties::ms_CubeSize + ActionSpacing, &LeftZone, &CenterZone);
	CenterZone.VSplitRight(SEdgeHelperProperties::ms_CubeSize + ActionSpacing, &CenterZone, &RightZone);
	LeftZone.VSplitRight(ActionSpacing + 2, &LeftZone, nullptr);
	RightZone.VSplitLeft(ActionSpacing + 2, nullptr, &RightZone);
	LeftZone.Margin(SEdgeHelperProperties::ms_ItemSpacing, &LeftZone);
	RightZone.Margin(SEdgeHelperProperties::ms_ItemSpacing, &RightZone);
	LeftZone.Draw(m_PosX >= 44 ? color_cast<ColorRGBA>(ColorHSLA(g_Config.m_EcEdgeInfoColorKill)) : m_PosX >= 28 ? color_cast<ColorRGBA>(ColorHSLA(g_Config.m_EcEdgeInfoColorSafe)) :
														       color_cast<ColorRGBA>(ColorHSLA(g_Config.m_EcEdgeInfoColorFreeze)),
		IGraphics::CORNER_ALL, SEdgeHelperProperties::ms_Rounding);
	RightZone.Draw(m_PosX <= 53 ? color_cast<ColorRGBA>(ColorHSLA(g_Config.m_EcEdgeInfoColorKill)) : m_PosX <= 69 ? color_cast<ColorRGBA>(ColorHSLA(g_Config.m_EcEdgeInfoColorSafe)) :
															color_cast<ColorRGBA>(ColorHSLA(g_Config.m_EcEdgeInfoColorFreeze)),
		IGraphics::CORNER_ALL, SEdgeHelperProperties::ms_Rounding);
	CenterZone.VSplitLeft(SEdgeHelperProperties::ms_WallWidth + ActionSpacing, &LeftZone, &CenterZone);
	CenterZone.VSplitRight(SEdgeHelperProperties::ms_WallWidth + ActionSpacing, &CenterZone, &RightZone);
	LeftZone.VSplitRight(ActionSpacing - 3, &LeftZone, nullptr);
	LeftZone.VSplitLeft(3, nullptr, &LeftZone);
	RightZone.VSplitLeft(ActionSpacing - 3, nullptr, &RightZone);
	RightZone.VSplitRight(3, &RightZone, nullptr);
	LeftZone.Draw(m_PosX >= 44 && m_PosX < 53 ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium(), IGraphics::CORNER_NONE, 0);
	RightZone.Draw(m_PosX <= 53 && m_PosX > 44 ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium(), IGraphics::CORNER_NONE, 0);
	CenterZone.Margin(SEdgeHelperProperties::ms_ItemSpacing, &CenterZone);
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(m_PosX > 44 && m_PosX < 53 ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium());
	Graphics()->DrawCircle(CenterZone.x + CenterZone.w / 2, CenterZone.y + CenterZone.h / 2, SEdgeHelperProperties::ms_CircleRadius, 16);
	Graphics()->QuadsEnd();
	//
	if(m_PosX == 44 || m_PosX == 53)
	{
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(SEdgeHelperProperties::ActionWhiteButtonColor());

		const int Segments = 16;
		const float CenterX = CenterZone.x + CenterZone.w / 2.0f;
		const float CenterY = CenterZone.y + CenterZone.h / 2.0f;
		const float Radius = SEdgeHelperProperties::ms_CircleRadius;
		const float Thickness = SEdgeHelperProperties::ms_CircleThickness;

		for(int i = 0; i < Segments; ++i)
		{
			float a1 = 2.0f * pi * i / Segments;
			float a2 = 2.0f * pi * (i + 1) / Segments;
			IGraphics::CFreeformItem Quad(
				CenterX + std::cos(a1) * (Radius - Thickness / 2.0f), CenterY + std::sin(a1) * (Radius - Thickness / 2.0f),
				CenterX + std::cos(a2) * (Radius - Thickness / 2.0f), CenterY + std::sin(a2) * (Radius - Thickness / 2.0f),
				CenterX + std::cos(a1) * (Radius + Thickness / 2.0f), CenterY + std::sin(a1) * (Radius + Thickness / 2.0f),
				CenterX + std::cos(a2) * (Radius + Thickness / 2.0f), CenterY + std::sin(a2) * (Radius + Thickness / 2.0f));
			Graphics()->QuadsDrawFreeform(&Quad, 1);
		}
		Graphics()->QuadsEnd();
	}
}

void CEdgeHelper::RenderEdgeHelperJumpInfo(CUIRect *pBase)
{
	pBase->HSplitTop(SEdgeHelperProperties::ms_ItemSpacing, nullptr, pBase);
	std::ranges::sort(m_Values);

	int Lower = -1;
	int Upper = -1;
	for(int v : m_Values)
	{
		if(v <= m_PosX)
			Lower = v;
		if(v >= m_PosX)
		{
			Upper = v;
			break;
		}
	}

	// DOClient: each arrow column has two indicators, the upper one lights up when several
	// jumps are possible, the lower one when exactly one jump works
	const bool LeftMany = m_PosX == 13 || m_PosX == 25 || m_PosX == 28 || m_PosX == 41;
	const bool LeftOne = m_PosX == 16 || m_PosX == 31;
	const bool RightMany = m_PosX == 56 || m_PosX == 69 || m_PosX == 72 || m_PosX == 84;
	const bool RightOne = m_PosX == 62 || m_PosX == 63 || m_PosX == 66 || m_PosX == 81;

	const float ArrowSize = SEdgeHelperProperties::ms_ArrowsSize;
	CUIRect ArrowsLeft, Mid, ArrowsRight;
	pBase->VSplitLeft(ArrowSize + 2.0f, &ArrowsLeft, &Mid);
	Mid.VSplitRight(ArrowSize + 2.0f, &Mid, &ArrowsRight);

	CUIRect LeftTop, LeftBottom, RightTop, RightBottom;
	ArrowsLeft.HSplitTop(ArrowsLeft.h / 2.0f, &LeftTop, &LeftBottom);
	ArrowsRight.HSplitTop(ArrowsRight.h / 2.0f, &RightTop, &RightBottom);

	DoDoubleIconButton(&LeftTop, FontIcon::CHEVRON_UP, ArrowSize, LeftMany ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium());
	DoIconButton(&LeftBottom, FontIcon::CHEVRON_UP, ArrowSize * 0.9f, LeftOne ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium());
	DoDoubleIconButton(&RightTop, FontIcon::CHEVRON_UP, ArrowSize, RightMany ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium());
	DoIconButton(&RightBottom, FontIcon::CHEVRON_UP, ArrowSize * 0.9f, RightOne ? SEdgeHelperProperties::ActionWhiteButtonColor() : SEdgeHelperProperties::WindowColorMedium());

	// DOClient: three separate columns so the numbers never overlap
	CUIRect LeftHalf, CenterHalf, RightHalf;
	Mid.VSplitLeft(Mid.w * 0.32f, &LeftHalf, &CenterHalf);
	CenterHalf.VSplitRight(CenterHalf.w * 0.32f, &CenterHalf, &RightHalf);

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%02i", m_PosX);
	if(m_PosX == Lower)
		TextRender()->TextColor(SEdgeHelperProperties::ActionActiveButtonColor());
	Ui()->DoLabel(&CenterHalf, aBuf, 12, TEXTALIGN_MC);
	TextRender()->TextColor(TextRender()->DefaultTextColor());

	if(Lower == -1)
		str_copy(aBuf, "- |");
	else
		str_format(aBuf, sizeof(aBuf), "%i |", Lower);
	if(m_PosX == Lower || m_PosX == Upper)
		TextRender()->TextColor(SEdgeHelperProperties::ActionActiveButtonColor());
	Ui()->DoLabel(&LeftHalf, aBuf, 12, TEXTALIGN_MR);
	TextRender()->TextColor(TextRender()->DefaultTextColor());

	if(Upper == -1)
		str_copy(aBuf, "| -");
	else
		str_format(aBuf, sizeof(aBuf), "| %i", Upper);
	if(m_PosX == Upper)
		TextRender()->TextColor(SEdgeHelperProperties::ActionActiveButtonColor());
	Ui()->DoLabel(&RightHalf, aBuf, 12, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}

bool CEdgeHelper::IsActive() const
{
	if(m_Active)
		return true;

	return false;
}

void CEdgeHelper::DoDoubleIconButton(CUIRect *pRect, const char *pIcon, float TextSize, ColorRGBA IconColor) const
{
	CUIRect Top, Bottom;
	pRect->HSplitMid(&Top, &Bottom);
	DoIconButton(&Top, pIcon, TextSize * 0.75f, IconColor);
	DoIconButton(&Bottom, pIcon, TextSize * 0.75f, IconColor);
}

void CEdgeHelper::DoIconButton(CUIRect *pRect, const char *pIcon, float TextSize, ColorRGBA IconColor) const
{
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	TextRender()->TextColor(IconColor);
	Ui()->DoLabel(pRect, pIcon, TextSize, TEXTALIGN_MC);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
}
