/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus.h"

#include <base/dbg.h>
#include <base/math.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/components/menu_background.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>

void CMenus::SetNeedSendInfo()
{
	if(m_Dummy)
		m_NeedSendDummyinfo = true;
	else
		m_NeedSendinfo = true;
}

void CMenus::RenderSettings(CUIRect MainView)
{

	// render background
	CUIRect Button, TabBar, RestartBar;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(20.0f, &MainView);

	const bool NeedRestart = m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartUpdate;
	if(NeedRestart)
	{
		MainView.HSplitBottom(20.0f, &MainView, &RestartBar);
		MainView.HSplitBottom(10.0f, &MainView, nullptr);
	}

	TabBar.HSplitTop(50.0f, &Button, &TabBar);
	Button.Draw(ms_ColorTabbarActive, IGraphics::CORNER_BR, 10.0f);

	const char *apTabs[SETTINGS_LENGTH] = {
		Localize("Language"),
		Localize("General"),
		Localize("Player"),
		Client()->IsSixup() ? "Tee 0.7" : Localize("Tee"),
		Localize("Appearance"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		Localize("DDNet"),
		Localize("Assets"),
		Localize("Credits"),
		"DOClient"};
	constexpr int NUM_VISIBLE_TABS = SETTINGS_LENGTH - 1;
	static const int s_aTabOrder[NUM_VISIBLE_TABS] = {
		SETTINGS_GENERAL,
		SETTINGS_PLAYER,
		SETTINGS_TEE,
		SETTINGS_APPEARANCE,
		SETTINGS_CONTROLS,
		SETTINGS_GRAPHICS,
		SETTINGS_SOUND,
		SETTINGS_DOCLIENT,
		SETTINGS_DDNET,
		SETTINGS_ASSETS,
		SETTINGS_CREDITS};
	static CButtonContainer s_aTabButtons[SETTINGS_LENGTH];

	for(int i = 0; i < NUM_VISIBLE_TABS; i++)
	{
		const int Page = s_aTabOrder[i];
		TabBar.HSplitTop(10.0f, nullptr, &TabBar);
		TabBar.HSplitTop(26.0f, &Button, &TabBar);
		const bool IsDoClientTab = Page == SETTINGS_DOCLIENT;
		if(DoButton_MenuTab(&s_aTabButtons[Page], IsDoClientTab ? "" : apTabs[Page], g_Config.m_UiSettingsPage == Page, &Button, IGraphics::CORNER_R, &m_aAnimatorsSettingsTab[Page]))
			g_Config.m_UiSettingsPage = Page;
		if(IsDoClientTab)
		{
			// "DO" in dark blue, "Client" in the default color
			CUIRect Label = Button;
			Label.HMargin(2.0f, &Label);
			const float FontSize = Label.h * CUi::ms_FontmodHeight;
			const float TotalWidth = TextRender()->TextWidth(FontSize, "DOClient", -1, -1.0f);
			const float DoWidth = TextRender()->TextWidth(FontSize, "DO", -1, -1.0f);
			const float StartX = Label.x + (Label.w - TotalWidth) / 2.0f;

			CUIRect DoPart = Label;
			DoPart.x = StartX;
			DoPart.w = DoWidth;
			TextRender()->TextColor(ColorRGBA(0.05f, 0.15f, 0.45f, 1.0f));
			Ui()->DoLabel(&DoPart, "DO", FontSize, TEXTALIGN_MR);
			TextRender()->TextColor(TextRender()->DefaultTextColor());

			CUIRect ClientPart = Label;
			ClientPart.x = StartX + DoWidth;
			ClientPart.w = TotalWidth - DoWidth;
			Ui()->DoLabel(&ClientPart, "Client", FontSize, TEXTALIGN_ML);
		}
	}

	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
		g_Config.m_UiSettingsPage = SETTINGS_GENERAL;

	const bool Animations = g_Config.m_EcUiAnimations && g_Config.m_EcMenuAnimation;
	if(Animations)
	{
		if(m_LastSettingsPage < 0 || m_LastSettingsPage != g_Config.m_UiSettingsPage)
			m_SettingsPageChangeTime = time();
		m_LastSettingsPage = g_Config.m_UiSettingsPage;
	}
	else
	{
		m_SettingsPageChangeTime = 0;
		m_LastSettingsPage = -1;
	}
	// keep the menu background position in sync (once per frame, not once per animation pass)
	switch(g_Config.m_UiSettingsPage)
	{
	case SETTINGS_LANGUAGE:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_LANGUAGE);
		break;
	case SETTINGS_GENERAL:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
		break;
	case SETTINGS_PLAYER:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_PLAYER);
		break;
	case SETTINGS_TEE:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_TEE);
		break;
	case SETTINGS_APPEARANCE:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_APPEARANCE);
		break;
	case SETTINGS_CONTROLS:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_CONTROLS);
		break;
	case SETTINGS_GRAPHICS:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_GRAPHICS);
		break;
	case SETTINGS_DOCLIENT:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_RESERVED0);
		break;
	case SETTINGS_SOUND:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_SOUND);
		break;
	case SETTINGS_DDNET:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_DDNET);
		break;
	case SETTINGS_ASSETS:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_ASSETS);
		break;
	case SETTINGS_CREDITS:
		GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_SETTINGS_CREDITS);
		break;
	default:
		break;
	}

	auto RenderPage = [&](const CUIRect &View) {
		m_SettingsViewY = View.y;
		m_SettingsViewX = View.x;
	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
	{
		RenderLanguageSettings(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
	{
		RenderSettingsGeneral(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
	{
		RenderSettingsPlayer(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
	{
		if(Client()->IsSixup())
			RenderSettingsTee7(View);
		else
			RenderSettingsTee(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_APPEARANCE)
	{
		RenderSettingsAppearance(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
	{
		m_MenusSettingsControls.Render(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
	{
		RenderSettingsGraphics(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_DOCLIENT)
	{
		RenderSettingsDOClient(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
	{
		RenderSettingsSound(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_DDNET)
	{
		RenderSettingsDDNet(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
	{
		RenderSettingsAssets(View);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CREDITS)
	{
		RenderSettingsCredits(View);
	}
	else
	{
		dbg_assert_failed("ui_settings_page invalid");
	}

	};

	const float AnimAge = Animations && m_SettingsPageChangeTime != 0 ? (time() - m_SettingsPageChangeTime) / (float)time_freq() : 1.0f;
	if(AnimAge < 0.45f)
	{
		const float Progress = std::clamp(AnimAge / 0.45f, 0.0f, 1.0f);
		const float Inv = 1.0f - Progress;
		const float Eased = 1.0f - Inv * Inv * Inv;

		const CUIRect PanelArea = MainView;
		Ui()->ClipEnable(&PanelArea);

		const float SplitY = m_SettingsSplitY >= 0.0f ? std::clamp(m_SettingsSplitY, 0.0f, MainView.h) : MainView.h * 0.5f;
		const float Slide = (1.0f - Eased) * 20.0f;

		if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
		{
			const float TopHeight = SplitY;
			const float BottomHeight = MainView.h - SplitY;
			const float TopVisible = TopHeight * Eased;
			const float BottomVisible = BottomHeight * Eased;

			if(TopVisible > 1.0f)
			{
				CUIRect ClipTop = MainView;
				ClipTop.h = TopVisible;
				CUIRect ViewTop = MainView;
				ViewTop.y -= Slide;
				Ui()->ClipEnable(&ClipTop);
				RenderPage(ViewTop);
				Ui()->ClipDisable();
			}

			if(BottomVisible > 1.0f)
			{
				CUIRect ClipBottom = MainView;
				ClipBottom.y += MainView.h - BottomVisible;
				ClipBottom.h = BottomVisible;
				CUIRect ViewBottom = MainView;
				ViewBottom.y += Slide;
				Ui()->ClipEnable(&ClipBottom);
				RenderPage(ViewBottom);
				Ui()->ClipDisable();
			}
		}
		else if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
		{
			const float LangW = 300.0f;
			const float Gap = 20.0f;
			const float InnerW = MainView.w - LangW - Gap;
			const float DemoW = InnerW * 0.42f;
			const float LeftW = InnerW - DemoW - Gap;
			const float ShiftH = (1.0f - Eased) * MainView.w;
			const float ShiftV = (1.0f - Eased) * MainView.h;

			// left column slides in from the left
			CUIRect ClipLeft = MainView;
			ClipLeft.w = LeftW;
			ClipLeft.x -= ShiftH;
			CUIRect ViewLeft = MainView;
			ViewLeft.x -= ShiftH;
			Ui()->ClipEnable(&ClipLeft);
			RenderPage(ViewLeft);
			Ui()->ClipDisable();

			// demo column slides in from the top
			CUIRect ClipCenter = MainView;
			ClipCenter.x += LeftW + Gap;
			ClipCenter.w = DemoW;
			ClipCenter.y -= ShiftV;
			CUIRect ViewCenter = MainView;
			ViewCenter.y -= ShiftV;
			Ui()->ClipEnable(&ClipCenter);
			RenderPage(ViewCenter);
			Ui()->ClipDisable();

			// language column slides in from the right
			CUIRect ClipRight = MainView;
			ClipRight.x += MainView.w - LangW + ShiftH;
			ClipRight.w = LangW;
			CUIRect ViewRight = MainView;
			ViewRight.x += ShiftH;
			Ui()->ClipEnable(&ClipRight);
			RenderPage(ViewRight);
			Ui()->ClipDisable();
		}		else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
		{
			const float SplitY = m_SettingsSplitY >= 0.0f ? std::clamp(m_SettingsSplitY, 0.0f, MainView.h) : MainView.h * 0.5f;
			const float BottomHeight = MainView.h - SplitY;
			const float BottomVisible = BottomHeight * Eased;
			const float Slide = (1.0f - Eased) * 20.0f;

			const float TopHalfWidth = MainView.w * 0.5f;
			const float TopShift = (1.0f - Eased) * TopHalfWidth;

			CUIRect ClipTopLeft = MainView;
			ClipTopLeft.h = SplitY;
			ClipTopLeft.w = TopHalfWidth;
			ClipTopLeft.x -= TopShift;
			CUIRect ViewTopLeft = MainView;
			ViewTopLeft.x -= TopShift;
			Ui()->ClipEnable(&ClipTopLeft);
			RenderPage(ViewTopLeft);
			Ui()->ClipDisable();

			CUIRect ClipTopRight = MainView;
			ClipTopRight.h = SplitY;
			ClipTopRight.x += TopHalfWidth + TopShift;
			ClipTopRight.w = MainView.w - TopHalfWidth;
			CUIRect ViewTopRight = MainView;
			ViewTopRight.x += TopShift;
			Ui()->ClipEnable(&ClipTopRight);
			RenderPage(ViewTopRight);
			Ui()->ClipDisable();

			if(BottomVisible > 1.0f)
			{
				CUIRect ClipBottom = MainView;
				ClipBottom.y += MainView.h - BottomVisible;
				ClipBottom.h = BottomVisible;
				CUIRect View = MainView;
				View.y += Slide;
				Ui()->ClipEnable(&ClipBottom);
				RenderPage(View);
				Ui()->ClipDisable();
			}
		}		else if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE || g_Config.m_UiSettingsPage == SETTINGS_SOUND)
		{
			CUIRect View = MainView;
			View.x -= (1.0f - Eased) * MainView.w;
			RenderPage(View);
		}
		else if(g_Config.m_UiSettingsPage == SETTINGS_CREDITS)
		{
			const float VisibleHeight = MainView.h * Eased;
			const float Slide = (1.0f - Eased) * 24.0f;

			CUIRect ClipTop = MainView;
			ClipTop.h = VisibleHeight;
			CUIRect View = MainView;
			View.y -= Slide;
			Ui()->ClipEnable(&ClipTop);
			RenderPage(View);
			Ui()->ClipDisable();
		}		else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
		{
			const float TopHeight = SplitY;
			const float BottomHeight = MainView.h - SplitY;
			const float TopEase = Progress < 0.5f ? 2.0f * Progress * Progress : 1.0f - std::pow(-2.0f * Progress + 2.0f, 2.0f) * 0.5f;
			const float TopVisible = MainView.w * TopEase;
			const float BottomVisible = BottomHeight * Eased;

			if(TopVisible > 1.0f)
			{
				CUIRect ClipTop = MainView;
				ClipTop.w = TopVisible;
				ClipTop.h = TopHeight;
				CUIRect ViewTop = MainView;
				ViewTop.x += Slide;
				Ui()->ClipEnable(&ClipTop);
				RenderPage(ViewTop);
				Ui()->ClipDisable();
			}

			if(BottomVisible > 1.0f)
			{
				CUIRect ClipBottom = MainView;
				ClipBottom.y += MainView.h - BottomVisible;
				ClipBottom.h = BottomVisible;
				CUIRect ViewBottom = MainView;
				ViewBottom.y += Slide;
				Ui()->ClipEnable(&ClipBottom);
				RenderPage(ViewBottom);
				Ui()->ClipDisable();
			}
		}
		else
		{
			float HalfWidth = MainView.w * 0.5f;
			if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS && m_SettingsSplitX >= 0.0f)
				HalfWidth = std::clamp(m_SettingsSplitX, 1.0f, MainView.w - 1.0f);
			else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
				HalfWidth = MainView.w * 0.49f;
			const float Shift = (1.0f - Eased) * HalfWidth;

			CUIRect ClipLeft = MainView;
			ClipLeft.w = HalfWidth;
			ClipLeft.x -= Shift;
			CUIRect ViewLeft = MainView;
			ViewLeft.x -= Shift;
			Ui()->ClipEnable(&ClipLeft);
			RenderPage(ViewLeft);
			Ui()->ClipDisable();

			CUIRect ClipRight = MainView;
			ClipRight.x += HalfWidth + Shift;
			ClipRight.w = MainView.w - HalfWidth;
			CUIRect ViewRight = MainView;
			ViewRight.x += Shift;
			Ui()->ClipEnable(&ClipRight);
			RenderPage(ViewRight);
			Ui()->ClipDisable();
		}

		Ui()->ClipDisable();
	}
	else
	{
		RenderPage(MainView);
	}
	if(NeedRestart)
	{
		CUIRect RestartWarning, RestartButton;
		RestartBar.VSplitRight(125.0f, &RestartWarning, &RestartButton);
		RestartWarning.VSplitRight(10.0f, &RestartWarning, nullptr);
		if(m_NeedRestartUpdate)
		{
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
			Ui()->DoLabel(&RestartWarning, Localize("DDNet Client needs to be restarted to complete update!"), 14.0f, TEXTALIGN_ML);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else
		{
			Ui()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 14.0f, TEXTALIGN_ML);
		}

		static CButtonContainer s_RestartButton;
		if(DoButton_Menu(&s_RestartButton, Localize("Restart"), 0, &RestartButton))
		{
			if(Client()->State() == IClient::STATE_ONLINE || GameClient()->Editor()->HasUnsavedData())
			{
				m_Popup = POPUP_RESTART;
			}
			else
			{
				Client()->Restart();
			}
		}
	}
}

bool CMenus::RenderHslaScrollbars(CUIRect *pRect, unsigned int *pColor, bool Alpha, float DarkestLight)
{
	const unsigned PrevPackedColor = *pColor;
	ColorHSLA Color(*pColor, Alpha);
	const ColorHSLA OriginalColor = Color;
	const char *apLabels[] = {Localize("Hue"), Localize("Sat."), Localize("Lht."), Localize("Alpha")};
	const float SizePerEntry = 20.0f;
	const float MarginPerEntry = 5.0f;
	const float PreviewMargin = 2.5f;
	const float PreviewHeight = 40.0f + 2 * PreviewMargin;
	const float OffY = (SizePerEntry + MarginPerEntry) * (3 + (Alpha ? 1 : 0)) - PreviewHeight;

	CUIRect Preview;
	pRect->VSplitLeft(PreviewHeight, &Preview, pRect);
	Preview.HSplitTop(OffY / 2.0f, nullptr, &Preview);
	Preview.HSplitTop(PreviewHeight, &Preview, nullptr);

	Preview.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 4.0f + PreviewMargin);
	Preview.Margin(PreviewMargin, &Preview);
	Preview.Draw(color_cast<ColorRGBA>(Color.UnclampLighting(DarkestLight)), IGraphics::CORNER_ALL, 4.0f + PreviewMargin);

	auto &&RenderHueRect = [&](CUIRect *pColorRect) {
		float CurXOff = pColorRect->x;
		const float SizeColor = pColorRect->w / 6;

		// red to yellow
		{
			Graphics()->SetColor4(
				ColorRGBA(1, 0, 0, 1),
				ColorRGBA(1, 1, 0, 1),
				ColorRGBA(1, 1, 0, 1),
				ColorRGBA(1, 0, 0, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		// yellow to green
		CurXOff += SizeColor;
		{
			Graphics()->SetColor4(
				ColorRGBA(1, 1, 0, 1),
				ColorRGBA(0, 1, 0, 1),
				ColorRGBA(0, 1, 0, 1),
				ColorRGBA(1, 1, 0, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// green to turquoise
		{
			Graphics()->SetColor4(
				ColorRGBA(0, 1, 0, 1),
				ColorRGBA(0, 1, 1, 1),
				ColorRGBA(0, 1, 1, 1),
				ColorRGBA(0, 1, 0, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// turquoise to blue
		{
			Graphics()->SetColor4(
				ColorRGBA(0, 1, 1, 1),
				ColorRGBA(0, 0, 1, 1),
				ColorRGBA(0, 0, 1, 1),
				ColorRGBA(0, 1, 1, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// blue to purple
		{
			Graphics()->SetColor4(
				ColorRGBA(0, 0, 1, 1),
				ColorRGBA(1, 0, 1, 1),
				ColorRGBA(1, 0, 1, 1),
				ColorRGBA(0, 0, 1, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// purple to red
		{
			Graphics()->SetColor4(
				ColorRGBA(1, 0, 1, 1),
				ColorRGBA(1, 0, 0, 1),
				ColorRGBA(1, 0, 0, 1),
				ColorRGBA(1, 0, 1, 1));

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
	};

	auto &&RenderSaturationRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColor) {
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);

		LeftColor.s = 0.0f;
		RightColor.s = 1.0f;

		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);

		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);

		IGraphics::CFreeformItem Freeform(
			pColorRect->x, pColorRect->y,
			pColorRect->x + pColorRect->w, pColorRect->y,
			pColorRect->x, pColorRect->y + pColorRect->h,
			pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	auto &&RenderLightingRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColor) {
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);

		LeftColor.l = DarkestLight;
		RightColor.l = 1.0f;

		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);

		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);

		IGraphics::CFreeformItem Freeform(
			pColorRect->x, pColorRect->y,
			pColorRect->x + pColorRect->w, pColorRect->y,
			pColorRect->x, pColorRect->y + pColorRect->h,
			pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	auto &&RenderAlphaRect = [&](CUIRect *pColorRect, const ColorRGBA &CurColorFull) {
		const ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(color_cast<ColorHSLA>(CurColorFull).WithAlpha(0.0f));
		const ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(color_cast<ColorHSLA>(CurColorFull).WithAlpha(1.0f));

		Graphics()->SetColor4(LeftColorRGBA, RightColorRGBA, RightColorRGBA, LeftColorRGBA);

		IGraphics::CFreeformItem Freeform(
			pColorRect->x, pColorRect->y,
			pColorRect->x + pColorRect->w, pColorRect->y,
			pColorRect->x, pColorRect->y + pColorRect->h,
			pColorRect->x + pColorRect->w, pColorRect->y + pColorRect->h);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	};

	for(int i = 0; i < 3 + Alpha; i++)
	{
		CUIRect Button, Label;
		pRect->HSplitTop(SizePerEntry, &Button, pRect);
		pRect->HSplitTop(MarginPerEntry, nullptr, pRect);
		Button.VSplitLeft(140.0f, &Label, &Button);
		Label.VMargin(10.0f, &Label);

		Button.Draw(ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), IGraphics::CORNER_ALL, 1.0f);

		CUIRect Rail;
		Button.Margin(2.0f, &Rail);

		char aBuf[32];

		// Hue
		if(i == 0)
			str_format(aBuf, sizeof(aBuf), "%s: %.1f° (%03d)", apLabels[i], Color[i] * 360.0f, round_to_int(Color[i] * 255.0f));
		// Lht
		else if(i == 2)
		{
			// handle internal light clamping, see `UnclampLighting`
			float Lht = DarkestLight + Color[i] * (1.0f - DarkestLight);
			str_format(aBuf, sizeof(aBuf), "%s: %.1f%% (%03d)", apLabels[i], Lht * 100.0f, round_to_int(Color[i] * 255.0f));
		}
		// Sat and Alpha
		else
			str_format(aBuf, sizeof(aBuf), "%s: %.1f%% (%03d)", apLabels[i], Color[i] * 100.0f, round_to_int(Color[i] * 255.0f));
		Ui()->DoLabel(&Label, aBuf, 12.0f, TEXTALIGN_ML);

		ColorRGBA HandleColor;
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();
		if(i == 0)
		{
			RenderHueRect(&Rail);
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, 1.0f, 0.5f, 1.0f));
		}
		else if(i == 1)
		{
			RenderSaturationRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, 1.0f, 0.5f, 1.0f)));
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, 0.5f, 1.0f));
		}
		else if(i == 2)
		{
			RenderLightingRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, 0.5f, 1.0f)));
			HandleColor = color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f).UnclampLighting(DarkestLight));
		}
		else if(i == 3)
		{
			RenderAlphaRect(&Rail, color_cast<ColorRGBA>(ColorHSLA(Color.h, Color.s, Color.l, 1.0f).UnclampLighting(DarkestLight)));
			HandleColor = color_cast<ColorRGBA>(Color.UnclampLighting(DarkestLight));
		}
		Graphics()->TrianglesEnd();

		Color[i] = Ui()->DoScrollbarH(&((char *)pColor)[i], &Button, Color[i], &HandleColor);
	}

	if(OriginalColor != Color)
	{
		*pColor = Color.Pack(Alpha);
	}
	return PrevPackedColor != *pColor;
}
