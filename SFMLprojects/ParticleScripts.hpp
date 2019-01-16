#pragma once
#include <fstream>
#include <thread>
#include <chrono>
#include "Particles.hpp"
#include "VectorEngine.hpp"

#if 0
#define log_init() log("")
#else
#define log_init()
#endif

namespace ParticlesScripts {

	class EmittFromMouse : public Script {
		Particles* p;
	public:
		void init() override
		{
			log_init();
			p = getComponent<Particles>();
		}
		void update() override
		{
			p->emitter = VectorEngine::mousePositon();
		}
	};

	class TraillingEffect : public Script {
		sf::Vector2f prevEmitter;
		Particles* p;
	public:
		bool spawn = true;

		void init()
		{
			log_init();
			p = getComponent<Particles>();
		}

		void update() override
		{
			if (spawn) {
				auto dv = p->emitter - prevEmitter;
				auto[speed, angle] = toPolar(-dv);
				if (speed != 0) {
					p->spawn = true;
					speed = std::clamp<float>(speed, 0, 5);
				} else
					p->spawn = false;
				p->angleDistribution.values = { angle - PI / 6, angle + PI / 6 };
				p->speedDistribution.values = { 5 * speed , 20 * speed };
				prevEmitter = p->emitter;
			}
		}
	};

	class RegisterMousePath : public Script {
		std::vector<sf::Vector2f> path;
		std::string file;
	public:

		RegisterMousePath(std::string file) : file(file) { }

		void update()
		{
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
				path.push_back(VectorEngine::mousePositon());
		}

		~RegisterMousePath()
		{
			if (path.empty())
				return;
			std::ofstream fout(file);
			fout << path.size() << ' ';
			for (auto[x, y] : path)
				fout << x << ' ' << y << ' ';
		}
	};

	class PlayModel : public Script {
		std::vector<sf::Vector2f> model;
		std::vector<sf::Vector2f>::iterator curr;
		std::string file;
		Particles* p;
		sf::Vector2f offset;
	public:

		PlayModel(std::string file, sf::Vector2f offset = { 0.f, 0.f }) : file(file), offset(offset) { }

		void init()
		{
			log_init();
			p = getComponent<Particles>();
			std::ifstream fin(file);
			if (!fin.is_open()) {
				std::cerr << "nu-i fisierul: " + file + "\n";
				std::getchar();
				exit(-2025);
			}
			int size;
			fin >> size;
			model.resize(size);
			for (auto&[x, y] : model) {
				fin >> x >> y;
				x += offset.x;
				y += offset.y;
			}
			curr = model.begin();
			p->spawn = false;
		}

		void update()
		{
			if (curr == model.begin())
				p->spawn = true;
			if (curr != model.end()) {
				auto pixel = *curr++;
				p->emitter = pixel;
				if (curr != model.end())
					curr++;
				if (curr != model.end())
					curr++;
			}
			else
				p->spawn = false;
		}
	};

	class ModifyColorsFromFile : public Script {
		Particles* p;
		std::string fileName;
	public:

		ModifyColorsFromFile(std::string fileName) : fileName(fileName) { }

		void init()
		{
			p = getComponent<Particles>();
			std::thread t([&](){
				while (true) {
					std::cout << "press enter to read colors from: " + fileName;
					std::cin.get();
					std::ifstream fin(fileName);
					int r1, r2, g1, g2, b1, b2;
					fin >> r1 >> r2 >> g1 >> g2 >> b1 >> b2;
					p->getColor = [=]() {
						auto r = RandomNumber<int>(r1, r2);
						auto g = RandomNumber<int>(g1, g2);
						auto b = RandomNumber<int>(b1, b2);
						return sf::Color(r, g, b);
					};
					fin.close();
				}
			});
			t.detach();
		}
	};

	class ModifyColorsFromConsole : public Script {
		Particles* p;
	public:
		void init()
		{
			p = getComponent<Particles>();
			std::thread t([&]() {
				while (true) {
					std::cout << "enter rgb low and high: ";
					int r1, r2, g1, g2, b1, b2;
					std::cin >> r1 >> r2 >> g1 >> g2 >> b1 >> b2;
					std::cout << std::endl;
					p->getColor = [=]() {
						auto r = RandomNumber<int>(r1, r2);
						auto g = RandomNumber<int>(g1, g2);
						auto b = RandomNumber<int>(b1, b2);
						return sf::Color(r, g, b);
					};
				}
			});
			t.detach();
		}
	};

	class SpawnLater : public Script {
		int seconds;
		Particles* p;
	public:
		SpawnLater(int seconds) : seconds(seconds) { }

		void init()
		{
			static int i = 1;
			log_init("%d", i++);
			p = getComponent<Particles>();
			p->spawn = false;

			std::thread waitToSpawn([pp = p, sec = seconds]() {
				std::this_thread::sleep_for(std::chrono::seconds(sec));
				pp->spawn = true;
			});
			waitToSpawn.detach();

			this->seppuku();
		}
	};

	template <typename T = Particles>
	class DeSpawnOnMouseClick : public Script {
		T* p = nullptr;
	public:
		void init() override
		{
			log_init();
			if (std::is_base_of_v<Script, T>)
				p = getScript<T>();
			if (std::is_same_v<Particles, T>)
				p = getComponent<T>();
		}
		void handleInput(sf::Event event)
		{
			switch (event.type) {
			case sf::Event::MouseButtonPressed:
				p->spawn = false;
				break;
			case sf::Event::MouseButtonReleased:
				p->spawn = true;
				break;
			default:
				break;
			}
		}
	};

	class SpawnOnRightClick : public Script {
		Particles* p;
	public:
		void init()
		{
			log_init();
			p = getComponent<Particles>();
		}
		void handleInput(sf::Event ev)
		{
			switch (ev.type)
			{
			case sf::Event::MouseButtonPressed:
				if (ev.mouseButton.button == sf::Mouse::Right)
					p->spawn = true;
				break;
			case sf::Event::MouseButtonReleased:
				if (ev.mouseButton.button == sf::Mouse::Right)
					p->spawn = false;
				break;
			default:
				break;
			}
		}
	};

	class SpawnOnLeftClick : public Script {
		Particles* p;
	public:
		void init()
		{
			log_init();
			p = getComponent<Particles>();
		}
		void handleInput(sf::Event ev)
		{
			switch (ev.type)
			{
			case sf::Event::MouseButtonPressed:
				if (ev.mouseButton.button == sf::Mouse::Left)
					p->spawn = true;
				break;
			case sf::Event::MouseButtonReleased:
				if (ev.mouseButton.button == sf::Mouse::Left)
					p->spawn = false;
				break;
			default:
				break;
			}
		}
	};
}
