#include "menus.h"



#include <base/str.h>



#include <engine/serverbrowser.h>

#include <engine/shared/config.h>



#include <game/client/components/chat.h>

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

	Console()->Register("ec_toggle_stt", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcStt, "Toggle push-to-talk speech recognition to chat");

	Console()->Register("ec_toggle_high_priority", "", CFGFLAG_CLIENT, ConToggleHighPriority, GameClient(), "Toggle high priority for the client process");
	Console()->Register("ec_toggle_visual_remove_hammer_effect", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcVisualRemoveHammerEffect, "Toggle removing the hammer hit explosion effect");

	Console()->Register("ec_toggle_discord_low_priority", "", CFGFLAG_CLIENT, ConToggleDiscordLowPriority, GameClient(), "Toggle lowering the priority of Discord processes");

	Console()->Register("ec_toggle_chat_mouse_selection", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatMouseSelection, "Toggle mouse cursor and message selection while the chat is open");

	Console()->Register("ec_toggle_chat_history", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatHistory, "Toggle chat history scrolling while the chat is open");

	Console()->Register("ec_toggle_ui_animations", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcUiAnimations, "Toggle smooth UI animations");

	Console()->Register("ec_toggle_chat_animation", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcChatAnimation, "Toggle chat animations");

	Console()->Register("ec_toggle_menu_animation", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcMenuAnimation, "Toggle menu and tab animations");

	Console()->Register("ec_toggle_emote_animation", "", CFGFLAG_CLIENT, ConToggleConfig, &g_Config.m_EcEmoteAnimation, "Toggle emote menu animation");

	Console()->Register("ec_toggle_custom_communities", "", CFGFLAG_CLIENT, ConToggleCustomCommunities, GameClient(), "Toggle the custom communities list in the server browser");

}



void CMenus::RenderSettingsDOClient(CUIRect MainView)

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

	const int PerfRows = 2;
	const int VisualRows = 1;

	int InputRows = 2;

	if(g_Config.m_EcFastInput)

	{

		InputRows += 2;

		if(!g_Config.m_ClSubTickAiming)

			InputRows += 1;

	}

	const int ClientRows = 4;

	const int TranslateRows = 5;

	const int VoiceRows = 3;



	// card height: top padding + heading + rows + bottom padding

	auto CardHeight = [](int Rows) { return 42.0f + Rows * 20.0f; };



	CUIRect DummyCard, SwapCard, ChatCard, AnimCard, PerfCard, VisualCard, InputCard, ClientCard, TranslateCard, VoiceCard;

	LeftColumn.HSplitTop(CardHeight(DummyRows), &DummyCard, &LeftColumn);

	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);

	LeftColumn.HSplitTop(CardHeight(SwapRows), &SwapCard, &LeftColumn);

	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);

	LeftColumn.HSplitTop(CardHeight(ChatRows), &ChatCard, &LeftColumn);

	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);

	LeftColumn.HSplitTop(CardHeight(AnimRows), &AnimCard, &LeftColumn);

	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);

	LeftColumn.HSplitTop(CardHeight(PerfRows), &PerfCard, &LeftColumn);



	RightColumn.HSplitTop(CardHeight(InputRows), &InputCard, &RightColumn);

	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);

	RightColumn.HSplitTop(CardHeight(ClientRows), &ClientCard, &RightColumn);

	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);

	RightColumn.HSplitTop(CardHeight(TranslateRows), &TranslateCard, &RightColumn);

	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);

	RightColumn.HSplitTop(CardHeight(VoiceRows), &VoiceCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(VisualRows), &VisualCard, &RightColumn);



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



	Column = DrawCard(ClientCard, "Клиенты");

	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypes, "Показывать теги клиентов", &g_Config.m_EcClientTypes, &Column, 20.0f);

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



	Column = DrawCard(VoiceCard, "Голос (STT)");

	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcStt, "Push-to-talk в чат (офлайн)", &g_Config.m_EcStt, &Column, 20.0f);



	{

		static const char *s_apSttModels[] = {"Русский", "English"};

		static CUi::SDropDownState s_SttModelDropDownState;

		CUIRect ModelLabel, ModelDrop;

		Column.HSplitTop(20.0f, &Row, &Column);

		Row.VSplitLeft(60.0f, &ModelLabel, &ModelDrop);

		Ui()->DoLabel(&ModelLabel, "Модель:", 10.0f, TEXTALIGN_ML);

		const int ModelNew = Ui()->DoDropDown(&ModelDrop, g_Config.m_EcSttLang, s_apSttModels, (int)std::size(s_apSttModels), s_SttModelDropDownState);

		if(ModelNew != g_Config.m_EcSttLang)

			g_Config.m_EcSttLang = ModelNew;

	}



	{

		static CButtonContainer s_SttKeyButton;

		static CButtonContainer s_SttKeyClearButton;

		CUIRect KeyLabel, KeyReader;

		Column.HSplitTop(20.0f, &Row, &Column);

		Row.VSplitLeft(60.0f, &KeyLabel, &KeyReader);

		Ui()->DoLabel(&KeyLabel, "Клавиша:", 10.0f, TEXTALIGN_ML);

		const int OldSttKey = CStt::ParseKey(g_Config.m_EcSttKey);

		const CKeyBinder::CKeyReaderResult KeyResult = GameClient()->m_KeyBinder.DoKeyReader(&s_SttKeyButton, &s_SttKeyClearButton, &KeyReader, CBindSlot(OldSttKey, KeyModifier::NONE), false);

		if(!KeyResult.m_Aborted && KeyResult.m_Bind.m_Key != KEY_UNKNOWN && KeyResult.m_Bind.m_Key != OldSttKey)

		{

			char aName[32];

			GameClient()->m_Binds.GetKeyBindName(KeyResult.m_Bind.m_Key, KeyModifier::NONE, aName, sizeof(aName));

			for(char *p = aName; *p; p++)

				*p = (char)tolower((unsigned char)*p);

			str_copy(g_Config.m_EcSttKey, aName);

		}

	}

	const float UsedHeight = std::max(PageView.h - LeftColumn.h, PageView.h - RightColumn.h);
	CUIRect ContentRect = {PageView.x, ContentTop, PageView.w, UsedHeight};
	s_ScrollRegion.AddRect(ContentRect);
	s_ScrollRegion.End();
}

