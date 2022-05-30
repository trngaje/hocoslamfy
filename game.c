/*
 * Hocoslamfy, game code file
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
#include <stdlib.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/stat.h>

#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#endif

#include "main.h"
#include "init.h"
#include "platform.h"
#include "game.h"
#include "score.h"
#include "bg.h"
#include "text.h"
#include "audio.h"

#ifndef NO_SHAKE
#include <shake.h>
#endif

#ifdef MIYOOMINI
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#endif

#ifdef SDL2
extern SDL_Window* sdlWindow;
extern SDL_Surface* sdlSurface;
#endif

static uint32_t               Score;

static bool                   Boost;
static bool                   Pause;
static enum PlayerStatus      PlayerStatus;

// Where the player is. (Center, meters.)
static float                  PlayerX;
static float                  PlayerY;
// Where the player is going. (Meters per second.)
static float                  PlayerSpeed;

// -- Animation control variables --

// Animation frame for the player's character.
static uint8_t                PlayerFrame;
// Time the player's character has had the current animation frame.
// (In milliseconds.)
static uint32_t               PlayerFrameTime;
// Whether the player's character is currently blinking.
static bool                   PlayerBlinking;
// Time the player's character has been blinking, if Blinking is true.
// Time the player's character has left before blinking, if Blinking is false.
static uint32_t               PlayerBlinkTime;

// Passed to the score screen after the player is done dying.
static enum GameOverReason    GameOverReason;

// What the player avoids.
static struct HocoslamfyRect* Rectangles     = NULL;
static uint32_t               RectangleCount = 0;

static float                  GenDistance;

#ifdef MIYOOMINI
void rumble_off_handler (int signum)
{
     system("echo 1 > /sys/class/gpio/gpio48/value;echo 48 > /sys/class/gpio/unexport");     
     fprintf(stderr, "rumble_off_handler \n");
}

void rumble_on(int ms)
{
	struct sigaction sa;
	struct itimerval timer;

	/* Install timer_handler as the signal handler for SIGVTALRM. */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &rumble_off_handler;
	sigaction (SIGVTALRM, &sa, NULL);

	/* Configure the timer to expire after 250 msec... */
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = ms * 1000;
	//timer.it_value.tv_usec = 2;

	/* ... and every 250 msec after that. */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
			
	/* Start a virtual timer. It counts down whenever this process is executing. */
	setitimer (ITIMER_VIRTUAL, &timer , NULL);	
	
   system("echo 48 > /sys/class/gpio/export;echo out > /sys/class/gpio/gpio48/direction;echo 0 > /sys/class/gpio/gpio48/value");


}
#endif

void GameGatherInput(bool* Continue)
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev))
	{
		if (IsBoostEvent(&ev) && !Pause)
			Boost = true;
		else if (IsPauseEvent(&ev) && PlayerStatus == ALIVE)
			Pause = !Pause;
		else if (IsRumbleEvent(&ev))
			Rumble = !Rumble;
		else if (IsScoreToggleEvent(&ev))
			FollowBee = !FollowBee;
		else if (IsExitGameEvent(&ev))
		{
			*Continue = false;
			return;
		}
	}
}

static void SetStatus(const enum PlayerStatus NewStatus)
{
	PlayerFrameTime = 0;
	if (NewStatus == COLLIDED && PlayerStatus != COLLIDED)
	{
#ifndef NO_SHAKE		
		Shake_Status ss;
    if (Rumble) {
		  ss = Shake_Play(device, crash_effect_id);
    }
#endif

#ifdef MIYOOMINI
	if (Rumble) {
		rumble_on(100);
	}
#endif
		PlaySFXCollision();
	}
	PlayerStatus = NewStatus;
	if (NewStatus == DYING) 
	{
		PlayerSpeed = 0.0f;
	}
}

