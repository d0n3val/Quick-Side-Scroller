// ----------------------------------------------------------------
// Quick Side Scroller (https://github.com/d0n3val/Quick-Side-Scroller)
// Simplistic side scroller made with SDL for learn&fun by Ricard Pillosu
//
// LICENSE
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non - commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain.We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors.We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
// ----------------------------------------------------------------

#include "SDL\include\SDL.h"
#include "SDL_image\include\SDL_image.h"
#include "SDL_mixer\include\SDL_mixer.h"

// Globals for design tweaks -------------------------------------
#define SCROLL_SPEED 2
#define SHIP_SPEED 4
#define NUM_SHOTS 32
#define SHOT_SPEED 20 
#define ENEMY_SPEED 8
#define NUM_ENEMIES 8
#define WAVE_TIMER 4000 // ms between waves
#define INTRO_TIMER 2000 // ms of invulnerability

// Globals for tech tweaks -------------------------------------
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SPRITE_SIZE 64
#define NUM_LAYERS 5
#define ASSETS_DIR "assets/"

// Macros --------------------------------------------------------
#define CAP(value, min, max) (value = (value<min) ? min : ((value>max) ? max : value));

const char* tex_layers[NUM_LAYERS] = { 
	ASSETS_DIR "bg0.png", ASSETS_DIR "bg1.png", ASSETS_DIR "bg2.png", ASSETS_DIR "bg3.png", ASSETS_DIR "fg.png" };

struct enemy
{
	bool alive;
	int x, y;
};

struct parallax
{
	int width, height;
	SDL_Texture* texture;
};

struct projectile
{
	int x, y;
	bool alive;
};

struct globals
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* ship;
	SDL_Texture* shot;
	SDL_Texture* tex_enemy;
	int ship_x, ship_y;
	int last_shot, last_enemy;
	int fire, up, down, left, right;
	int scroll;
	int score, max_score;
	int frame;
	float wave_timer, intro_timer;
	Mix_Music* music;
	Mix_Chunk* fx_shoot;
	SDL_Texture* tex_max;
	struct
	{
		SDL_Texture* tex;
		int w, h;
	} font;
	projectile shots[NUM_SHOTS];
	parallax layers[NUM_LAYERS];
	enemy enemies[NUM_ENEMIES];
} g; // automatically create an insteance called "g"

