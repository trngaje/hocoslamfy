/*
 * Hocoslamfy, OpenDingux platform-specific code file
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

#include <stdbool.h>
#include <stdint.h>

#include "SDL.h"

#include "platform.h"

#if defined(MIYOOMINI) || defined(SDL2)
static Uint32 LastTicks = 0;
#endif
void InitializePlatform(void)
{
#if defined(MIYOOMINI) || defined(SDL2)
	LastTicks = SDL_GetTicks();
#endif
}

Uint32 ToNextFrame(void)
{
#if defined(MIYOOMINI) || defined(SDL2)
	SDL_Delay(8);
	Uint32 Ticks = SDL_GetTicks();
	Uint32 Duration = Ticks - LastTicks;
	LastTicks = Ticks;
	return Duration;
#else
	// OpenDingux waits for vertical sync by itself.
	return 16;
#endif
}

/*
for miyoomini
X shift
y alt
A space
B ctrl
select rctrl
start enter
L2 tab
R2 backspace
L e
R t
*/

bool IsEnterGamePressingEvent(const SDL_Event* event)
{
	return event->type == SDL_KEYDOWN
#if defined(MIYOOMINI) || defined(SDL2)
	    && (event->key.keysym.sym == SDLK_SPACE  /* A */
#else
	    && (event->key.keysym.sym == SDLK_LCTRL  /* A */
#endif
	     || event->key.keysym.sym == SDLK_RETURN /* Start */);
}

bool IsEnterGameReleasingEvent(const SDL_Event* event)
{
	return event->type == SDL_KEYUP
#if defined(MIYOOMINI) || defined(SDL2)
	    && (event->key.keysym.sym == SDLK_SPACE  /* A */
#else
	    && (event->key.keysym.sym == SDLK_LCTRL  /* A */
#endif
	
	     || event->key.keysym.sym == SDLK_RETURN /* Start */);
}

const char* GetEnterGamePrompt(void)
{
	return "A/Start";
}

bool IsExitGameEvent(const SDL_Event* event)
{
	return event->type == SDL_QUIT
	    || (event->type == SDL_KEYDOWN
#if defined(MIYOOMINI) || defined(SDL2)
	     && (event->key.keysym.sym == SDLK_LCTRL   /* B */
#else
	     && (event->key.keysym.sym == SDLK_LALT   /* B */
#endif
	      || event->key.keysym.sym == SDLK_ESCAPE /* Select */));
}

const char* GetExitGamePrompt(void)
{
#if defined(MIYOOMINI) || defined(SDL2)
	return "B/Function";
#else
	return "B/Select";
#endif
}

bool IsBoostEvent(const SDL_Event* event)
{
	return event->type == SDL_KEYDOWN
	    && (event->key.keysym.sym == SDLK_LCTRL  /* A */
	     || event->key.keysym.sym == SDLK_LALT   /* B */
	     || event->key.keysym.sym == SDLK_LSHIFT /* GCW Zero: X; Dingoo A320: Y */
	     || event->key.keysym.sym == SDLK_SPACE  /* GCW Zero: Y; Dingoo A320: X */);
}

const char* GetBoostPrompt(void)
{
	return "A/B/X/Y";
}

bool IsPauseEvent(const SDL_Event* event)
{
	return event->type == SDL_KEYDOWN
	    && event->key.keysym.sym == SDLK_RETURN /* Start */;
}

bool IsRumbleEvent(const SDL_Event* event)
{
	return event->type == SDL_KEYDOWN
#if defined(MIYOOMINI) || defined(SDL2)
	    && event->key.keysym.sym == SDLK_e;
#else
	    && event->key.keysym.sym == SDLK_TAB;
#endif
}

const char* GetRumblePrompt(void)
{
	return "L1";
}

bool IsScoreToggleEvent(const SDL_Event* event)
{
	return event->type == SDL_KEYDOWN
#if defined(MIYOOMINI) || defined(SDL2)
	    && event->key.keysym.sym == SDLK_t;
#else
	    && event->key.keysym.sym == SDLK_BACKSPACE;
#endif
}

const char* GetScoreTogglePrompt(void)
{
	return "R1";
}

const char* GetPausePrompt(void)
{
	return "Start";
}
