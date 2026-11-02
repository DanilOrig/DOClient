#include "gradient.h"

#include <base/math.h>
#include <base/time.h>

#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/image.h>
#include <engine/storage.h>

#include <game/client/gameclient.h>

bool CGradient::Enabled() const
{
	return g_Config.m_EcGradientEnabled;
}

bool CGradient::NamesEnabled() const
{
	if(!Enabled())
		return false;
	if(g_Config.m_EcGradientEverything)
		return true;
	return g_Config.m_EcGradientNames;
}

bool CGradient::ClansEnabled() const
{
	if(!Enabled())
		return false;
	if(g_Config.m_EcGradientEverything)
		return true;
	return g_Config.m_EcGradientClans;
}

bool CGradient::SkinEnabled() const
{
	if(!Enabled())
		return false;
	if(g_Config.m_EcGradientEverything)
		return true;
	return g_Config.m_EcGradientSkin;
}

float CGradient::Phase() const
{
	const float Speed = std::clamp(g_Config.m_EcGradientSpeed, 0, 100) / 100.0f;
	const double Seconds = (double)time_get() / (double)time_freq();
	return (float)(Seconds * (0.05f + Speed * 0.45f));
}

float CGradient::BaseHue(int ClientId) const
{
	if(g_Config.m_EcGradientMode == GRADIENT_MODE_RAINBOW)
		return 0.0f;

	if(g_Config.m_EcGradientMode == GRADIENT_MODE_CUSTOM)
		return ColorHSLA(g_Config.m_EcGradientColor, true).h;

	if(ClientId >= 0 && ClientId < MAX_CLIENTS)
	{
		const auto &Client = GameClient()->m_aClients[ClientId];
		// use the color the tee is actually rendered with, so custom colors and skin colors both work
		const ColorRGBA RenderColor = Client.m_RenderInfo.m_ColorBody;
		const float RenderHue = RgbToHue(RenderColor.r, RenderColor.g, RenderColor.b);
		if(RenderColor.r < 0.01f && RenderColor.g < 0.01f && RenderColor.b < 0.01f)
			return ColorHSLA((unsigned)Client.m_ColorBody, true).h;
		return RenderHue;
	}
	return 0.0f;
}

ColorHSLA CGradient::BaseColor(int ClientId) const
{
	if(ClientId >= 0 && ClientId < MAX_CLIENTS)
	{
		const auto &Client = GameClient()->m_aClients[ClientId];
		const ColorRGBA RenderColor = Client.m_RenderInfo.m_ColorBody;
		ColorHSLA Color = color_cast<ColorHSLA>(RenderColor);
		// fall back to the custom body color when the render color is unusable
		if(Color.s < 0.01f && Color.l < 0.01f)
			Color = color_cast<ColorHSLA>(color_cast<ColorRGBA>(ColorHSLA((unsigned)Client.m_ColorBody, true)));
		return Color;
	}
	return ColorHSLA(0.0f, 1.0f, 0.5f, 1.0f);
}

bool CGradient::AppliesTo(int ClientId) const
{
	if(!Enabled())
		return false;

	bool IsLocal = false;
	for(const int LocalId : GameClient()->m_aLocalIds)
	{
		if(LocalId == ClientId)
		{
			IsLocal = true;
			break;
		}
	}

	if(g_Config.m_EcGradientApply == GRADIENT_APPLY_ALL)
		return true;
	if(g_Config.m_EcGradientApply == GRADIENT_APPLY_OWN)
		return IsLocal;
	return !IsLocal;
}