// ----------------------------------------------------------------
void Start()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_memset(&g, 0, sizeof(g)); // Clear g to 0/null

	// Create window & renderer
	g.window = SDL_CreateWindow("QSS - 0.6", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	g.renderer = SDL_CreateRenderer(g.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Load image lib --
	IMG_Init(IMG_INIT_PNG);
	for (int i = 0; i < NUM_LAYERS; ++i)
	{
		g.layers[i].texture = SDL_CreateTextureFromSurface(g.renderer, IMG_Load(tex_layers[i]));
		SDL_QueryTexture(g.layers[i].texture, nullptr, nullptr, &g.layers[i].width, &g.layers[i].height);
	}
	g.ship = SDL_CreateTextureFromSurface(g.renderer, IMG_Load(ASSETS_DIR "ship.png"));
	g.shot = SDL_CreateTextureFromSurface(g.renderer, IMG_Load(ASSETS_DIR "shot.png"));
	g.tex_enemy = SDL_CreateTextureFromSurface(g.renderer, IMG_Load(ASSETS_DIR "enemy.png"));
	g.tex_max = SDL_CreateTextureFromSurface(g.renderer, IMG_Load(ASSETS_DIR "max.png"));
	g.font.tex = SDL_CreateTextureFromSurface(g.renderer, IMG_Load(ASSETS_DIR "font.png"));
	SDL_QueryTexture(g.font.tex, nullptr, nullptr, &g.font.w, &g.font.h);
	g.font.w /= 10; // w,h are for every single letter, this font should have "0123456789"

	// Create mixer --
	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	g.music = Mix_LoadMUS(ASSETS_DIR "music.ogg");
	Mix_PlayMusic(g.music, -1);
	g.fx_shoot = Mix_LoadWAV(ASSETS_DIR "laser.wav");

	// Init other vars --
	g.ship_x = -SPRITE_SIZE * 3;
	g.ship_y = SCREEN_HEIGHT / 2;
	g.wave_timer = g.intro_timer = SDL_GetTicks();
}

// ----------------------------------------------------------------
void Finish()
{
	Mix_FreeMusic(g.music);
	Mix_FreeChunk(g.fx_shoot);
	Mix_CloseAudio();
	Mix_Quit();

	for(int i = 0; i < NUM_LAYERS; ++i) 
		SDL_DestroyTexture(g.layers[i].texture);
	SDL_DestroyTexture(g.ship);
	SDL_DestroyTexture(g.shot);
	SDL_DestroyTexture(g.tex_enemy);
	SDL_DestroyTexture(g.tex_max);
	SDL_DestroyTexture(g.font.tex);
	IMG_Quit();

	SDL_DestroyRenderer(g.renderer);
	SDL_DestroyWindow(g.window);
	SDL_Quit();
}

// ----------------------------------------------------------------
bool CheckInput()
{
	bool ret = true;
	SDL_Event event;

	while(SDL_PollEvent(&event) != 0)
	{
		if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
		{
			switch(event.key.keysym.sym)
			{
				case SDLK_w: g.up = (event.type == SDL_KEYDOWN); break;
				case SDLK_s: g.down = (event.type == SDL_KEYDOWN); break;
				case SDLK_a: g.left = (event.type == SDL_KEYDOWN); break;
				case SDLK_d: g.right = (event.type == SDL_KEYDOWN); break;
				case SDLK_SPACE: g.fire = (event.type == SDL_KEYDOWN && event.key.repeat == 0); break;
				case SDLK_ESCAPE: ret = false; break;
			}
		}
		else if (event.type == SDL_QUIT)
			ret = false;
	}

	return ret;
}

// ----------------------------------------------------------------
void MoveStuff()
{
	if (g.intro_timer == 0.0f)
	{
		// Calc new ship position
		g.ship_y += (-SHIP_SPEED * g.up) + (SHIP_SPEED * g.down);
		g.ship_x += (-SHIP_SPEED * g.left) + (SHIP_SPEED * g.right);

		// Limit position to current screen
		CAP(g.ship_y, 0, SCREEN_HEIGHT - SPRITE_SIZE);
		CAP(g.ship_x, 0, SCREEN_WIDTH - SPRITE_SIZE);

		// Check if we need to spawn a new laser --
		if(g.fire)
		{
			Mix_PlayChannel(-1, g.fx_shoot, 0);
			g.fire = false;

			if(g.last_shot == NUM_SHOTS)
				g.last_shot = 0;

			g.shots[g.last_shot].alive = true;
			g.shots[g.last_shot].x = g.ship_x + SPRITE_SIZE/2;
			g.shots[g.last_shot].y = g.ship_y;
			++g.last_shot;
		}
	}
	else if (SDL_GetTicks() - g.intro_timer > INTRO_TIMER)
		g.intro_timer = 0.0f; // Cycle invulnerability timer clock 
	else
		g.ship_x += SHIP_SPEED;


	// Move all lasers --
	for(int i = 0; i < NUM_SHOTS; ++i)
	{
		if(g.shots[i].alive)
		{
			if (g.shots[i].x < SCREEN_WIDTH)
			{
				g.shots[i].x += SHOT_SPEED;
				SDL_Rect a = {g.shots[i].x, g.shots[i].y, SPRITE_SIZE, SPRITE_SIZE};
				for (int k = 0; k < NUM_ENEMIES; ++k)
				{
					SDL_Rect b = {g.enemies[k].x, g.enemies[k].y, SPRITE_SIZE, SPRITE_SIZE};
					if (g.enemies[k].alive && SDL_HasIntersection(&a, &b))
					{
						// we have a hit!
						g.shots[i].alive = g.enemies[k].alive = false;
						g.enemies[k].x = -100;
						g.score++;
					}
				}
			}
			else
				g.shots[i].alive = false;
		}
	}

	// Wave timer to decide to spawn enemies of not --
	static int spawn_height = 100;
	if (SDL_GetTicks() - g.wave_timer > WAVE_TIMER)
	{
		if (g.last_enemy == NUM_ENEMIES)
		{
			g.last_enemy = 0;
			g.wave_timer = SDL_GetTicks();
			spawn_height = SPRITE_SIZE + (g.frame % (SCREEN_HEIGHT-SPRITE_SIZE-SPRITE_SIZE));
		}
		else if (g.last_enemy == 0 || g.enemies[g.last_enemy-1].x < SCREEN_WIDTH-SPRITE_SIZE)
		{
			g.enemies[g.last_enemy].alive = true;
			g.enemies[g.last_enemy].x = SCREEN_WIDTH;
			g.enemies[g.last_enemy].y = spawn_height;
			
			++g.last_enemy;
		}
	}
	
	// move all enemies --
	for(int i = 0; i < NUM_ENEMIES; ++i)
	{
		if(g.enemies[i].alive)
		{
			if (g.enemies[i].x > -SPRITE_SIZE)
			{
				g.enemies[i].x -= ENEMY_SPEED;
				g.enemies[i].y += SDL_sinf(g.enemies[i].x/(SCREEN_WIDTH/20)) * 4;
				SDL_Rect a = {g.enemies[i].x, g.enemies[i].y, SPRITE_SIZE, SPRITE_SIZE};
				SDL_Rect b = {g.ship_x, g.ship_y, SPRITE_SIZE, SPRITE_SIZE};
				if (g.intro_timer == 0.0f && SDL_HasIntersection(&a, &b))
				{
					// we have been hit!
					g.enemies[i].alive = false;
					g.enemies[i].x = -100;
					g.score = 0;
					g.intro_timer = SDL_GetTicks();
					g.ship_x = -SPRITE_SIZE * 4;
					g.ship_y = SCREEN_HEIGHT / 2;
				}
			}
			else
				g.enemies[i].alive = false;
		}
	}
}

// ----------------------------------------------------------------
void Draw()
{
	SDL_Rect target, section;

	// Scroll and draw all parallax layers --
	g.scroll += SCROLL_SPEED;
	for (int i = 0; i < NUM_LAYERS; ++i)
	{
		parallax* p = &g.layers[i];
		target = { (-g.scroll * i) % p->width, SCREEN_HEIGHT - p->height, p->width, p->height };
		SDL_RenderCopy(g.renderer, p->texture, nullptr, &target);
		target.x += p->width;
		SDL_RenderCopy(g.renderer, p->texture, nullptr, &target);
	}

	// Draw player's ship --
	if (g.intro_timer == 0.0f || g.frame % 2)
	{
		target = { g.ship_x, g.ship_y, SPRITE_SIZE, SPRITE_SIZE };
		SDL_RenderCopy(g.renderer, g.ship, nullptr, &target);
	}

	// Draw lasers --
	for(int i = 0; i < NUM_SHOTS; ++i)
	{
		if(g.shots[i].alive)
		{
			target = { g.shots[i].x, g.shots[i].y, SPRITE_SIZE, SPRITE_SIZE };
			SDL_RenderCopy(g.renderer, g.shot, nullptr, &target);
		}
	}

	// Draw enemies ---
	for(int i = 0; i < NUM_ENEMIES; ++i)
	{
		if(g.enemies[i].alive)
		{
			target = { g.enemies[i].x, g.enemies[i].y, SPRITE_SIZE, SPRITE_SIZE };
			SDL_RenderCopy(g.renderer, g.tex_enemy, nullptr, &target);
		}
	}

	// Draw score --
	if (g.score > g.max_score)
		g.max_score = g.score;

	target = { SCREEN_WIDTH - 20 - g.font.w*8, 10, g.font.w*3, g.font.h };
	SDL_RenderCopy(g.renderer, g.tex_max, nullptr, &target);

	for (int n = 0; n < 2; ++n)
	{
		int num = (n ? g.score : g.max_score);
		for (int i = 0; i < 5; ++i)
		{
			target = { SCREEN_WIDTH - 10 - (g.font.w*(i + 1)), 10 + ((10+g.font.h)*n), g.font.w, g.font.h };
			section = { (num % 10) * g.font.w, 0, g.font.w, g.font.h };
			SDL_RenderCopy(g.renderer, g.font.tex, &section, &target);
			num /= 10;
		}
	}
	
	// Finally swap buffers --
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
		++g.frame;
	}

	Finish();

	return(0); // EXIT_SUCCESS
}