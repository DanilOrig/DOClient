#ifndef GAME_CLIENT_COMPONENTS_GRADIENT_H
#define GAME_CLIENT_COMPONENTS_GRADIENT_H

#include <base/color.h>

#include <engine/console.h>
#include <engine/shared/protocol.h>

#include <game/client/component.h>

#include <string>
#include <utility>
#include <unordered_map>

enum
{
	GRADIENT_MODE_SKIN = 0,
	GRADIENT_MODE_CUSTOM = 1,
	GRADIENT_MODE_RAINBOW = 2,

	GRADIENT_APPLY_OWN = 0,
	GRADIENT_APPLY_OTHERS = 1,
	GRADIENT_APPLY_ALL = 2,
};

class CGradient : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;

	bool Enabled() const;
	bool AppliesTo(int ClientId) const;
	bool NamesEnabled() const;
	bool ClansEnabled() const;
	bool SkinEnabled() const;

	ColorRGBA ColorFor(int ClientId) const;

private:
	static void ConGradientDebug(IConsole::IResult *pResult, void *pUserData);
	float Phase() const;
	float BaseHue(int ClientId) const;
	ColorHSLA BaseColor(int ClientId) const;
	ColorRGBA SkinColorFor(int ClientId) const;
	void SkinColorsFor(int ClientId, ColorRGBA *pFirst, ColorRGBA *pSecond) const;

	mutable std::unordered_map<std::string, ColorRGBA> m_SkinColorCache;
	mutable std::unordered_map<std::string, std::pair<ColorRGBA, ColorRGBA>> m_SkinPaletteCache;
};

#endif