ColorRGBA CGradient::ColorFor(int ClientId) const
{
	// per-client offset so names flow at different phases instead of blinking in sync
	const float Offset = ClientId >= 0 ? ClientId * 0.137f : 0.0f;
	const float P = Phase() + Offset;
	const float Wave = std::sin(P * 2.0f * pi);
	const float Wave2 = std::sin(P * 2.0f * pi + 1.7f);

	if(g_Config.m_EcGradientMode == GRADIENT_MODE_SKIN)
	{
		// blend between the tee body and feet colors, so the gradient literally flows from body to feet
		ColorRGBA BodyColor(1.0f, 1.0f, 1.0f, 1.0f);
		ColorRGBA FeetColor(1.0f, 1.0f, 1.0f, 1.0f);
		if(ClientId >= 0 && ClientId < MAX_CLIENTS)
		{
			const auto &Client = GameClient()->m_aClients[ClientId];
			if(Client.m_RenderInfo.m_CustomColoredSkin)
			{
				BodyColor = Client.m_RenderInfo.m_ColorBody;
				FeetColor = Client.m_RenderInfo.m_ColorFeet;
			}
			else
			{
				// same approach as the TClient trails (color mode "tee"): the skin blood color is the
				// average color of the tee body sprite, so it always matches the skin
				const ColorRGBA BloodColor = Client.m_RenderInfo.m_BloodColor;
				const ColorHSLA BloodHsla = color_cast<ColorHSLA>(BloodColor);
				if(BloodHsla.s > 0.05f)
				{
					BodyColor = BloodColor;
					FeetColor = ColorRGBA(BloodColor.r * 0.6f, BloodColor.g * 0.6f, BloodColor.b * 0.6f, 1.0f);
				}
				else
				{
					// fall back to the dominant colors of the skin image for gray/white skins
					SkinColorsFor(ClientId, &BodyColor, &FeetColor);
				}
			}
		}
		const float Blend = 0.5f + 0.5f * Wave;
		ColorRGBA Mixed = ColorRGBA(
			BodyColor.r + (FeetColor.r - BodyColor.r) * Blend,
			BodyColor.g + (FeetColor.g - BodyColor.g) * Blend,
			BodyColor.b + (FeetColor.b - BodyColor.b) * Blend,
			1.0f);
		const float Brightness = 1.0f + 0.10f * Wave2;
		Mixed.r = std::clamp(Mixed.r * Brightness, 0.0f, 1.0f);
		Mixed.g = std::clamp(Mixed.g * Brightness, 0.0f, 1.0f);
		Mixed.b = std::clamp(Mixed.b * Brightness, 0.0f, 1.0f);
		return Mixed;
	}

	const ColorHSLA Base = ColorHSLA(BaseHue(ClientId), 1.0f, 0.5f, 1.0f);
	const float Hue = Base.h + P;
	const float Wrapped = Hue - std::floor(Hue);
	const float Light = 0.5f + 0.06f * Wave2;
	return color_cast<ColorRGBA>(ColorHSLA(Wrapped, 1.0f, Light, 1.0f));
}

ColorRGBA CGradient::SkinColorFor(int ClientId) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	const char *pSkinName = GameClient()->m_aClients[ClientId].m_aSkinName;
	if(pSkinName[0] == '\0')
		return ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);

	const auto It = m_SkinColorCache.find(pSkinName);
	if(It != m_SkinColorCache.end())
		return It->second;

	ColorRGBA Average(1.0f, 1.0f, 1.0f, 1.0f);
	char aPath[IO_MAX_PATH_LENGTH];
	str_format(aPath, sizeof(aPath), "skins/%s.png", pSkinName);
	CImageInfo Info;
	if(Graphics()->LoadPng(Info, aPath, IStorage::TYPE_ALL) && Info.m_pData != nullptr && Info.m_Width > 0 && Info.m_Height > 0)
	{
		double r = 0.0, g = 0.0, b = 0.0, weight = 0.0;
		const unsigned char *pData = (const unsigned char *)Info.m_pData;
		for(int y = 0; y < Info.m_Height; y++)
		{
			for(int x = 0; x < Info.m_Width; x++)
			{
				const int Index = (y * Info.m_Width + x) * 4;
				const double Alpha = pData[Index + 3] / 255.0;
				if(Alpha < 0.5)
					continue;
				r += pData[Index + 0] * Alpha;
				g += pData[Index + 1] * Alpha;
				b += pData[Index + 2] * Alpha;
				weight += Alpha;
			}
		}
		if(weight > 0.0)
			Average = ColorRGBA((float)(r / weight / 255.0), (float)(g / weight / 255.0), (float)(b / weight / 255.0), 1.0f);
		Info.Free();
	}

	m_SkinColorCache[pSkinName] = Average;
	return Average;
}

void CGradient::OnConsoleInit()
{
	Console()->Register("gradient_debug", "", CFGFLAG_CLIENT, ConGradientDebug, this, "Print gradient debug information for all clients");
}

