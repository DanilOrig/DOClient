#include "menus.h"

#include <base/str.h>
#include <game/client/ui_listbox.h>
#include <algorithm>
#include <base/time.h>

#include <engine/serverbrowser.h>
#include <engine/shared/config.h>

#include <game/client/components/chat.h>
#include <game/client/components/items.h>
#include <game/client/gameclient.h>
#include <game/client/perf.h>

#include <cctype>
#include <iterator>
#include <vector>

static void ConToggleConfig(IConsole::IResult *pResult, void *pUserData)
{
	int *pValue = static_cast<int *>(pUserData);
	*pValue = !*pValue;
}

static void ConToggleNoCopyWeapon(IConsole::IResult *pResult, void *pUserData)
{
	IConsole *pConsole = static_cast<IConsole *>(pUserData);
	g_Config.m_EcNoCopyWeapon ^= 1;
	pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "doclient", g_Config.m_EcNoCopyWeapon ? "Do not copy weapon: on" : "Do not copy weapon: off");
}

static void ConToggleClientTypes(IConsole::IResult *pResult, void *pUserData)
{
	IConsole *pConsole = static_cast<IConsole *>(pUserData);
	g_Config.m_EcClientTypes ^= 1;
	pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "doclient", g_Config.m_EcClientTypes ? "Client types: on" : "Client types: off");
}

static void ConToggleCustomCommunities(IConsole::IResult *pResult, void *pUserData)
{
	CGameClient *pGameClient = static_cast<CGameClient *>(pUserData);
	g_Config.m_EcCustomCommunities ^= 1;
	pGameClient->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "doclient", g_Config.m_EcCustomCommunities ? "Custom communities: on" : "Custom communities: off");
	pGameClient->ServerBrowser()->ReloadCommunities();
}

static void ConToggleHighPriority(IConsole::IResult *pResult, void *pUserData)
{
	CGameClient *pGameClient = static_cast<CGameClient *>(pUserData);
	g_Config.m_EcHighPriority ^= 1;
	pGameClient->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "doclient", g_Config.m_EcHighPriority ? "High process priority: on" : "High process priority: off");
	ApplyPerformanceSettings();
}

static void ConToggleDiscordLowPriority(IConsole::IResult *pResult, void *pUserData)
{
	CGameClient *pGameClient = static_cast<CGameClient *>(pUserData);
	g_Config.m_EcDiscordLowPriority ^= 1;
	pGameClient->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "doclient", g_Config.m_EcDiscordLowPriority ? "Lower Discord priority: on" : "Lower Discord priority: off");
	ApplyPerformanceSettings();
}
void CMenus::RegisterDOClientCommands()
{
	Console()->Register("ec_toggle_dummy_show_position", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcDummyShowPosition, "Toggle dummy position display in the movement information");
	Console()->Register("ec_toggle_dummy_show_speed", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcDummyShowSpeed, "Toggle dummy speed display in the movement information");
	Console()->Register("ec_toggle_dummy_show_angle", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcDummyShowAngle, "Toggle dummy target angle display in the movement information");
	Console()->Register("ec_toggle_dummy_advanced_actions", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcDummyAdvancedActions, "Toggle advanced dummy actions display");
	Console()->Register("ec_toggle_swap_timer", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcSwapTimer, "Toggle swap timer display");
	Console()->Register("ec_toggle_no_copy_weapon", "", CFGFLAG_CLIENT, ConToggleNoCopyWeapon, Console(), "Toggle not copying weapon selection to the dummy");
	Console()->Register("ec_toggle_fast_input", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcFastInput, "Toggle fast input");
	Console()->Register("ec_toggle_fast_input_others", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcFastInputOthers, "Toggle fast input for other tees");
	Console()->Register("ec_toggle_fast_input_repredict_hook", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcFastInputRepredictHook, "Toggle fast input hook reprediction");
	Console()->Register("ec_toggle_sub_tick_aiming", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_ClSubTickAiming, "Toggle sub-tick aiming");
	Console()->Register("ec_toggle_client_types", "", CFGFLAG_CLIENT, ConToggleClientTypes, Console(), "Toggle client type tags");
	Console()->Register("ec_toggle_chat_translate", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatTranslate, "Toggle translation of incoming chat messages");
	Console()->Register("ec_toggle_chat_translate_outgoing", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatTranslateOutgoing, "Toggle translation of own chat messages");
	Console()->Register("ec_toggle_chat_translate_apostrophes", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatTranslateApostrophes, "Toggle removing apostrophes from translated chat");
	Console()->Register("ec_toggle_chat_translate_show_original", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatTranslateShowOriginal, "Toggle showing the original message below the translated one");
	Console()->Register("ec_toggle_high_priority", "", CFGFLAG_CLIENT, ConToggleHighPriority, GameClient(), "Toggle high priority for the client process");	Console()->Register("ec_toggle_visual_remove_hammer_effect", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcVisualRemoveHammerEffect, "Toggle removing the hammer hit explosion effect");
	Console()->Register("ec_toggle_laser_glow", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcLaserGlow, "Toggle enhanced laser glow effect");
	Console()->Register("ec_toggle_discord_low_priority", "", CFGFLAG_CLIENT, ConToggleDiscordLowPriority, GameClient(), "Toggle lowering the priority of Discord processes");
	Console()->Register("ec_toggle_chat_mouse_selection", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatMouseSelection, "Toggle mouse cursor and message selection while the chat is open");
	Console()->Register("ec_toggle_chat_history", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatHistory, "Toggle chat history scrolling while the chat is open");
	Console()->Register("ec_toggle_ui_animations", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcUiAnimations, "Toggle smooth UI animations");
	Console()->Register("ec_toggle_chat_animation", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatAnimation, "Toggle chat animations");
	Console()->Register("ec_toggle_menu_animation", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcMenuAnimation, "Toggle menu and tab animations");
	Console()->Register("ec_toggle_emote_animation", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcEmoteAnimation, "Toggle emote menu animation");
	Console()->Register("ec_toggle_custom_communities", "", CFGFLAG_CLIENT, ConToggleCustomCommunities, GameClient(), "Toggle the custom communities list in the server browser");
}