static void AnimationControl(Uint32 Milliseconds)
{
	Uint32 Remainder = Milliseconds;
	switch (PlayerStatus)
	{
		case ALIVE:
			Remainder = Remainder % (ANIMATION_TIME * ANIMATION_FRAMES);
			PlayerFrame = (PlayerFrame + (PlayerFrameTime + Remainder) / ANIMATION_TIME) % ANIMATION_FRAMES;
			PlayerFrameTime = (PlayerFrameTime + Remainder) % ANIMATION_TIME;
			break;
		case DYING:
			// Get rid of all the times the animation could have been fully
			// completed since the last frame displayed.
			Remainder = Remainder % (ANIMATION_TIME * ANIMATION_FRAMES);
			// If needed, advance the frame by however many steps are now
			// fully done.
			PlayerFrame = (PlayerFrame + (PlayerFrameTime + Remainder) / ANIMATION_TIME) % ANIMATION_FRAMES;
			// Then add milliseconds for the current frame.
			PlayerFrameTime = (PlayerFrameTime + Remainder) % ANIMATION_TIME;
			break;

		case COLLIDED:
			PlayerFrameTime += Remainder;
			if (PlayerFrameTime > COLLISION_TIME)
				SetStatus(DYING);
			break;
	}

	// Make the player's character blink for some time every so often.
	Remainder = Milliseconds;
	while (Remainder > 0)
	{
		if (PlayerBlinking)
		{
			if (PlayerBlinkTime + Remainder >= BLINK_TIME)
			{
				Remainder -= BLINK_TIME - PlayerBlinkTime;
				PlayerBlinking = false;
				PlayerBlinkTime = NONBLINK_TIME_MIN + rand() % (NONBLINK_TIME_MAX - NONBLINK_TIME_MIN);
			}
			else
			{
				PlayerBlinkTime += Remainder;
				Remainder = 0;
			}
		}
		else
		{
			if (Remainder >= PlayerBlinkTime)
			{
				Remainder -= PlayerBlinkTime;
				PlayerBlinking = true;
				PlayerBlinkTime = 0;
			}
			else
			{
				PlayerBlinkTime -= Remainder;
				Remainder = 0;
			}
		}
	}
}