void CGradient::ConGradientDebug(IConsole::IResult *pResult, void *pUserData)
{
	CGradient *pThis = (CGradient *)pUserData;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "enabled=%d names=%d skin=%d mode=%d apply=%d speed=%d", g_Config.m_EcGradientEnabled, g_Config.m_EcGradientNames, g_Config.m_EcGradientSkin, g_Config.m_EcGradientMode, g_Config.m_EcGradientApply, g_Config.m_EcGradientSpeed);
	pThis->GameClient()->Echo(aBuf);
	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		const auto &Client = pThis->GameClient()->m_aClients[ClientId];
		if(Client.m_aName[0] == '\0')
			continue;
		const ColorRGBA Skin = pThis->SkinColorFor(ClientId);
		const ColorRGBA Result = pThis->ColorFor(ClientId);
		const ColorRGBA RenderBody = Client.m_RenderInfo.m_ColorBody;
		str_format(aBuf, sizeof(aBuf), "id=%d name='%s' skin='%s' custom=%d body=%.2f,%.2f,%.2f sampled=%.2f,%.2f,%.2f out=%.2f,%.2f,%.2f applies=%d", ClientId, Client.m_aName, Client.m_aSkinName, Client.m_RenderInfo.m_CustomColoredSkin, RenderBody.r, RenderBody.g, RenderBody.b, Skin.r, Skin.g, Skin.b, Result.r, Result.g, Result.b, pThis->AppliesTo(ClientId) ? 1 : 0);
		pThis->GameClient()->Echo(aBuf);
	}
}

void CGradient::SkinColorsFor(int ClientId, ColorRGBA *pFirst, ColorRGBA *pSecond) const
{
	*pFirst = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	*pSecond = ColorRGBA(0.65f, 0.65f, 0.65f, 1.0f);
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;
	const char *pSkinName = GameClient()->m_aClients[ClientId].m_aSkinName;
	if(pSkinName[0] == '\0')
		return;

	const auto It = m_SkinPaletteCache.find(pSkinName);
	if(It != m_SkinPaletteCache.end())
	{
		*pFirst = It->second.first;
		*pSecond = It->second.second;
		return;
	}

	char aPath[IO_MAX_PATH_LENGTH];
	str_format(aPath, sizeof(aPath), "skins/%s.png", pSkinName);
	CImageInfo Info;
	if(!Graphics()->LoadPng(Info, aPath, IStorage::TYPE_ALL) || Info.m_pData == nullptr || Info.m_Width <= 0 || Info.m_Height <= 0)
	{
		m_SkinPaletteCache[pSkinName] = std::make_pair(*pFirst, *pSecond);
		return;
	}

	// collect opaque pixels
	std::vector<ColorRGBA> vPixels;
	vPixels.reserve((size_t)Info.m_Width * Info.m_Height / 4);
	const unsigned char *pData = (const unsigned char *)Info.m_pData;
	for(int y = 0; y < Info.m_Height; y++)
	{
		for(int x = 0; x < Info.m_Width; x++)
		{
			const int Index = (y * Info.m_Width + x) * 4;
			if(pData[Index + 3] < 128)
				continue;
			vPixels.emplace_back(pData[Index + 0] / 255.0f, pData[Index + 1] / 255.0f, pData[Index + 2] / 255.0f, 1.0f);
		}
	}
	Info.Free();

	if(vPixels.empty())
	{
		m_SkinPaletteCache[pSkinName] = std::make_pair(*pFirst, *pSecond);
		return;
	}

	// simple 2-means clustering over the skin colors
	ColorRGBA CenterA = vPixels[0];
	ColorRGBA CenterB = CenterA;
	float BestDistance = -1.0f;
	for(const ColorRGBA &Pixel : vPixels)
	{
		const float D = distance(vec2(Pixel.r, Pixel.g), vec2(CenterA.r, CenterA.g));
		if(D > BestDistance)
		{
			BestDistance = D;
			CenterB = Pixel;
		}
	}

	for(int Iteration = 0; Iteration < 6; Iteration++)
	{
		double aR = 0, aG = 0, aB = 0, aCount = 0;
		double bR = 0, bG = 0, bB = 0, bCount = 0;
		for(const ColorRGBA &Pixel : vPixels)
		{
			const float DA = distance(vec2(Pixel.r, Pixel.g), vec2(CenterA.r, CenterA.g));
			const float DB = distance(vec2(Pixel.r, Pixel.g), vec2(CenterB.r, CenterB.g));
			if(DA <= DB)
			{
				aR += Pixel.r; aG += Pixel.g; aB += Pixel.b; aCount++; 
			}
			else
			{
				bR += Pixel.r; bG += Pixel.g; bB += Pixel.b; bCount++; 
			}
		}
		if(aCount > 0 && bCount > 0)
		{
			CenterA = ColorRGBA((float)(aR / aCount), (float)(aG / aCount), (float)(aB / aCount), 1.0f);
			CenterB = ColorRGBA((float)(bR / bCount), (float)(bG / bCount), (float)(bB / bCount), 1.0f);
		}
	}

	*pFirst = CenterA;
	*pSecond = CenterB;
	m_SkinPaletteCache[pSkinName] = std::make_pair(CenterA, CenterB);
}

