#include<SDL.h>
#include<sdl_ttf.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <cmath>
#include <fstream>
#include <sstream>
#include <time.h>
#include "game.h"
#include "json.hpp"
#include <chrono>
#include <random>

class Rnd {
public:
	std::mt19937 rng;

	Rnd()
	{
		std::mt19937 prng(std::chrono::steady_clock::now().time_since_epoch().count());
		rng = prng;
	}

	int getRndInt(int min, int max)
	{
		std::uniform_int_distribution<int> distribution(min, max);
		return distribution(rng);
	}

	double getRndDouble(double min, double max)
	{
		std::uniform_real_distribution<double> distribution(min, max);
		return distribution(rng);
	}
} rnd;

//la clase juego:
Game* Game::s_pInstance = 0;

Game::Game()
{
	m_pRenderer = NULL;
	m_pWindow = NULL;
}

Game::~Game()
{

}

SDL_Window* g_pWindow = 0;
SDL_Renderer* g_pRenderer = 0;

bool Game::init(const char* title, int xpos, int ypos, int width,
	int height, bool fullscreen)
{
	// almacenar el alto y ancho del juego.
	m_gameWidth = width;
	m_gameHeight = height;

	// attempt to initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		int flags = 0;
		if (fullscreen)
		{
			flags = SDL_WINDOW_FULLSCREEN;
		}

		std::cout << "SDL init success\n";
		// init the window
		m_pWindow = SDL_CreateWindow(title, xpos, ypos,
			width, height, flags);
		if (m_pWindow != 0) // window init success
		{
			std::cout << "window creation success\n";
			m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
			if (m_pRenderer != 0) // renderer init success
			{
				std::cout << "renderer creation success\n";
				SDL_SetRenderDrawColor(m_pRenderer,
					255, 255, 255, 255);
			}
			else
			{
				std::cout << "renderer init fail\n";
				return false; // renderer init fail
			}
		}
		else
		{
			std::cout << "window init fail\n";
			return false; // window init fail
		}
	}
	else
	{
		std::cout << "SDL init fail\n";
		return false; // SDL init fail
	}
	if (TTF_Init() == 0)
	{
		std::cout << "sdl font initialization success\n";
	}
	else
	{
		std::cout << "sdl font init fail\n";
		return false;
	}

	std::cout << "init success\n";
	m_bRunning = true; // everything inited successfully, start the main loop

	//Joysticks
	InputHandler::Instance()->initialiseJoysticks();

	//load images, sounds, music and fonts
	//AssetsManager::Instance()->loadAssets();
	AssetsManager::Instance()->loadAssetsJson(); //ahora con formato json
	Mix_Volume(-1, 16); //adjust sound/music volume for all channels

	p = new player();
	p->settings("chicken", Vector2D(4, 175), Vector2D(0,0), 58, 58, 0, 0, 0, 0.0, 0);
	entities.push_back(p);

	//car creation
	c1 = new car();
	c2 = new car();
	c3 = new car();
	c4 = new car();
	c1->settings("car1", Vector2D(70, 0), Vector2D(0, 1), 74, 126, 0, 0, 0, 0.0, 0);
	c2->settings("car2", Vector2D(160, 0), Vector2D(0, 2), 56, 126, 0, 0, 0, 0.0, 0);
	c3->settings("car3", Vector2D(239, 400), Vector2D(0, -1), 73, 109, 0, 0, 0, 0.0, 0);
	c4->settings("car4", Vector2D(329, 400), Vector2D(0, -3), 56, 126, 0, 0, 0, 0.0, 0);
	entities.push_back(c1);
	entities.push_back(c2);
	entities.push_back(c3);
	entities.push_back(c4);

	ReadHiScores();
	
	state = MENU;

	//crear el archivo json
	/*nlohmann::json j;

	j["fnt"]["font"] = "sansation.ttf";
	j["ico"]["lchicken"] = "henway.ico";
	j["ico"]["lhead"] = "henway_sm.ico";
	j["img"]["car1"] = "car1.png";
	j["img"]["car2"] = "car2.png";
	j["img"]["car3"] = "car3.png";
	j["img"]["car4"] = "car4.png";
	j["img"]["chicken"] = "chicken.png";
	j["img"]["chickenhead"] = "chickenhead.png";
	j["img"]["highway"] = "highway.png";
	j["mus"]["music"] = "music.ogg";
	j["snd"]["bok"] = "bok.wav";
	j["snd"]["carhorn1"] = "carhorn1.wav";
	j["snd"]["carhorn2"] = "carhorn2.wav";
	j["snd"]["celebrate"] = "celebrate.wav";
	j["snd"]["gameover"] = "gameover.wav";
	j["snd"]["squish"] = "squish.wav";

	std::ofstream o("assets.json");
	o << std::setw(4) << j << std::endl;*/

	return true;
}