void CMenus::RenderDOClientSettings(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollbarThickness = 10.0f;
	ScrollParams.m_ScrollbarMargin = 4.0f;
	ScrollParams.m_ScrollUnit = 60.0f;
	const CUIRect PageView = MainView;
	s_ScrollRegion.Begin(&MainView, &ScrollParams);
	const float ContentTop = MainView.y;

	CUIRect LeftColumn, RightColumn;
	MainView.VSplitMid(&LeftColumn, &RightColumn, 14.0f);

	const int DummyRows = 6;
	const int SwapRows = 3;
	const int ChatRows = 2;
	const int AnimRows = 4;
	const int PerfRows = 2;	const int VisualRows = 1;
	int InputRows = 2;
	if(g_Config.m_EcFastInput)
	{
		InputRows += 2;
		if(!g_Config.m_ClSubTickAiming)
			InputRows += 1;
	}
	const int ClientRows = 5;
	const int TranslateRows = 5;

	// card height: top padding + heading + rows + bottom padding
	auto CardHeight = [](int Rows) { return 42.0f + Rows * 20.0f; };

	CUIRect DummyCard, SwapCard, ChatCard, AnimCard, PerfCard, LaserCard, VisualCard, GradientCard, InputCard, ClientCard, TranslateCard;
	LeftColumn.HSplitTop(CardHeight(DummyRows), &DummyCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(SwapRows), &SwapCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(ChatRows), &ChatCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(AnimRows), &AnimCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(2) + 108.0f, &LaserCard, &LeftColumn);

	RightColumn.HSplitTop(CardHeight(InputRows), &InputCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(ClientRows), &ClientCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(TranslateRows), &TranslateCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(VisualRows), &VisualCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(PerfRows), &PerfCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(8), &GradientCard, &RightColumn);

	auto DrawCard = [&](const CUIRect &Card, const char *pTitle) {
		Card.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.30f), IGraphics::CORNER_ALL, 10.0f);
		CUIRect Inner;
		Card.Margin(10.0f, &Inner);
		CUIRect Title;
		Inner.HSplitTop(22.0f, &Title, &Inner);
		Ui()->DoLabel(&Title, pTitle, 16.0f, TEXTALIGN_MC);
		return Inner;
	};

	CUIRect Column = DrawCard(DummyCard, "Дамми");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyShowPosition, "Показывать позицию дамми", &g_Config.m_EcDummyShowPosition, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyShowSpeed, "Показывать скорость дамми", &g_Config.m_EcDummyShowSpeed, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyShowAngle, "Показывать угол дамми", &g_Config.m_EcDummyShowAngle, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyAdvancedActions, "Улучшенные действия дамми", &g_Config.m_EcDummyAdvancedActions, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcNoCopyWeapon, "Не копировать оружие", &g_Config.m_EcNoCopyWeapon, &Column, 20.0f);
	{
		static CButtonContainer s_NoCopyWeaponButton;
		static CButtonContainer s_NoCopyWeaponClearButton;
		static const char *s_pNoCopyWeaponCommand = "ec_toggle_no_copy_weapon";

		int BoundKey = KEY_UNKNOWN;
		for(int Key = KEY_FIRST; Key < KEY_LAST; Key++)
		{
			const char *pBind = GameClient()->m_Binds.Get(Key, KeyModifier::NONE);
			if(pBind != nullptr && str_comp(pBind, s_pNoCopyWeaponCommand) == 0)
			{
				BoundKey = Key;
				break;
			}
		}

		CUIRect BindRow;
		Column.HSplitTop(20.0f, &BindRow, &Column);
		const CKeyBinder::CKeyReaderResult Result = GameClient()->m_KeyBinder.DoKeyReader(&s_NoCopyWeaponButton, &s_NoCopyWeaponClearButton, &BindRow, CBindSlot(BoundKey, KeyModifier::NONE), false);
		if(!Result.m_Aborted && Result.m_Bind.m_Key != KEY_UNKNOWN && Result.m_Bind.m_Key != BoundKey)
			GameClient()->m_Binds.Bind(Result.m_Bind.m_Key, s_pNoCopyWeaponCommand);
	}

	Column = DrawCard(SwapCard, "Swap Time");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcSwapTimer, "Показывать таймер свапа", &g_Config.m_EcSwapTimer, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcSwapTimerOnLeftSide, "Показывать слева", &g_Config.m_EcSwapTimerOnLeftSide, &Column, 20.0f);
	CUIRect Slider;
	Column.HSplitTop(20.0f, &Slider, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcSwapTimerPosY, &g_Config.m_EcSwapTimerPosY, &Slider, "Позиция по вертикали", 0, 100, &CUi::ms_LinearScrollbarScale, 0);

	Column = DrawCard(ChatCard, "Чат");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatMouseSelection, "Мышь и выделение в чате", &g_Config.m_EcChatMouseSelection, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatHistory, "История чата (колесо, PgUp/PgDn)", &g_Config.m_EcChatHistory, &Column, 20.0f);

	Column = DrawCard(AnimCard, "Анимации");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcUiAnimations, "Плавный интерфейс", &g_Config.m_EcUiAnimations, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatAnimation, "Анимации чата (открытие, сообщения, ввод)", &g_Config.m_EcChatAnimation, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcMenuAnimation, "Анимации меню и вкладок", &g_Config.m_EcMenuAnimation, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcEmoteAnimation, "Анимация меню эмоций", &g_Config.m_EcEmoteAnimation, &Column, 20.0f);

	Column = DrawCard(PerfCard, "Производительность");
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcHighPriority, "Высокий приоритет процесса DDNet", &g_Config.m_EcHighPriority, &Column, 20.0f))
		ApplyPerformanceSettings();
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDiscordLowPriority, "Понизить приоритет процесса Discord", &g_Config.m_EcDiscordLowPriority, &Column, 20.0f))
		ApplyPerformanceSettings();

	Column = DrawCard(LaserCard, "Лазер");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcLaserGlow, "Улучшенные лазеры (свечение)", &g_Config.m_EcLaserGlow, &Column, 20.0f);
	Column.HSplitTop(20.0f, &Slider, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcLaserGlowIntensity, &g_Config.m_EcLaserGlowIntensity, &Slider, "Интенсивность", 0, 100, &CUi::ms_LinearScrollbarScale, 0);
	{
		CUIRect Preview;
		Column.HSplitTop(48.0f, &Preview, &Column);
		Preview.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 6.0f);
		Preview.Margin(6.0f, &Preview);
		DoLaserPreview(&Preview, ColorHSLA(g_Config.m_ClLaserRifleOutlineColor), ColorHSLA(g_Config.m_ClLaserRifleInnerColor), LASERTYPE_RIFLE);
		Column.HSplitTop(4.0f, nullptr, &Column);
		CUIRect PreviewShotgun;
		Column.HSplitTop(48.0f, &PreviewShotgun, &Column);
		PreviewShotgun.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 6.0f);
		PreviewShotgun.Margin(6.0f, &PreviewShotgun);
		DoLaserPreview(&PreviewShotgun, ColorHSLA(g_Config.m_ClLaserShotgunOutlineColor), ColorHSLA(g_Config.m_ClLaserShotgunInnerColor), LASERTYPE_SHOTGUN);
	}

	Column = DrawCard(VisualCard, "Визуал");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcVisualRemoveHammerEffect, "Убрать эффект хаммера", &g_Config.m_EcVisualRemoveHammerEffect, &Column, 20.0f);
	Column = DrawCard(InputCard, "Ввод");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcFastInput, "Fast Input (уменьшает визуальную задержку)", &g_Config.m_EcFastInput, &Column, 20.0f);
	Column.HSplitTop(20.0f, &Slider, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcFastInputAmount, &g_Config.m_EcFastInputAmount, &Slider, "Кол.:", 1, 40, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);
	if(g_Config.m_EcFastInput)
	{
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcFastInputOthers, "Fast Input для остальных", &g_Config.m_EcFastInputOthers, &Column, 20.0f);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClSubTickAiming, "Sub-Tick aiming", &g_Config.m_ClSubTickAiming, &Column, 20.0f);
		if(!g_Config.m_ClSubTickAiming)
			DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcFastInputRepredictHook, "Repredict при смене направления хука в середине тика (может лагать)", &g_Config.m_EcFastInputRepredictHook, &Column, 20.0f);
	}

	Column = DrawCard(GradientCard, "Градиент");
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientEnabled, "Включить градиент", &g_Config.m_EcGradientEnabled, &Column, 20.0f))
	{
	}
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientNames, "Градиент ников", &g_Config.m_EcGradientNames, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientClans, "Градиент кланов", &g_Config.m_EcGradientClans, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientSkin, "Градиент скина", &g_Config.m_EcGradientSkin, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientEverything, "Градиент всего", &g_Config.m_EcGradientEverything, &Column, 20.0f);
	CUIRect GradientSlider;
	Column.HSplitTop(20.0f, &GradientSlider, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcGradientSpeed, &g_Config.m_EcGradientSpeed, &GradientSlider, "Скорость:", 0, 100, &CUi::ms_LinearScrollbarScale, 0);

	{
		static CButtonContainer s_aGradientModeButtons[3];
		CUIRect ModeRow;
		Column.HSplitTop(20.0f, &ModeRow, &Column);
		const char *apModeNames[3] = {"Скин", "Свой", "Радуга"};
		const float ModeWidth = ModeRow.w / 3.0f;
		for(int Mode = 0; Mode < 3; Mode++)
		{
			CUIRect ModeButton;
			ModeRow.VSplitLeft(ModeWidth, &ModeButton, &ModeRow);
			if(DoButton_Menu(&s_aGradientModeButtons[Mode], apModeNames[Mode], g_Config.m_EcGradientMode == Mode, &ModeButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
				g_Config.m_EcGradientMode = Mode;
		}

		static CButtonContainer s_aGradientApplyButtons[3];
		CUIRect ApplyRow;
		Column.HSplitTop(4.0f, nullptr, &Column);
		Column.HSplitTop(20.0f, &ApplyRow, &Column);
		const char *apApplyNames[3] = {"Свои", "Другие", "Все"};
		const float ApplyWidth = ApplyRow.w / 3.0f;
		for(int Apply = 0; Apply < 3; Apply++)
		{
			CUIRect ApplyButton;
			ApplyRow.VSplitLeft(ApplyWidth, &ApplyButton, &ApplyRow);
			if(DoButton_Menu(&s_aGradientApplyButtons[Apply], apApplyNames[Apply], g_Config.m_EcGradientApply == Apply, &ApplyButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
				g_Config.m_EcGradientApply = Apply;
		}
	}

	Column = DrawCard(ClientCard, "Клиенты");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypes, "Показывать теги клиентов", &g_Config.m_EcClientTypes, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypesNamePlates, "Теги клиентов в нейм-плейтах", &g_Config.m_EcClientTypesNamePlates, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypesBestClient, "Определять BestClient по списку", &g_Config.m_EcClientTypesBestClient, &Column, 20.0f);
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcCustomCommunities, "Сообщества BestClient", &g_Config.m_EcCustomCommunities, &Column, 20.0f))
		ServerBrowser()->ReloadCommunities();
	{
		CUIRect Credit;
		Column.HSplitTop(20.0f, &Credit, &Column);
		Ui()->DoLabel(&Credit, "Данные: TaterClient/ddnet-custom-communities", 9.0f, TEXTALIGN_ML);
	}

	Column = DrawCard(TranslateCard, "Перевод чата");
	std::vector<const char *> vpLanguageNames;
	for(int i = 0; i < CChat::NumTranslateLanguages(); i++)
		vpLanguageNames.push_back(CChat::TranslateLanguageName(i));

	static CUi::SDropDownState s_IncomingDropDownState;
	static CUi::SDropDownState s_OutgoingDropDownState;
	CUIRect Row, CheckRect, DropRect;

	Column.HSplitTop(20.0f, &Row, &Column);
	Row.VSplitMid(&CheckRect, &DropRect, 0.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatTranslate, "Входящие", &g_Config.m_EcChatTranslate, &CheckRect, 20.0f);
	const int IncomingSelected = CChat::TranslateLanguageIndexOf(g_Config.m_EcChatTranslateTarget);
	const int IncomingNew = Ui()->DoDropDown(&DropRect, IncomingSelected, vpLanguageNames.data(), (int)vpLanguageNames.size(), s_IncomingDropDownState);
	if(IncomingNew != IncomingSelected)
		str_copy(g_Config.m_EcChatTranslateTarget, CChat::TranslateLanguageCode(IncomingNew));

	Column.HSplitTop(20.0f, &Row, &Column);
	Row.VSplitMid(&CheckRect, &DropRect, 0.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatTranslateOutgoing, "Свои сообщения", &g_Config.m_EcChatTranslateOutgoing, &CheckRect, 20.0f);
	const int OutgoingSelected = CChat::TranslateLanguageIndexOf(g_Config.m_EcChatTranslateOutTarget);
	const int OutgoingNew = Ui()->DoDropDown(&DropRect, OutgoingSelected, vpLanguageNames.data(), (int)vpLanguageNames.size(), s_OutgoingDropDownState);
	if(OutgoingNew != OutgoingSelected)
		str_copy(g_Config.m_EcChatTranslateOutTarget, CChat::TranslateLanguageCode(OutgoingNew));

	Column.HSplitTop(20.0f, &Row, &Column);
	{
		static const char *s_apBackendNames[] = {"Auto", "Bing", "Google"};
		static CUi::SDropDownState s_BackendDropDownState;
		CUIRect BackendLabel, BackendDrop;
		Row.VSplitLeft(60.0f, &BackendLabel, &BackendDrop);
		Ui()->DoLabel(&BackendLabel, "Бэкенд:", 10.0f, TEXTALIGN_ML);
		const int BackendNew = Ui()->DoDropDown(&BackendDrop, g_Config.m_EcChatTranslateBackend, s_apBackendNames, (int)std::size(s_apBackendNames), s_BackendDropDownState);
		if(BackendNew != g_Config.m_EcChatTranslateBackend)
			g_Config.m_EcChatTranslateBackend = BackendNew;
	}

	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatTranslateApostrophes, "Убрать апострофы", &g_Config.m_EcChatTranslateApostrophes, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatTranslateShowOriginal, "Показывать оригинал снизу", &g_Config.m_EcChatTranslateShowOriginal, &Column, 20.0f);

	const float UsedHeight = std::max(PageView.h - LeftColumn.h, PageView.h - RightColumn.h);
	CUIRect ContentRect = {PageView.x, ContentTop, PageView.w, UsedHeight};
	s_ScrollRegion.AddRect(ContentRect);
	s_ScrollRegion.End();
}