void GameDoLogic(bool* Continue, bool* Error, Uint32 Milliseconds)
{
	if (!Pause && PlayerStatus == ALIVE)
	{
		bool PointAwarded = false;
		uint32_t Millisecond;
		for (Millisecond = 0; Millisecond < Milliseconds; Millisecond++)
		{
			// Scroll all rectangles to the left.
			int32_t i;
			for (i = RectangleCount - 1; i >= 0; i--)
			{
				Rectangles[i].Left += FIELD_SCROLL / 1000;
				Rectangles[i].Right += FIELD_SCROLL / 1000;
				// If a rectangle is past the player, award the player with a
				// point. But there is a pair of them per column!
				if (!Rectangles[i].Passed
				 && Rectangles[i].Right < PlayerX)
				{
					Rectangles[i].Passed = true;
					if (!PointAwarded)
					{
						Score++;
						PointAwarded = true;
						PlaySFXPass();
					}
				}
				// If a rectangle is past the left side, remove it.
				if (Rectangles[i].Right < 0.0f)
				{
					memmove(&Rectangles[i], &Rectangles[i + 1], (RectangleCount - i) * sizeof(struct HocoslamfyRect));
					RectangleCount--;
				}
			}
			// Generate a pair of rectangles now if needed.
			if (RectangleCount == 0 || FIELD_WIDTH - Rectangles[RectangleCount - 1].Right >= GenDistance)
			{
				float Left;
				if (RectangleCount == 0)
					Left = FIELD_WIDTH + FIELD_SCROLL / 1000;
				else
				{
					Left = Rectangles[RectangleCount - 1].Right + GenDistance;
					GenDistance += RECT_GEN_SPEED;
					if (GenDistance < RECT_GEN_MIN)
						GenDistance = RECT_GEN_MIN;
				}
				Rectangles = realloc(Rectangles, (RectangleCount + 2) * sizeof(struct HocoslamfyRect));
				RectangleCount += 2;
				Rectangles[RectangleCount - 2].Passed = Rectangles[RectangleCount - 1].Passed = false;
				Rectangles[RectangleCount - 2].Left = Rectangles[RectangleCount - 1].Left = Left;
				Rectangles[RectangleCount - 2].Right = Rectangles[RectangleCount - 1].Right = Left + RECT_WIDTH;
				// Where's the place for the player to go through?
				float GapTop = GAP_HEIGHT + (FIELD_HEIGHT / 16.0f) + ((float) rand() / (float) RAND_MAX) * (FIELD_HEIGHT - GAP_HEIGHT - (FIELD_HEIGHT / 8.0f));
				Rectangles[RectangleCount - 2].Top = FIELD_HEIGHT;
				Rectangles[RectangleCount - 2].Bottom = GapTop;
				Rectangles[RectangleCount - 1].Top = GapTop - GAP_HEIGHT;
				Rectangles[RectangleCount - 1].Bottom = 0.0f;
				Rectangles[RectangleCount - 2].Frame = rand() % 3;
				Rectangles[RectangleCount - 1].Frame = rand() % 3;
			}
			// Update the speed at which the player is going.
			PlayerSpeed += GRAVITY / 1000;
			if (Boost)
			{
				// The player expects to rise a constant amount with each press of
				// the triggering key or button, so set his or her speed to
				// boost him or her from zero, even if the speed was positive.
				// For a more physically-realistic version of thrust, use
				// [PlayerSpeed += SPEED_BOOST;].
				PlayerSpeed = SPEED_BOOST;
				Boost = false;
#ifndef NO_SHAKE
				Shake_Stop(device, flap_effect_id);
				Shake_Stop(device, flap_effect_id1);
        if (Rumble) {
				  Shake_Play(device, flap_effect_id);
				  Shake_Play(device, flap_effect_id1);
        }
#endif

#ifdef MIYOOMINI
        if (Rumble) {
			rumble_on(38);
        }
#endif
				PlaySFXFly();
			}
			// Update the player's position.
			// If the player's position has collided with the borders of the field,
			// the player's game is over.
			PlayerY += PlayerSpeed / 1000;
			if (PlayerY + (COLLISION_B_HEIGHT / 2) > FIELD_HEIGHT || PlayerY - (COLLISION_B_HEIGHT / 2) < 0.0f)
			{
				SetStatus(COLLIDED);
				GameOverReason = FIELD_BORDER_COLLISION;
				break;
			}

			// Collision detection.
			for (i = 0; i < RectangleCount; i++)
			{
				if ((((PlayerY + (COLLISION_A_HEIGHT / 2) > Rectangles[i].Bottom
				    && PlayerY + (COLLISION_A_HEIGHT / 2) < Rectangles[i].Top)
				   || (PlayerY - (COLLISION_A_HEIGHT / 2) > Rectangles[i].Bottom
				    && PlayerY - (COLLISION_A_HEIGHT / 2) < Rectangles[i].Top))
				  && ((PlayerX - (COLLISION_A_WIDTH  / 2) > Rectangles[i].Left
				    && PlayerX - (COLLISION_A_WIDTH  / 2) < Rectangles[i].Right)
				   || (PlayerX + (COLLISION_A_WIDTH  / 2) > Rectangles[i].Left
				    && PlayerX + (COLLISION_A_WIDTH  / 2) < Rectangles[i].Right)))
				 || (((PlayerY + (COLLISION_B_HEIGHT / 2) > Rectangles[i].Bottom
				    && PlayerY + (COLLISION_B_HEIGHT / 2) < Rectangles[i].Top)
				   || (PlayerY - (COLLISION_B_HEIGHT / 2) > Rectangles[i].Bottom
				    && PlayerY - (COLLISION_B_HEIGHT / 2) < Rectangles[i].Top))
				  && ((PlayerX - (COLLISION_B_WIDTH  / 2) > Rectangles[i].Left
				    && PlayerX - (COLLISION_B_WIDTH  / 2) < Rectangles[i].Right)
				   || (PlayerX + (COLLISION_B_WIDTH  / 2) > Rectangles[i].Left
				    && PlayerX + (COLLISION_B_WIDTH  / 2) < Rectangles[i].Right))))
				{
					SetStatus(COLLIDED);
					GameOverReason = RECTANGLE_COLLISION;
					break;
				}
			}
		}

		AdvanceBackground(Milliseconds);
	}
	else if (PlayerStatus == DYING)
	{
		uint32_t Millisecond;
		for (Millisecond = 0; Millisecond < Milliseconds; Millisecond++)
		{
			// Update the speed at which the player is going.
			PlayerSpeed += GRAVITY / 1000;
			// Update the player's position.
			// If the player's position has reached the bottom of the screen,
			// send him or her to the score screen.
			PlayerY += PlayerSpeed / 1000;
			if (PlayerY < 0.0f)
			{
				uint32_t HighScore = GetHighScore();
				
				ToScore(Score, GameOverReason, HighScore);
				
				if (Score > HighScore)
					SaveHighScore(Score);
				return;
			}
		}
	}

	AnimationControl(Milliseconds);
}