void Game::render()
{
	SDL_RenderClear(m_pRenderer); // clear the renderer to the draw color

	AssetsManager::Instance()->draw("highway", 0, 0, 465, 400, m_pRenderer, SDL_FLIP_NONE);

		if (state == MENU)
		{
			AssetsManager::Instance()->Text("Menu , press S to play", "font", 100, 100, SDL_Color({ 0,0,0,0 }), getRenderer());

			////Show hi scores
			int y = 350;
			AssetsManager::Instance()->Text("HiScores", "font", 580 - 50, y, SDL_Color({ 255,255,255,0 }), getRenderer());
			for (int i = 0; i < 5; i++) {
				y += 30;
				AssetsManager::Instance()->Text(std::to_string(vhiscores[i]), "font", 580, y, SDL_Color({ 255,255,255,0 }), getRenderer());
			}
		}

		if (state == GAME)
		{
			for (auto i : entities)
				i->draw();

			// draw the lives
			int dx = 20;
			for (int i = 0; i < lives; i++) {
				AssetsManager::Instance()->draw("chickenhead", 200 + dx, 10, 18, 16, Game::getRenderer());
				dx += 20;
			}

			// Draw the score
			std::string sc = "Score: " + std::to_string(score);
			AssetsManager::Instance()->Text(sc, "font", 0, 0, SDL_Color({ 255,255,255,0 }), getRenderer());
		}

		if (state == END_GAME)
		{
			AssetsManager::Instance()->Text("End Game press space", "font", 100, 100, SDL_Color({ 0,0,0,0 }), Game::Instance()->getRenderer());
		}

	SDL_RenderPresent(m_pRenderer); // draw to the screen
}

void Game::quit()
{
	m_bRunning = false;
}

void Game::clean()
{
	WriteHiScores();
	std::cout << "cleaning game\n";
	InputHandler::Instance()->clean();
	AssetsManager::Instance()->clearFonts();
	TTF_Quit();
	SDL_DestroyWindow(m_pWindow);
	SDL_DestroyRenderer(m_pRenderer);
	Game::Instance()->m_bRunning = false;
	SDL_Quit();
}

void Game::handleEvents()
{
	InputHandler::Instance()->update();

	//HandleKeys
	if (state == MENU)
	{
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_S))
		{
			state = GAME;
			lives = 3;
			score = 0;
			AssetsManager::Instance()->playMusic("music", 1);
		}
	}

	if (state == GAME)
	{
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_RIGHT)) p->m_velocity.m_x = 2;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_LEFT)) p->m_velocity.m_x = -2;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_UP)) p->m_velocity.m_y = -2;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_DOWN)) p->m_velocity.m_y = 2;
	}

	if (state == END_GAME)
	{
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_SPACE)) state = MENU;
	}

}

bool Game::isCollide(Entity *a, Entity *b)
{
	return (b->m_position.m_x - a->m_position.m_x)*(b->m_position.m_x - a->m_position.m_x) +
		(b->m_position.m_y - a->m_position.m_y)*(b->m_position.m_y - a->m_position.m_y) <
		(a->m_radius + b->m_radius)*(a->m_radius + b->m_radius);
}

