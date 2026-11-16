#include <algorithm>

#include <engine/shared/config.h>

#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>

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

	auto CardHeight = [](int Rows) { return 42.0f + Rows * 20.0f; };

	const int ClientRows = 4;
	const int GradientRows = 7;
	const int SpectatorRows = 3;
	const int SwapRows = 4;
	const int DummyRows = 7;
	const int LaserRows = 3;
	const int AnimRows = 5;

	CUIRect ClientCard, GradientCard, SpectatorCard, SwapCard, DummyCard, LaserCard, AnimCard;
	LeftColumn.HSplitTop(CardHeight(ClientRows), &ClientCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(GradientRows), &GradientCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(SpectatorRows), &SpectatorCard, &LeftColumn);
	LeftColumn.HSplitTop(12.0f, nullptr, &LeftColumn);
	LeftColumn.HSplitTop(CardHeight(SwapRows), &SwapCard, &LeftColumn);

	RightColumn.HSplitTop(CardHeight(DummyRows), &DummyCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(LaserRows) + 112.0f, &LaserCard, &RightColumn);
	RightColumn.HSplitTop(12.0f, nullptr, &RightColumn);
	RightColumn.HSplitTop(CardHeight(AnimRows), &AnimCard, &RightColumn);

	auto DrawCard = [&](const CUIRect &Card, const char *pTitle) {
		Card.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.30f), IGraphics::CORNER_ALL, 10.0f);
		CUIRect Inner;
		Card.Margin(10.0f, &Inner);
		CUIRect Title;
		Inner.HSplitTop(22.0f, &Title, &Inner);
		Ui()->DoLabel(&Title, pTitle, 16.0f, TEXTALIGN_MC);
		return Inner;
	};

	// ---------------- Клиенты ----------------
	CUIRect Column = DrawCard(ClientCard, "Клиенты");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypes, "Показывать теги клиентов", &g_Config.m_EcClientTypes, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypesNamePlates, "Теги клиентов в нейм-плейтах", &g_Config.m_EcClientTypesNamePlates, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcClientTypesBestClient, "Определять BestClient (тег BC)", &g_Config.m_EcClientTypesBestClient, &Column, 20.0f);
	Ui()->DoLabel(&Column, "[DO] DOClient · [EC] Entity · [RC] Rushie", 9.0f, TEXTALIGN_ML);

	// ---------------- Градиент ----------------
	Column = DrawCard(GradientCard, "Градиент");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientEnabled, "Включить градиент", &g_Config.m_EcGradientEnabled, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientNames, "Градиент ников", &g_Config.m_EcGradientNames, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientClans, "Градиент кланов", &g_Config.m_EcGradientClans, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcGradientSkin, "Градиент скина", &g_Config.m_EcGradientSkin, &Column, 20.0f);
	CUIRect GradMode, GradSpeed, GradApply;
	Column.HSplitTop(20.0f, &GradMode, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcGradientMode, &g_Config.m_EcGradientMode, &GradMode, "Режим (0 скин / 1 свой / 2 радуга):", 0, 2, &CUi::ms_LinearScrollbarScale, 0);
	Column.HSplitTop(20.0f, &GradSpeed, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcGradientSpeed, &g_Config.m_EcGradientSpeed, &GradSpeed, "Скорость:", 0, 100, &CUi::ms_LinearScrollbarScale, 0);
	Column.HSplitTop(20.0f, &GradApply, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcGradientApply, &g_Config.m_EcGradientApply, &GradApply, "Применять (0 себе / 1 другим / 2 всем):", 0, 2, &CUi::ms_LinearScrollbarScale, 0);

	// ---------------- Наблюдатели ----------------
	Column = DrawCard(SpectatorCard, "Наблюдатели");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcShowSpectators, "Показывать кто за мной наблюдает", &g_Config.m_EcShowSpectators, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcSpectatorsShowNames, "Показывать ники наблюдателей", &g_Config.m_EcSpectatorsShowNames, &Column, 20.0f);
	Ui()->DoLabel(&Column, "ники показываются под счётчиком HUD", 9.0f, TEXTALIGN_ML);

	// ---------------- Свап-таймер ----------------
	Column = DrawCard(SwapCard, "Свап-таймер");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcSwapTimer, "Показывать таймер свапа", &g_Config.m_EcSwapTimer, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcSwapTimerOnLeftSide, "Показывать слева", &g_Config.m_EcSwapTimerOnLeftSide, &Column, 20.0f);
	CUIRect SwapSlider;
	Column.HSplitTop(20.0f, &SwapSlider, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcSwapTimerPosY, &g_Config.m_EcSwapTimerPosY, &SwapSlider, "Позиция по вертикали:", 0, 100, &CUi::ms_LinearScrollbarScale, 0);
	Ui()->DoLabel(&Column, "показывает ожидающие запросы на свап", 9.0f, TEXTALIGN_ML);

	// ---------------- Дамми ----------------
	Column = DrawCard(DummyCard, "Дамми");
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
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyShowPosition, "Позиция дамми в HUD", &g_Config.m_EcDummyShowPosition, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyShowSpeed, "Скорость дамми в HUD", &g_Config.m_EcDummyShowSpeed, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcDummyShowAngle, "Угол дамми в HUD", &g_Config.m_EcDummyShowAngle, &Column, 20.0f);
	Ui()->DoLabel(&Column, "иконки в HUD показывают состояние", 9.0f, TEXTALIGN_ML);

	// ---------------- Лазер ----------------
	Column = DrawCard(LaserCard, "Лазер");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcLaserGlow, "Улучшенные лазеры (свечение)", &g_Config.m_EcLaserGlow, &Column, 20.0f);
	CUIRect LaserSlider;
	Column.HSplitTop(20.0f, &LaserSlider, &Column);
	Ui()->DoScrollbarOption(&g_Config.m_EcLaserGlowIntensity, &g_Config.m_EcLaserGlowIntensity, &LaserSlider, "Интенсивность:", 0, 100, &CUi::ms_LinearScrollbarScale, 0);
	{
		CUIRect RiflePreview;
		Column.HSplitTop(48.0f, &RiflePreview, &Column);
		RiflePreview.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 6.0f);
		RiflePreview.Margin(6.0f, &RiflePreview);
		DoLaserPreview(&RiflePreview, ColorHSLA(g_Config.m_ClLaserRifleOutlineColor), ColorHSLA(g_Config.m_ClLaserRifleInnerColor), LASERTYPE_RIFLE);
	}
	Column.HSplitTop(4.0f, nullptr, &Column);
	{
		CUIRect ShotgunPreview;
		Column.HSplitTop(48.0f, &ShotgunPreview, &Column);
		ShotgunPreview.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 6.0f);
		ShotgunPreview.Margin(6.0f, &ShotgunPreview);
		DoLaserPreview(&ShotgunPreview, ColorHSLA(g_Config.m_ClLaserShotgunOutlineColor), ColorHSLA(g_Config.m_ClLaserShotgunInnerColor), LASERTYPE_SHOTGUN);
	}
	Ui()->DoLabel(&Column, "мягкое свечение вокруг лазеров", 9.0f, TEXTALIGN_ML);

	// ---------------- Анимации ----------------
	Column = DrawCard(AnimCard, "Анимации");
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcUiAnimations, "Плавный интерфейс", &g_Config.m_EcUiAnimations, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcChatAnimation, "Анимации чата (открытие)", &g_Config.m_EcChatAnimation, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcMenuAnimation, "Анимации меню и вкладок", &g_Config.m_EcMenuAnimation, &Column, 20.0f);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_EcEmoteAnimation, "Анимация меню эмоций", &g_Config.m_EcEmoteAnimation, &Column, 20.0f);
	Ui()->DoLabel(&Column, "колесо эмоций и вкладки Entity анимируют сами", 9.0f, TEXTALIGN_ML);

	// register the used content height so the page can be scrolled
	const float UsedHeight = std::max(PageView.h - LeftColumn.h, PageView.h - RightColumn.h);
	CUIRect ContentRect = {PageView.x, ContentTop, PageView.w, UsedHeight};
	s_ScrollRegion.AddRect(ContentRect);
	s_ScrollRegion.End();
}
