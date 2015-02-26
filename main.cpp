// ----------------------------------------------------------------
// Awesome simple game with SDL
// http://wiki.libsdl.org/APIByCategory
// ----------------------------------------------------------------

#include "stdlib.h"

#include "SDL\include\SDL.h"
#include "SDL_image\include\SDL_image.h"
#include "SDL_mixer\include\SDL_mixer.h"

#pragma comment( lib, "SDL/libx86/SDL2.lib" )
#pragma comment( lib, "SDL/libx86/SDL2main.lib" )
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )
#pragma comment( lib, "SDL_mixer/libx86/SDL2_mixer.lib" )

// Globals --------------------------------------------------------
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define SCROLL_SPEED 5
#define SHIP_SPEED 3
#define NUM_SHOTS 32
#define SHOT_SPEED 5

struct projectile
{
	int x, y;
	bool alive;
};

struct globals
{
	SDL_Window* window;
	SDL_Surface* w_surface;
	SDL_Renderer* renderer;
	SDL_Texture* background;
	int background_width;
	SDL_Texture* ship;
	int ship_x;
	int ship_y;
	SDL_Texture* shot;
	projectile shots[NUM_SHOTS];
	int last_shot;
	bool fire, up, down, left, right;
	const Uint8 *keys;
	Mix_Music* music;
	Mix_Chunk* fx_shoot;
	int scroll;
};

globals g;

// ----------------------------------------------------------------
void Start()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	// Create window --
	g.window = SDL_CreateWindow("Super Awesome Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	g.w_surface = SDL_GetWindowSurface(g.window);

	// Create renderer --
	g.renderer = SDL_CreateRenderer(g.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawColor(g.renderer, 0, 0, 0, 255);

	// Load image lib --
	IMG_Init(IMG_INIT_PNG);
	g.background = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("background.png"));
	g.ship = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("ship.png"));
	g.shot = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("shot.png"));
	SDL_QueryTexture(g.background, NULL, NULL, &g.background_width, NULL);

	// Create mixer --
	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	g.music = Mix_LoadMUS("music.ogg");
	Mix_PlayMusic(g.music, -1);
	g.fx_shoot = Mix_LoadWAV("laser.wav");

	// Init other vars --
	g.scroll = 0;
	g.ship_x = 100;
	g.ship_y = SCREEN_HEIGHT / 2;
	g.fire = g.up = g.down = g.left = g.right = false;
	g.last_shot = 0;
}

// ----------------------------------------------------------------
void Finish()
{
	Mix_FreeMusic(g.music);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_DestroyTexture(g.background);
	SDL_DestroyTexture(g.ship);
	IMG_Quit();
	SDL_DestroyRenderer(g.renderer);
	SDL_DestroyWindow(g.window);
	SDL_Quit();
}

// ----------------------------------------------------------------
bool CheckInput()
{
	SDL_Event event;

	while(SDL_PollEvent(&event) != 0)
	{
		if(event.type == SDL_KEYUP && event.key.repeat == 0)
		{
			switch(event.key.keysym.sym)
			{
				case SDLK_ESCAPE:
				return false;
				break;
				case SDLK_UP:
				g.up = false;
				break;
				case SDLK_DOWN:
				g.down = false;
				break;
				case SDLK_LEFT:
				g.left = false;
				break;
				case SDLK_RIGHT:
				g.right = false;
				break;
			}
		}
		if(event.type == SDL_KEYDOWN && event.key.repeat == 0)
		{
			switch(event.key.keysym.sym)
			{
				case SDLK_SPACE:
				g.fire = true;
				break;
				case SDLK_UP:
				g.up = true;
				break;
				case SDLK_DOWN:
				g.down = true;
				break;
				case SDLK_LEFT:
				g.left = true;
				break;
				case SDLK_RIGHT:
				g.right = true;
				break;
			}
		}
	}

	return true;
}

// ----------------------------------------------------------------
void MoveStuff()
{
	// Calc new ship position
	if(g.up)
		g.ship_y -= SHIP_SPEED;
	if(g.down)
		g.ship_y += SHIP_SPEED;
	if(g.left)
		g.ship_x -= SHIP_SPEED;
	if(g.right)
		g.ship_x += SHIP_SPEED;

	if(g.fire)
	{
		Mix_PlayChannel(-1, g.fx_shoot, 0);
		g.fire = false;

		if(g.last_shot == NUM_SHOTS)
			g.last_shot = 0;

		g.shots[g.last_shot].alive = true;
		g.shots[g.last_shot].x = g.ship_x + 32;
		g.shots[g.last_shot].y = g.ship_y;
		g.last_shot++;
	}

	for(int i = 0; i < NUM_SHOTS; ++i)
	{
		if(g.shots[i].alive)
		{
			if(g.shots[i].x < SCREEN_WIDTH)
				g.shots[i].x += SHOT_SPEED;
			else
				g.shots[i].alive = false;
		}
	}
}

// ----------------------------------------------------------------
void Draw()
{
	// Clear screen to black
	SDL_RenderClear(g.renderer);

	SDL_Rect target;

	// Pintem el fons amb scroll --

	g.scroll += SCROLL_SPEED;
	if(g.scroll >= g.background_width)
		g.scroll = 0;

	target.x = -g.scroll;
	target.y = 0;
	target.w = g.background_width;
	target.h = SCREEN_HEIGHT;

	SDL_RenderCopy(g.renderer, g.background, NULL, &target);
	target.x += g.background_width;
	SDL_RenderCopy(g.renderer, g.background, NULL, &target);

	// Pintem la nau --
	target.x = g.ship_x;
	target.y = g.ship_y;
	target.w = 64;
	target.h = 64;
	SDL_RenderCopy(g.renderer, g.ship, NULL, &target);

	// Pintem els lasers --
	target.w = 64;
	target.h = 64;

	for(int i = 0; i < NUM_SHOTS; ++i)
	{
		if(g.shots[i].alive)
		{
			target.x = g.shots[i].x;
			target.y = g.shots[i].y;
			SDL_RenderCopy(g.renderer, g.shot, NULL, &target);
		}
	}

	// Finally present
	SDL_RenderPresent(g.renderer);
}

// ----------------------------------------------------------------
int main(int argc, char* args[])
{
	Start();

	while(CheckInput())
	{
		MoveStuff();
		Draw();
	}

	Finish();

	return(EXIT_SUCCESS);
}