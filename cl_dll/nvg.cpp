#include "stdio.h"
#include "stdlib.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <time.h>

DECLARE_MESSAGE(m_NVG, NVGActivate)
DECLARE_MESSAGE(m_NVG, NVG)

bool CHudNVG::Init()
{
	HOOK_MESSAGE(NVGActivate);
	HOOK_MESSAGE(NVG);
	m_iUnk0x30 = 0;
	m_iUnk0x34 = 0;
	m_flUnk0x38 = 0.0f;
	m_iFlags = 0;
	gHUD.AddHudElem(this);
	srand((unsigned)time(NULL));
	return true;
}

bool CHudNVG::VidInit()
{
	m_hsprNVG = LoadSprite("sprites/nvg.spr");

	m_hsprBatteryFull = gHUD.GetSprite(gHUD.GetSpriteIndex("nvg_battery_full"));
	m_hsprBatteryEmpty = gHUD.GetSprite(gHUD.GetSpriteIndex("nvg_battery_empty"));

	m_iUnk0x20 = &gHUD.GetSpriteRect(gHUD.GetSpriteIndex("nvg_battery_full"));
	m_iUnk0x24 = &gHUD.GetSpriteRect(gHUD.GetSpriteIndex("nvg_battery_empty"));
	m_iUnk0x2c = m_iUnk0x20->right - m_iUnk0x20->left;

	int flashEmpty = gHUD.GetSpriteIndex("flash_empty");
	Rect flashEmptyRect = gHUD.GetSpriteRect(flashEmpty);

	m_iUnk0x28 = (flashEmptyRect.bottom - flashEmptyRect.top) * 2;
	return true;
}

bool CHudNVG::Draw(float flTime)
{
	int iHeight;
	int iWidth;
	int v14;
	int a;
	int v17;
	int b;
	int g;
	int r;

	if (m_iUnk0x30 != 0)
	{
		if (m_iUnk0x34 == 0)
		{
			m_flUnk0x38 = gHUD.m_flTimeDelta * 7.5f + m_flUnk0x38;
			if (100.0f < m_flUnk0x38)
			{
				m_flUnk0x38 = 100.0f;
			}
			a = 100;
		}
		else
		{
			m_flUnk0x38 = m_flUnk0x38 - gHUD.m_flTimeDelta * 0.75f;
			if (m_flUnk0x38 < 0.0f)
			{
				m_flUnk0x38 = 0.0f;
			}
			a = 255;
		}

		if (20.0f <= m_flUnk0x38)
		{
			UnpackRGB(r, g, b, 0xFF1010);
		}
		else
		{
			UnpackRGB(r, g, b, 0x437DFF);
		}

		ScaleColors(r, g, b, a);

		v17 = (gHUD.m_scrinfo.iWidth - m_iUnk0x2c) - m_iUnk0x2c / 2;

		v14 = m_iUnk0x2c * m_flUnk0x38 / 100.0f;

		Rect test;

		if (v14 < m_iUnk0x2c)
		{
			SPR_Set(m_hsprBatteryEmpty, r, g, b);
			test.left = m_iUnk0x24->left;
			test.top = m_iUnk0x24->top;
			test.bottom = m_iUnk0x24->bottom;
			test.right = m_iUnk0x24->right - v14;
			SPR_DrawAdditive(0, v17, m_iUnk0x28, &test);
		}
		if (0 < v14)
		{
			SPR_Set(m_hsprBatteryFull, r, g, b);
			test.right = m_iUnk0x20->right;
			test.top = m_iUnk0x20->top;
			test.bottom = m_iUnk0x20->bottom;
			test.right = m_iUnk0x20->right + (m_iUnk0x2c - v14);
			SPR_DrawAdditive(0, (v17 + m_iUnk0x2c) - v14, m_iUnk0x28, &test);
		}
	}

	if (m_iUnk0x34 != 0)
	{
		FillRGBA(0, 0, gHUD.m_scrinfo.iWidth, gHUD.m_scrinfo.iHeight, 50, 0, 0, 255);
		SPR_Set(m_hsprNVG, 10, 100, 10);
		iWidth = SPR_Width(m_hsprNVG, 0);
		iHeight = SPR_Height(m_hsprNVG, 0);

		int i, j;

		for (i = -(rand() % iHeight); i < gHUD.m_scrinfo.iHeight; i += iHeight)
		{
			for (j = -(rand() % iWidth); j < gHUD.m_scrinfo.iHeight; j += iWidth)
			{
				SPR_DrawHoles((int)(flTime * 15.0f) % SPR_Frames(m_hsprNVG), j, i, NULL);
			}
		}
	}
	return true;
}

bool CHudNVG::MsgFunc_NVGActivate(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iUnk0x34 = READ_BYTE();
	m_flUnk0x38 = READ_BYTE();
	return true;
}

bool CHudNVG::MsgFunc_NVG(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iUnk0x30 = READ_BYTE();
	m_flUnk0x38 = READ_BYTE();

	if (m_iUnk0x30 == 0)
	{
		m_iFlags &= ~HUD_ACTIVE;
	}
	else
	{
		m_iFlags |= HUD_ACTIVE;
	}

	m_iUnk0x34 = 0;
	return true;
}
