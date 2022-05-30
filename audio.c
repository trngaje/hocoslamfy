/*
 * Hocoslamfy, sound playback code file
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

#include <stdlib.h>
#include <stdbool.h>

#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif

#include "init.h"

static bool       SND_Available = false;

static Mix_Music* BGM           = NULL;

static Mix_Chunk* SFX_Fly       = NULL;
static Mix_Chunk* SFX_Pass      = NULL;
static Mix_Chunk* SFX_Collision = NULL;
static Mix_Chunk* SFX_HighScore = NULL;

static Mix_Chunk* LoadSFX(const char* Path)
{
	Mix_Chunk* Result = Mix_LoadWAV(Path);
	if (Result == NULL)
		printf("%s: Mix_LoadWAV failed: %s\n", Path, Mix_GetError());
	else
		printf("Successfully loaded %s\n", Path);
	return Result;
}

bool InitializeAudio()
{
	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2 /* stereo */, 1024 /* buffer size */))
	{
		printf("warning: Mix_OpenAudio failed: %s\n", Mix_GetError());
		printf("Sound will not be available.\n");
	}
	else
	{
		printf("Mix_OpenAudio succeeded\n");
		SND_Available = true;
	}

	if (SND_Available)
	{
		BGM = Mix_LoadMUS(DATA_PATH "bgm.wav");
		if (BGM == NULL)
		{
			printf("%s: Mix_LoadMUS failed: %s\n", DATA_PATH "bgm.wav", Mix_GetError());
			return false;
		}
		else
			printf("Successfully loaded %s\n", DATA_PATH "bgm.wav");

		SFX_Fly       = LoadSFX(DATA_PATH "fly.wav");
		SFX_Pass      = LoadSFX(DATA_PATH "pass.wav");
		SFX_Collision = LoadSFX(DATA_PATH "collision.wav");
		SFX_HighScore = LoadSFX(DATA_PATH "highscore.wav");
	}

	return true;
}

void FinalizeAudio()
{
	if (SND_Available)
	{
		Mix_HaltMusic();
		Mix_FreeMusic(BGM);
		BGM = NULL;
		Mix_FreeChunk(SFX_Fly);
		Mix_FreeChunk(SFX_Pass);
		Mix_FreeChunk(SFX_Collision);
		Mix_FreeChunk(SFX_HighScore);
		Mix_CloseAudio();
	}
}

void StartBGM()
{
	if (SND_Available)
	{
		Mix_PlayMusic(BGM, -1 /* loop indefinitely */);
	}
}

void StopBGM()
{
	if (SND_Available)
	{
		Mix_HaltMusic();
	}
}

// In all of the below functions, -1 as parameter #1 means "don't care which
// SDL_mixer channel gets used to play this sound effect", and 0 as parameter
// #3 means "when done, loop 0 times".
void PlaySFXFly()
{
	if (SND_Available)
	{
		Mix_PlayChannel(-1, SFX_Fly, 0);
	}
}

void PlaySFXPass()
{
	if (SND_Available)
	{
		Mix_PlayChannel(-1, SFX_Pass, 0);
	}
}

void PlaySFXCollision()
{
	if (SND_Available)
	{
		Mix_PlayChannel(-1, SFX_Collision, 0);
	}
}

void PlaySFXHighScore()
{
	if (SND_Available)
	{
		Mix_PlayChannel(-1, SFX_HighScore, 0);
	}
}