bool Game::isCollideRect(Entity *a, Entity * b) {
	if (a->m_position.m_x < b->m_position.m_x + b->m_width &&
		a->m_position.m_x + a->m_width > b->m_position.m_x &&
		a->m_position.m_y < b->m_position.m_y + b->m_height &&
		a->m_height + a->m_position.m_y > b->m_position.m_y) {
		return true;
	}
	return false;
}

void Game::update()
{
	if (state == GAME)
	{

		//play some random car horns
		if (rnd.getRndInt(0, 100) == 0)
		{
			if (rnd.getRndInt(0, 1) == 0)
				AssetsManager::Instance()->playSound("carhorn1", 0);
			else
				AssetsManager::Instance()->playSound("carhorn2", 0);
		}

		// See if the chicken made it across
		if (p->m_position.m_x > 400.0)
		{
			// Play a sound for the chicken making it safely across
			AssetsManager::Instance()->playSound("celebrate", 0);

			// Move the chicken back to the start and add to the score
			p->m_position.m_x = 4; p->m_position.m_y = 175;
			score += 150;
		}

		for (auto a : entities)
		{
			for (auto b : entities)
			{
				if (a->m_name == "player" && b->m_name == "car")
					if (isCollideRect(a, b))
					{
						lives--;
						if (lives <= 0)
						{
							AssetsManager::Instance()->playSound("gameover", 0);
							state = END_GAME;
						}
						else
						{
							AssetsManager::Instance()->playSound("squish", 0);
						}

						//relocate the chicken
						p->m_position.m_x = 4; p->m_position.m_y = 175;
						p->m_velocity.m_x = 0;
						p->m_velocity.m_y = 0;
					}
			}
		}

		for (auto i = entities.begin(); i != entities.end(); i++)
		{
			Entity *e = *i;

			e->update();
		}
	}

}

void Game::UpdateHiScores(int newscore)
{
	//new score to the end
	vhiscores.push_back(newscore);
	//sort
	sort(vhiscores.rbegin(), vhiscores.rend());
	//remove the last
	vhiscores.pop_back();
}

void Game::ReadHiScores()
{
	std::ifstream in("hiscores.dat");
	if (in.good())
	{
		std::string str;
		getline(in, str);
		std::stringstream ss(str);
		int n;
		for (int i = 0; i < 5; i++)
		{
			ss >> n;
			vhiscores.push_back(n);
		}
		in.close();
	}
	else
	{
		//if file does not exist fill with 5 scores
		for (int i = 0; i < 5; i++)
		{
			vhiscores.push_back(0);
		}
	}
}

void Game::WriteHiScores()
{
	std::ofstream out("hiscores.dat");
	for (int i = 0; i < 5; i++)
	{
		out << vhiscores[i] << " ";
	}
	out.close();
}

const int FPS = 60;
const int DELAY_TIME = 1000.0f / FPS;

int main(int argc, char* args[])
{
	srand(time(NULL));

	Uint32 frameStart, frameTime;

	std::cout << "game init attempt...\n";
	if (Game::Instance()->init("SDLHenway", 100, 100, 465, 400,
		false))
	{
		std::cout << "game init success!\n";
		while (Game::Instance()->running())
		{
			frameStart = SDL_GetTicks(); //tiempo inicial

			Game::Instance()->handleEvents();
			Game::Instance()->update();
			Game::Instance()->render();

			frameTime = SDL_GetTicks() - frameStart; //tiempo final - tiempo inicial

			if (frameTime < DELAY_TIME)
			{
				SDL_Delay((int)(DELAY_TIME - frameTime)); //esperamos hasta completar los 60 fps
			}
		}
	}
	else
	{
		std::cout << "game init failure - " << SDL_GetError() << "\n";
		return -1;
	}
	std::cout << "game closing...\n";
	Game::Instance()->clean();
	return 0;
}