void CMenus::RenderSettingsDOClient(CUIRect MainView)
{
	static int s_SubTab = 0;

	CUIRect TabBar, Content;
	MainView.HSplitTop(20.0f, &TabBar, &Content);
	const float SubTabWidth = TabBar.w / 2.0f;
	static CButtonContainer s_aSubTabButtons[2];
	const char *apSubTabNames[2] = {"Настройки", "Список войн"};
	for(int Tab = 0; Tab < 2; Tab++)
	{
		CUIRect Button;
		TabBar.VSplitLeft(SubTabWidth, &Button, &TabBar);
		const int Corners = Tab == 0 ? IGraphics::CORNER_L : IGraphics::CORNER_R;
		if(DoButton_MenuTab(&s_aSubTabButtons[Tab], apSubTabNames[Tab], s_SubTab == Tab, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			s_SubTab = Tab;
	}

	Content.HSplitTop(18.0f, nullptr, &Content);

	static int s_LastSubTab = -1;
	static int64_t s_SubTabAnimStart = 0;
	if(s_LastSubTab != s_SubTab)
	{
		s_LastSubTab = s_SubTab;
		s_SubTabAnimStart = time();
	}

	auto RenderSubTab = [&](const CUIRect &View) {
		if(s_SubTab == 0)
			RenderDOClientSettings(View);
		else
			RenderDOClientWarList(View);
	};

	const bool Animations = g_Config.m_EcUiAnimations && g_Config.m_EcMenuAnimation;
	const float Age = s_SubTabAnimStart != 0 ? (time() - s_SubTabAnimStart) / (float)time_freq() : 1.0f;
	if(Animations && s_SubTabAnimStart != 0 && Age < 0.45f)
	{
		const float Progress = std::clamp(Age / 0.45f, 0.0f, 1.0f);
		const float Inv = 1.0f - Progress;
		const float Eased = 1.0f - Inv * Inv * Inv;
		const CUIRect Panel = Content;
		Ui()->ClipEnable(&Panel);

		if(s_SubTab == 1)
		{
			// warlist: three columns (left, center, right)
			const float ShiftH = (1.0f - Eased) * Content.w;
			const float ShiftV = (1.0f - Eased) * Content.h;
			const float LeftW = Content.w * 0.40f;
			const float CenterX = LeftW + 8.0f;
			const float CenterW = Content.w * 0.33f;
			const float RightX = CenterX + CenterW + 8.0f;

			CUIRect ClipLeft = Content;
			ClipLeft.w = LeftW;
			ClipLeft.x -= ShiftH;
			CUIRect ViewLeft = Content;
			ViewLeft.x -= ShiftH;
			Ui()->ClipEnable(&ClipLeft);
			RenderSubTab(ViewLeft);
			Ui()->ClipDisable();

			CUIRect ClipCenter = Content;
			ClipCenter.x += CenterX;
			ClipCenter.w = CenterW;
			ClipCenter.y -= ShiftV;
			CUIRect ViewCenter = Content;
			ViewCenter.y -= ShiftV;
			Ui()->ClipEnable(&ClipCenter);
			RenderSubTab(ViewCenter);
			Ui()->ClipDisable();

			CUIRect ClipRight = Content;
			ClipRight.x += RightX + ShiftH;
			ClipRight.w = Content.w - RightX;
			CUIRect ViewRight = Content;
			ViewRight.x += ShiftH;
			Ui()->ClipEnable(&ClipRight);
			RenderSubTab(ViewRight);
			Ui()->ClipDisable();
		}
		else
		{
			// settings: the original two-half animation of the settings page
			const float HalfWidth = Content.w * 0.5f;
			const float Shift = (1.0f - Eased) * HalfWidth;

			CUIRect ClipLeft = Content;
			ClipLeft.w = HalfWidth;
			ClipLeft.x -= Shift;
			CUIRect ViewLeft = Content;
			ViewLeft.x -= Shift;
			Ui()->ClipEnable(&ClipLeft);
			RenderSubTab(ViewLeft);
			Ui()->ClipDisable();

			CUIRect ClipRight = Content;
			ClipRight.x += HalfWidth + Shift;
			ClipRight.w = Content.w - HalfWidth;
			CUIRect ViewRight = Content;
			ViewRight.x += Shift;
			Ui()->ClipEnable(&ClipRight);
			RenderSubTab(ViewRight);
			Ui()->ClipDisable();
		}

		Ui()->ClipDisable();
	}
	else
	{
		RenderSubTab(Content);
	}
}

void CMenus::RenderDOClientWarList(CUIRect MainView)
{
	CWarList *pWarList = &GameClient()->m_WarList;
	if(pWarList->m_vTypes.empty())
	{
		Ui()->DoLabel(&MainView, "Список войн недоступен", 16.0f, TEXTALIGN_MC);
		return;
	}

	static CLineInputBuffered<64> s_NameInput;
	static CLineInputBuffered<32> s_ClanInput;
	static CLineInputBuffered<128> s_ReasonInput;
	static int s_SelectedType = 1;
	static int s_SelectedEntry = -1;
	static int s_SelectedGroup = 1;
	static CUi::SDropDownState s_TypeDropDownState;

	CUIRect Left, Middle, Right;
	MainView.VSplitLeft(MainView.w * 0.40f, &Left, &Middle);
	Middle.VSplitLeft(8.0f, nullptr, &Middle);
	Middle.VSplitLeft(MainView.w * 0.33f, &Middle, &Right);
	Right.VSplitLeft(8.0f, &Right, &Right);

	// ***** Left: entries ***** //
	{
		CUIRect Header;
		Left.HSplitTop(24.0f, &Header, &Left);
		Ui()->DoLabel(&Header, "Игроки и кланы", 16.0f, TEXTALIGN_ML);
		Left.HSplitTop(4.0f, nullptr, &Left);

		static CListBox s_ListBox;
		s_ListBox.DoStart(20.0f, pWarList->m_vEntries.size(), 1, 3, s_SelectedEntry, &Left, true);
		for(size_t i = 0; i < pWarList->m_vEntries.size(); i++)
		{
			CWarListEntry &Entry = pWarList->m_vEntries[i];
			char aText[128];
			if(Entry.m_aName[0] != '\0')
				str_format(aText, sizeof(aText), "%s", Entry.m_aName);
			else
				str_format(aText, sizeof(aText), "clan %s", Entry.m_aClan);

			const CListboxItem Item = s_ListBox.DoNextItem(&Entry, s_SelectedEntry == (int)i);
			if(!Item.m_Visible)
				continue;

			CUIRect Label, DeleteButton;
			Item.m_Rect.VSplitRight(20.0f, &Label, &DeleteButton);

			const ColorRGBA Color = (Entry.m_Type >= 0 && Entry.m_Type < (int)pWarList->m_vTypes.size()) ? pWarList->m_vTypes[Entry.m_Type].m_Color : ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
			TextRender()->TextColor(Color);
			Ui()->DoLabel(&Label, aText, 14.0f, TEXTALIGN_ML);
			TextRender()->TextColor(TextRender()->DefaultTextColor());

			static CButtonContainer s_aDeleteButtons[64];
			if(DoButton_Menu(&s_aDeleteButtons[i % 64], "x", 0, &DeleteButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 3.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f)))
			{
				pWarList->RemoveEntry((int)i);
				s_SelectedEntry = -1;
				break;
			}
		}
		s_SelectedEntry = s_ListBox.DoEnd();

	}

	// ***** Middle: editing ***** //
	{
		CUIRect Header, Row, Label;
		Middle.HSplitTop(24.0f, &Header, &Middle);
		Ui()->DoLabel(&Header, "Редактирование", 16.0f, TEXTALIGN_ML);
		Middle.HSplitTop(4.0f, nullptr, &Middle);

		Middle.HSplitTop(20.0f, &Row, &Middle);
		Row.VSplitLeft(50.0f, &Label, &Row);
		Ui()->DoLabel(&Label, "Имя:", 14.0f, TEXTALIGN_ML);
		Ui()->DoClearableEditBox(&s_NameInput, &Row, 14.0f);

		Middle.HSplitTop(4.0f, nullptr, &Middle);
		Middle.HSplitTop(20.0f, &Row, &Middle);
		Row.VSplitLeft(50.0f, &Label, &Row);
		Ui()->DoLabel(&Label, "Клан:", 14.0f, TEXTALIGN_ML);
		Ui()->DoClearableEditBox(&s_ClanInput, &Row, 14.0f);

		Middle.HSplitTop(4.0f, nullptr, &Middle);
		Middle.HSplitTop(20.0f, &Row, &Middle);
		Row.VSplitLeft(50.0f, &Label, &Row);
		Ui()->DoLabel(&Label, "Причина:", 14.0f, TEXTALIGN_ML);
		Ui()->DoClearableEditBox(&s_ReasonInput, &Row, 14.0f);

		Middle.HSplitTop(4.0f, nullptr, &Middle);
		Middle.HSplitTop(20.0f, &Row, &Middle);
		Row.VSplitLeft(50.0f, &Label, &Row);
		Ui()->DoLabel(&Label, "Группа:", 14.0f, TEXTALIGN_ML);

		static const char *s_apTypeNames[32];
		const int NumTypes = std::min<int>((int)pWarList->m_vTypes.size(), 32);
		for(int i = 0; i < NumTypes; i++)
			s_apTypeNames[i] = pWarList->m_vTypes[i].m_aName;
		const int NewType = Ui()->DoDropDown(&Row, s_SelectedType, s_apTypeNames, NumTypes, s_TypeDropDownState);
		if(NewType != s_SelectedType)
			s_SelectedType = NewType;

		Middle.HSplitTop(8.0f, nullptr, &Middle);
		CUIRect ButtonsRow;
		Middle.HSplitTop(22.0f, &ButtonsRow, &Middle);
		CUIRect AddNameButton, AddClanButton;
		ButtonsRow.VSplitLeft(ButtonsRow.w / 2.0f - 3.0f, &AddNameButton, nullptr);
		ButtonsRow.VSplitRight(ButtonsRow.w / 2.0f - 3.0f, nullptr, &AddClanButton);

		static CButtonContainer s_AddNameButton, s_AddClanButton, s_RemoveButton;
		const bool HasName = s_NameInput.GetString()[0] != '\0';
		const bool HasClan = s_ClanInput.GetString()[0] != '\0';
		const int Type = (s_SelectedType >= 0 && s_SelectedType < NumTypes) ? s_SelectedType : 1;

		if(DoButton_Menu(&s_AddNameButton, "Добавить игрока", HasName ? 0 : 2, &AddNameButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f)) && HasName)
		{
			pWarList->AddEntry(s_NameInput.GetString(), "", s_ReasonInput.GetString(), pWarList->m_vTypes[Type].m_aName);
			s_NameInput.Clear();
		}
		if(DoButton_Menu(&s_AddClanButton, "Добавить клан", HasClan ? 0 : 2, &AddClanButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f)) && HasClan)
		{
			pWarList->AddEntry("", s_ClanInput.GetString(), s_ReasonInput.GetString(), pWarList->m_vTypes[Type].m_aName);
			s_ClanInput.Clear();
		}

		Middle.HSplitTop(8.0f, nullptr, &Middle);
		CUIRect RemoveRow;
		Middle.HSplitTop(22.0f, &RemoveRow, &Middle);
		if(DoButton_Menu(&s_RemoveButton, "Удалить выбранное", s_SelectedEntry < 0 ? 2 : 0, &RemoveRow, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 4.0f, 0.0f, ColorRGBA(0.4f, 0.0f, 0.0f, 0.3f)) && s_SelectedEntry >= 0)
		{
			pWarList->RemoveEntry(s_SelectedEntry);
			s_SelectedEntry = -1;
		}
	}

	// ***** Right: groups ***** //
	{
		CUIRect Header;
		Right.HSplitTop(24.0f, &Header, &Right);
		Ui()->DoLabel(&Header, "Группы", 16.0f, TEXTALIGN_ML);
		Right.HSplitTop(4.0f, nullptr, &Right);

		static CButtonContainer s_aGroupRows[16];
		for(int i = 0; i < (int)pWarList->m_vTypes.size(); i++)
		{
			CUIRect Row, Label, Swatch, DeleteButton, ColorSquare;
			Right.HSplitTop(22.0f, &Row, &Right);
			Row.VSplitRight(20.0f, &Row, &DeleteButton);
			Row.VSplitRight(26.0f, &Label, &Swatch);

			if(Ui()->DoButtonLogic(&s_aGroupRows[i % 16], s_SelectedGroup == i, &Row, BUTTONFLAG_LEFT))
				s_SelectedGroup = i;
			if(s_SelectedGroup == i)
				Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.12f), IGraphics::CORNER_ALL, 4.0f);

			// text first, then the color square
			CUIRect LabelRect = Label;
			LabelRect.VMargin(4.0f, &LabelRect);
			Ui()->DoLabel(&LabelRect, pWarList->m_vTypes[i].m_aName, 14.0f, TEXTALIGN_ML);

			ColorSquare = Swatch;
			ColorSquare.Margin(2.0f, &ColorSquare);
			ColorSquare.Draw(pWarList->m_vTypes[i].m_Color, IGraphics::CORNER_ALL, 4.0f);

			if(pWarList->m_vTypes[i].m_Removable)
			{
				static CButtonContainer s_aGroupDeleteButtons[16];
				if(DoButton_Menu(&s_aGroupDeleteButtons[i % 16], "x", 0, &DeleteButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 3.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f)))
					pWarList->RemoveType(i);
			}
		}

		if(s_SelectedGroup >= 0 && s_SelectedGroup < (int)pWarList->m_vTypes.size())
		{
			Right.HSplitTop(8.0f, nullptr, &Right);
			CUIRect PickerRow, PickerSquare, PickerLabel;
			Right.HSplitTop(24.0f, &PickerRow, &Right);
			PickerRow.VSplitLeft(48.0f, &PickerSquare, &PickerRow);
			unsigned HslaColor = color_cast<ColorHSLA>(pWarList->m_vTypes[s_SelectedGroup].m_Color).Pack(false);
			const ColorHSLA Picked = DoButton_ColorPicker(&PickerSquare, &HslaColor, false);
			const ColorRGBA PickedColor = color_cast<ColorRGBA>(Picked);
			if(PickedColor != pWarList->m_vTypes[s_SelectedGroup].m_Color)
			{
				pWarList->SetTypeColor(s_SelectedGroup, PickedColor);
				if(!Ui()->IsPopupOpen())
					pWarList->Save();
			}
			PickerLabel = PickerRow;
			PickerLabel.x += 8.0f;
			Ui()->DoLabel(&PickerLabel, "Цвет выбранной группы", 14.0f, TEXTALIGN_ML);
		}

		Right.HSplitTop(14.0f, nullptr, &Right);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcWarListColorsTab, "Цвета в табло", &g_Config.m_EcWarListColorsTab, &Right, 20.0f);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcWarListColorsSpectator, "Цвета в наблюдателях", &g_Config.m_EcWarListColorsSpectator, &Right, 20.0f);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcWarListColorsNamePlates, "Цвета в нейм-плейтах", &g_Config.m_EcWarListColorsNamePlates, &Right, 20.0f);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcWarListColorsChat, "Цвета в чате", &g_Config.m_EcWarListColorsChat, &Right, 20.0f);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcWarListShowReason, "Показывать причины", &g_Config.m_EcWarListShowReason, &Right, 20.0f);
		}
	}
