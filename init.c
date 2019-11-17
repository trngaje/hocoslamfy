/*
 * Hocoslamfy, initialisation code file
 * Copyright (C) 2014 Nebuleon Fumika <nebuleon@gcw-zero.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "init.h"

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_error.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mouse.h>
#include <SDL/SDL_video.h>

#include "audio.h"
#include "bg.h"
#include "main.h"
#include "game.h"
#include "platform.h"
#include "title.h"
#ifndef NO_SHAKE
#include <shake.h>

Shake_Device *device;
Shake_Effect flap_effect, flap_effect1, crash_effect;
int flap_effect_id, flap_effect_id1, crash_effect_id;
#endif

static const char* BackgroundImageNames[BG_LAYER_COUNT] = {
	"Sky.png",
	"Mountains.png",
	"Clouds3.png",
	"Clouds2.png",
	"Clouds1.png",
	"Grass3.png",
	"Grass2.png",
	"Grass1.png"
};

static const char* TitleScreenFrameNames[TITLE_FRAME_COUNT] = {
	"TitleHeader1.png",
	"TitleHeader2.png",
	"TitleHeader3.png",
	"TitleHeader4.png",
	"TitleHeader5.png",
	"TitleHeader6.png",
	"TitleHeader7.png",
	"TitleHeader8.png"
};

static SDL_Surface* LoadImage(const char* Path)
{
	char path[256];
	snprintf(path, 256, DATA_PATH "%s", Path);
	return IMG_Load(path);
}

static bool CheckImage(bool* Continue, bool* Error, const SDL_Surface* Image, const char* Name)
{
	if (Image == NULL)
	{
		*Continue = false;  *Error = true;
		printf("%s: LoadImage failed: %s\n", Name, IMG_GetError());
		return false;
	}
	else
	{
		printf("Successfully loaded %s\n", Name);
		return true;
	}
}

static SDL_Surface* ConvertSurface(bool* Continue, bool* Error, SDL_Surface* Source, const char* Name)
{
	SDL_Surface* Dest;
	if (Source->format->Amask != 0)
		Dest = SDL_DisplayFormatAlpha(Source);
	else
		Dest = SDL_DisplayFormat(Source);
	if (Dest == NULL)
	{
		*Continue = false;  *Error = true;
		printf("%s: SDL_ConvertSurface failed: %s\n", Name, SDL_GetError());
		SDL_ClearError();
		return NULL;
	}
	else
	{
		printf("Successfully converted %s to the screen's pixel format\n", Name);
		SDL_FreeSurface(Source);
		return Dest;
	}
}

void Initialize(bool* Continue, bool* Error)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		*Continue = false;  *Error = true;
		printf("SDL initialisation failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	} else printf("SDL initialisation succeeded\n");

	SDL_Surface* WindowIcon = LoadImage("hocoslamfy.png");
	if (!CheckImage(Continue, Error, WindowIcon, "hocoslamfy.png"))
		return;
	SDL_WM_SetIcon(WindowIcon, NULL);
	SDL_WM_SetCaption("hocoslamfy", "hocoslamfy");
#ifdef USE_16BPP
	Screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE |
#else
	Screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_HWSURFACE |
#endif
#ifdef SDL_TRIPLEBUF
		SDL_TRIPLEBUF
#else
		SDL_DOUBLEBUF
#endif
		);

	if (Screen == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_SetVideoMode succeeded\n");

	SDL_ShowCursor(0);

	uint32_t i;
	for (i = 0; i < BG_LAYER_COUNT; i++)
	{
		BackgroundImages[i] = LoadImage(BackgroundImageNames[i]);
		if (!CheckImage(Continue, Error, BackgroundImages[i], BackgroundImageNames[i]))
			return;
		if ((BackgroundImages[i] = ConvertSurface(Continue, Error, BackgroundImages[i], BackgroundImageNames[i])) == NULL)
			return;
	}

	for (i = 0; i < TITLE_FRAME_COUNT; i++)
	{
		TitleScreenFrames[i] = LoadImage(TitleScreenFrameNames[i]);
		if (!CheckImage(Continue, Error, TitleScreenFrames[i], TitleScreenFrameNames[i]))
			return;
		if ((TitleScreenFrames[i] = ConvertSurface(Continue, Error, TitleScreenFrames[i], TitleScreenFrameNames[i])) == NULL)
			return;
	}

	CharacterFrames = LoadImage("Bee.png");
	if (!CheckImage(Continue, Error, CharacterFrames, "Bee.png"))
		return;
	if ((CharacterFrames = ConvertSurface(Continue, Error, CharacterFrames, "Bee.png")) == NULL)
		return;
	CollisionImage = LoadImage("Crash.png");
	if (!CheckImage(Continue, Error, CollisionImage, "Crash.png"))
		return;
	if ((CollisionImage = ConvertSurface(Continue, Error, CollisionImage, "Crash.png")) == NULL)
		return;
	ColumnImage = LoadImage("Bamboo.png");
	if (!CheckImage(Continue, Error, ColumnImage, "Bamboo.png"))
		return;
	if ((ColumnImage = ConvertSurface(Continue, Error, ColumnImage, "Bamboo.png")) == NULL)
		return;
	GameOverFrame = LoadImage("GameOverHeader.png");
	if (!CheckImage(Continue, Error, GameOverFrame, "GameOverHeader.png"))
		return;
	if ((GameOverFrame = ConvertSurface(Continue, Error, GameOverFrame, "GameOverHeader.png")) == NULL)
		return;

	InitializePlatform();

#ifndef NO_SHAKE
	// Title screen. (-> title.c)
	Rumble = true;
	Shake_Init();
        device = Shake_Open(0);

        Shake_InitEffect(&flap_effect, SHAKE_EFFECT_RUMBLE);
        flap_effect.u.rumble.strongMagnitude = SHAKE_RUMBLE_STRONG_MAGNITUDE_MAX;
        flap_effect.u.rumble.weakMagnitude = SHAKE_RUMBLE_STRONG_MAGNITUDE_MAX*0.9;
        flap_effect.length = 380;
        flap_effect.delay = 0;

        Shake_InitEffect(&flap_effect1, SHAKE_EFFECT_RUMBLE);
        flap_effect1.u.rumble.strongMagnitude = SHAKE_RUMBLE_STRONG_MAGNITUDE_MAX;
        flap_effect1.u.rumble.weakMagnitude = SHAKE_RUMBLE_STRONG_MAGNITUDE_MAX*0.9;
        flap_effect1.length = 380;
        flap_effect1.delay = 0;

        Shake_InitEffect(&crash_effect, SHAKE_EFFECT_RUMBLE);
        crash_effect.u.rumble.strongMagnitude = SHAKE_RUMBLE_STRONG_MAGNITUDE_MAX;
        crash_effect.u.rumble.weakMagnitude = SHAKE_RUMBLE_STRONG_MAGNITUDE_MAX;
        crash_effect.length = 1000;
        crash_effect.delay = 0;

        flap_effect_id = Shake_UploadEffect(device, &flap_effect);
        flap_effect_id1 = Shake_UploadEffect(device, &flap_effect1);
        crash_effect_id = Shake_UploadEffect(device, &crash_effect);
#endif

	FollowBee = false;

	if (!InitializeAudio())
	{
		*Continue = false;  *Error = true;
		return;
	}
	else
		StartBGM();


	ToTitleScreen();
}

void Finalize()
{
	uint32_t i;
	StopBGM();
	FinalizeAudio();
	for (i = 0; i < BG_LAYER_COUNT; i++)
	{
		SDL_FreeSurface(BackgroundImages[i]);
		BackgroundImages[i] = NULL;
	}
	for (i = 0; i < TITLE_FRAME_COUNT; i++)
	{
		SDL_FreeSurface(TitleScreenFrames[i]);
		TitleScreenFrames[i] = NULL;
	}
	SDL_FreeSurface(CharacterFrames);
	CharacterFrames = NULL;
	SDL_FreeSurface(ColumnImage);
	ColumnImage = NULL;
	SDL_FreeSurface(GameOverFrame);
	GameOverFrame = NULL;

#ifndef NO_SHAKE
	Shake_Stop(device, flap_effect_id);
	Shake_Stop(device, flap_effect_id1);
        Shake_Stop(device, crash_effect_id);

        Shake_EraseEffect(device, flap_effect_id);
        Shake_EraseEffect(device, flap_effect_id1);
        Shake_EraseEffect(device, crash_effect_id);
        Shake_Close(device);
        Shake_Quit();
#endif
	SDL_Quit();
}
