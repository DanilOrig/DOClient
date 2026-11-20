#include "aspect_ratio.h"

#include <base/time.h>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/components/chat.h>
#include <game/client/components/hud.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

CAspectRatio::SConfig CAspectRatio::ReadConfig() const
{
	return {g_Config.m_EcAspectMode, g_Config.m_EcAspectRatio, g_Config.m_EcAspectNum, g_Config.m_EcAspectDen};
}

void CAspectRatio::WriteConfig(const SConfig &Config)
{
	g_Config.m_EcAspectMode = Config.m_Mode;
	g_Config.m_EcAspectRatio = Config.m_Ratio;
	g_Config.m_EcAspectNum = Config.m_Num;
	g_Config.m_EcAspectDen = Config.m_Den;
}

float CAspectRatio::TargetAspect() const
{
	if(g_Config.m_EcAspectMode == 0)
		return 0.0f;
	if(g_Config.m_EcAspectMode == 2 && g_Config.m_EcAspectNum > 0 && g_Config.m_EcAspectDen > 0)
		return (float)g_Config.m_EcAspectNum / (float)g_Config.m_EcAspectDen;
	if(g_Config.m_EcAspectRatio >= 100)
		return g_Config.m_EcAspectRatio / 100.0f;
	return 0.0f;
}

void CAspectRatio::Apply()
{
	const float Aspect = TargetAspect();
	if(Aspect == m_AppliedAspect)
		return;
	m_AppliedAspect = Aspect;
	Graphics()->SetScreenAspectOverride(Aspect);
	// the chat and the hud cache text/quad containers laid out in screen space
	GameClient()->m_Chat.RebuildChat();
	GameClient()->m_Hud.OnReset();
}

void CAspectRatio::OnConsoleInit()
{
	Console()->Register("ec_toggle_aspect", "?i[mode]", CFGFLAG_CLIENT, ConToggleAspect, this, "Set the custom aspect ratio mode (0 = off)");
}

void CAspectRatio::ConToggleAspect(IConsole::IResult *pResult, void *pUserData)
{
	if(pResult->NumArguments() > 0)
		g_Config.m_EcAspectMode = pResult->GetInteger(0);
	else
		g_Config.m_EcAspectMode = g_Config.m_EcAspectMode == 0 ? 1 : 0;
	if(g_Config.m_EcAspectMode != 0 && g_Config.m_EcAspectRatio < 100)
		g_Config.m_EcAspectRatio = 133;
}

void CAspectRatio::OnReset()
{
	Graphics()->SetScreenAspectOverride(0.0f);
	m_AppliedAspect = -1.0f;
	m_ConfirmActive = false;
}

void CAspectRatio::OnUpdate()
{
	const SConfig Current = ReadConfig();
	if(!m_Seeded)
	{
		m_Seeded = true;
		m_Last = Current;
		m_Prev = Current;
	}
	else if(m_Last != Current)
	{
		if(Current.m_Mode != 0 && Current != m_Prev)
		{
			m_ConfirmActive = true;
			m_ConfirmDeadline = time_get() + time_freq() * 10;
		}
		else
		{
			m_ConfirmActive = false;
			m_Prev = Current;
		}
		m_Last = Current;
	}

	if(m_ConfirmActive && time_get() >= m_ConfirmDeadline)
	{
		WriteConfig(m_Prev);
		m_Last = m_Prev;
		m_ConfirmActive = false;
	}

	Apply();
}

void CAspectRatio::OnRender()
{
	if(!m_ConfirmActive)
		return;

	Ui()->MapScreen();
	const CUIRect *pScreen = Ui()->Screen();
	const float SecondsLeft = std::max(0.0f, (m_ConfirmDeadline - time_get()) / (float)time_freq());

	CUIRect Panel;
	const float PanelWidth = std::min(460.0f, pScreen->w - 12.0f);
	Panel.x = pScreen->x + (pScreen->w - PanelWidth) * 0.5f;
	Panel.y = pScreen->y + 8.0f;
	Panel.w = PanelWidth;
	Panel.h = 34.0f;
	Graphics()->DrawRect(Panel.x, Panel.y, Panel.w, Panel.h, ColorRGBA(0.05f, 0.06f, 0.09f, 0.9f), IGraphics::CORNER_ALL, 6.0f);
	Panel.Margin(4.0f, &Panel);

	CUIRect RevertRect, ApplyRect, TextRect;
	Panel.VSplitRight(90.0f, &Panel, &RevertRect);
	Panel.VSplitRight(6.0f, &Panel, nullptr);
	Panel.VSplitRight(90.0f, &Panel, &ApplyRect);
	Panel.VSplitRight(6.0f, &Panel, nullptr);
	TextRect = Panel;

	char aText[128];
	str_format(aText, sizeof(aText), "Оставить это соотношение? Откат через %.0f с", SecondsLeft);
	Ui()->DoLabel(&TextRect, aText, 10.0f, TEXTALIGN_ML);

	auto DrawOverlayButton = [&](const CUIRect &Rect, const char *pText, ColorRGBA Normal, ColorRGBA Hovered) -> bool {
		const bool Hover = Ui()->MouseInside(&Rect);
		Graphics()->DrawRect(Rect.x, Rect.y, Rect.w, Rect.h, Hover ? Hovered : Normal, IGraphics::CORNER_ALL, 4.0f);
		CUIRect Label = Rect;
		Ui()->DoLabel(&Label, pText, 10.0f, TEXTALIGN_MC);
		return Hover && Ui()->MouseButtonClicked(0);
	};

	if(DrawOverlayButton(ApplyRect, "Оставить", ColorRGBA(0.18f, 0.42f, 0.2f, 1.0f), ColorRGBA(0.26f, 0.58f, 0.28f, 1.0f)))
	{
		m_Prev = ReadConfig();
		m_ConfirmActive = false;
	}
	if(DrawOverlayButton(RevertRect, "Откатить", ColorRGBA(0.55f, 0.2f, 0.2f, 1.0f), ColorRGBA(0.72f, 0.28f, 0.28f, 1.0f)))
	{
		WriteConfig(m_Prev);
		m_Last = m_Prev;
		m_ConfirmActive = false;
	}
}
