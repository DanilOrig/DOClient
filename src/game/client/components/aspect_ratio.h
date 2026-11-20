#ifndef GAME_CLIENT_COMPONENTS_ASPECT_RATIO_H
#define GAME_CLIENT_COMPONENTS_ASPECT_RATIO_H

#include <engine/console.h>

#include <game/client/component.h>

class CAspectRatio : public CComponent
{
	struct SConfig
	{
		int m_Mode = 0;
		int m_Ratio = 133;
		int m_Num = 16;
		int m_Den = 9;

		bool operator==(const SConfig &Other) const { return m_Mode == Other.m_Mode && m_Ratio == Other.m_Ratio && m_Num == Other.m_Num && m_Den == Other.m_Den; }
		bool operator!=(const SConfig &Other) const { return !(*this == Other); }
	};

	static void ConToggleAspect(IConsole::IResult *pResult, void *pUserData);

	SConfig ReadConfig() const;
	void WriteConfig(const SConfig &Config);
	float TargetAspect() const;
	void Apply();

	bool m_Seeded = false;
	bool m_ConfirmActive = false;
	int64_t m_ConfirmDeadline = 0;
	SConfig m_Last;
	SConfig m_Prev;
	float m_AppliedAspect = -1.0f;

public:
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnReset() override;
};

#endif