void GameOutputFrame()
{
	// Draw the background.
	DrawBackground();

	// Draw the rectangles.
	uint32_t i;
	for (i = 0; i < RectangleCount; i++)
	{
		SDL_Rect ColumnDestRect = {
			.x = (int) (Rectangles[i].Left * SCREEN_WIDTH / FIELD_WIDTH) - 20,
			.y = SCREEN_HEIGHT - (int) (Rectangles[i].Top * SCREEN_HEIGHT / FIELD_HEIGHT),
			.w = (int) ((Rectangles[i].Right - Rectangles[i].Left) * SCREEN_WIDTH / FIELD_WIDTH) + 40,
			.h = (int) ((Rectangles[i].Top - Rectangles[i].Bottom) * SCREEN_HEIGHT / FIELD_HEIGHT)
		};
		SDL_Rect ColumnSourceRect = { .x = 0, .y = 0, .w = ColumnDestRect.w, .h = ColumnDestRect.h };
		// Odd-numbered rectangle indices are at the bottom of the field,
		// so start their column image from the top.
		if (i & 1) {
			ColumnSourceRect.y = 0;
		} else {
			ColumnSourceRect.y = 480 - ColumnDestRect.h;
		}
		ColumnSourceRect.x = 64 * Rectangles[i].Frame;
		SDL_BlitSurface(ColumnImage, &ColumnSourceRect, Screen, &ColumnDestRect);
	}

	uint32_t PassedCount = 0;
	for (i = 0; i < RectangleCount; i += 2)
	{
		if (Rectangles[i].Passed)
			PassedCount++;
	}

if (!FollowBee) {
	// Draw the scores corresponding to each rectangle.
	// Above, we grabbed the number of passed rectangles, so now we can get
	// the score represented by the first rectangle shown.
	uint32_t RectScore = Score - PassedCount;
	if (SDL_MUSTLOCK(Screen))
		SDL_LockSurface(Screen);
	for (i = 0; i < RectangleCount; i += 2)
	{
		RectScore++;
		char RectScoreString[11];
		sprintf(RectScoreString, "%" PRIu32, RectScore);
		uint32_t RenderedWidth = GetRenderedWidth(RectScoreString) + 2;
		int32_t Left = (int32_t) (((Rectangles[i].Left + Rectangles[i].Right) / 2) * SCREEN_WIDTH / FIELD_WIDTH) - RenderedWidth / 2;

		if (Left >= 0 && Left + RenderedWidth < SCREEN_WIDTH)
		{
			Uint32 RectScoreColor;
			if (Rectangles[i].Passed)
				RectScoreColor = SDL_MapRGB(Screen->format, 64, 255, 64); // green
			else
				RectScoreColor = SDL_MapRGB(Screen->format, 255, 255, 255); // white
#ifdef USE_16BPP			
			PrintStringOutline16(RectScoreString,
#else
			PrintStringOutline32(RectScoreString,	
#endif
				RectScoreColor,
				SDL_MapRGB(Screen->format, 0, 0, 0),
				Screen->pixels,
				Screen->pitch,
				Left,
				/* Even-numbered rectangle indices are at the top of the field,
				 * so start the Y below that. */
				SCREEN_HEIGHT - (int) (Rectangles[i].Bottom * SCREEN_HEIGHT / FIELD_HEIGHT),
				RenderedWidth,
				(int) (GAP_HEIGHT * SCREEN_HEIGHT / FIELD_HEIGHT),
				CENTER,
				MIDDLE);
		}
	}
}
	if (SDL_MUSTLOCK(Screen))
		SDL_UnlockSurface(Screen);

	// Draw the character.
	SDL_Rect PlayerDestRect = {
		.x = (int) (PlayerX * SCREEN_WIDTH / FIELD_WIDTH) - (PLAYER_FRAME_SIZE / 2),
		.y = (int) (SCREEN_HEIGHT - (PlayerY * SCREEN_HEIGHT / FIELD_HEIGHT)) - (PLAYER_FRAME_SIZE / 2),
		.w = (int) PLAYER_FRAME_SIZE,
		.h = (int) PLAYER_FRAME_SIZE
	};
	SDL_Rect PlayerSourceRect = {
		.x = 0,
		.y = 0,
		.w = 32,
		.h = 32
	};
	
if (FollowBee) {
	// Draw the scores corresponding to each rectangle.
	// Above, we grabbed the number of passed rectangles, so now we can get
	// the score represented by the first rectangle shown.
	uint32_t RectScore = Score - PassedCount;
	if (SDL_MUSTLOCK(Screen))
		SDL_UnlockSurface(Screen);
	for (i = 0; i < 1; i += 2)
	{
		char RectScoreString[11];
		sprintf(RectScoreString, "%" PRIu32, Score);
		uint32_t RenderedWidth = GetRenderedWidth(RectScoreString) + 2;
		int32_t Left = PlayerDestRect.x;
		RectScore++;
		if (Left >= 0 && Left + RenderedWidth < SCREEN_WIDTH)
		{
			Uint32 RectScoreColor;
			RectScoreColor = SDL_MapRGB(Screen->format, 255, 255, 255); // white

			if (PlayerDestRect.y<200) {
#ifdef USE_16BPP			
				PrintStringOutline16(RectScoreString,
#else
				PrintStringOutline32(RectScoreString,	
#endif
					RectScoreColor,
					SDL_MapRGB(Screen->format, 0, 0, 0),
					Screen->pixels,
					Screen->pitch,
					Left,
					/* Even-numbered rectangle indices are at the top of the field,
					 * so start the Y below that. */
					PlayerDestRect.y,
					RenderedWidth,
					(int) (GAP_HEIGHT * SCREEN_HEIGHT / FIELD_HEIGHT),
					CENTER,
					MIDDLE);
			}
		}
	}
}
	
#ifdef DRAW_BEE_COLLISION
	SDL_Rect PlayerPixelsA = {
		.x = (int) ((PlayerX - (COLLISION_A_WIDTH / 2)) * SCREEN_WIDTH / FIELD_WIDTH),
		.y = (int) (SCREEN_HEIGHT - ((PlayerY + (COLLISION_A_HEIGHT / 2)) * SCREEN_HEIGHT / FIELD_HEIGHT)),
		.w = (int) (COLLISION_A_WIDTH * SCREEN_HEIGHT / FIELD_HEIGHT),
		.h = (int) (COLLISION_A_HEIGHT * SCREEN_HEIGHT / FIELD_HEIGHT)
	};
	SDL_Rect PlayerPixelsB = {
		.x = (int) ((PlayerX - (COLLISION_B_WIDTH / 2)) * SCREEN_WIDTH / FIELD_WIDTH),
		.y = (int) (SCREEN_HEIGHT - ((PlayerY + (COLLISION_B_HEIGHT / 2)) * SCREEN_HEIGHT / FIELD_HEIGHT)),
		.w = (int) (COLLISION_B_WIDTH * SCREEN_HEIGHT / FIELD_HEIGHT),
		.h = (int) (COLLISION_B_HEIGHT * SCREEN_HEIGHT / FIELD_HEIGHT)
	};
#endif
	switch (PlayerStatus)
	{
		case ALIVE:
			if (PlayerSpeed > -2.0f) {
				PlayerSourceRect.x = 32 * PlayerFrame;
			} else {
				PlayerSourceRect.x = 128 + 32 * PlayerFrame;
			}
			if (PlayerBlinking)
				PlayerSourceRect.x += 64;
			SDL_BlitSurface(CharacterFrames, &PlayerSourceRect, Screen, &PlayerDestRect);
#ifdef DRAW_BEE_COLLISION
			SDL_FillRect(Screen, &PlayerPixelsA, SDL_MapRGB(Screen->format, 255, 255, 255));
			SDL_FillRect(Screen, &PlayerPixelsB, SDL_MapRGB(Screen->format, 255, 255, 255));
#endif
			break;

		case COLLIDED:
			PlayerSourceRect.w = 48;
			PlayerSourceRect.h = 48;
			PlayerDestRect.x -= 8;
			PlayerDestRect.y -= 8;
			PlayerDestRect.w += 16;
			PlayerDestRect.h += 16;
			SDL_BlitSurface(CollisionImage, &PlayerSourceRect, Screen, &PlayerDestRect);
			break;

		case DYING:
			PlayerSourceRect.x = 256 + 32 * PlayerFrame;
			SDL_BlitSurface(CharacterFrames, &PlayerSourceRect, Screen, &PlayerDestRect);
			break;
	}

#ifdef SDL2
	SDL_BlitScaled(Screen, NULL, sdlSurface, NULL);
	SDL_UpdateWindowSurface(sdlWindow);
#else
	SDL_Flip(Screen);
#endif
}

void ToGame(void)
{

	Score = 0;
	Boost = false;
	Pause = false;
	SetStatus(ALIVE);
	PlayerX = FIELD_WIDTH / 4;
	PlayerY = FIELD_HEIGHT / 2;
	PlayerSpeed = 0.0f;

	PlayerFrame = 0;
	PlayerFrameTime = 0;
	PlayerBlinking = true;
	PlayerBlinkTime = 0;

	if (Rectangles != NULL)
	{
		free(Rectangles);
		Rectangles = NULL;
	}
	RectangleCount = 0;
	GenDistance = RECT_GEN_START;

	GatherInput = GameGatherInput;
	DoLogic     = GameDoLogic;
	OutputFrame = GameOutputFrame;
}
