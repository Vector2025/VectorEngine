#include "VectorEngine.hpp"
#include <iostream>
#include <vector>

#define VEngineDebug false

#if VEngineDebug
#define debug_log(fmt, ...) log(fmt, __VA_ARGS__)
#else
#define debug_log(fmt, ...)
#endif

int getUniqueComponentID()
{
	static int id = 0;
	id += 1;
	return id;
}

System::~System() { }

void Entity::setActive(bool b)
{
	this->isActive = b;
	for (auto& ids : this->components) {
		this->scene->setComponentActive(ids.first, ids.second, b);
	}
}

void Entity::setAction(std::function<void(Entity&, std::any)> f, std::any args)
{
	this->action = f;
	if (VectorEngine::running())
		this->action(*this, std::move(args));
	else {
		if(args.has_value())
			// save argumets until VectorEngine::run() is called
			this->actionArgs = std::make_unique<std::any>(std::move(args));
	}
}

std::deque<Entity>& System::getEntities()
{
	return VectorEngine::currentScene->entities;
}

Entity* Scene::createEntity()
{
	this->entities.emplace_back();
	Entity* e = &this->entities.back();
	e->scene = this;
	return e;
}

void Scene::setComponentActive(int typeId, int index, bool b)
{
	auto& data = this->componentTable.at(typeId);
	data.active.at(index) = b;
}

inline int Scene::getComponentSize(int id)
{
	auto& data = this->componentTable.at(id);
	return data.sizeOfComponent;
}

void VectorEngine::create(sf::VideoMode vm, std::string name, sf::Time fixedUT, sf::ContextSettings settings)
{
	frameTime = fixedUT;
	width = vm.width;
	height = vm.height;
	view.setSize(vm.width, vm.height);
	view.setCenter(0, 0);
	window.create(vm, name, sf::Style::Close | sf::Style::Resize, settings);
}

void VectorEngine::initScene()
{
	currentScene->init();

	for (auto& s : currentScene->systems)
		s->init();

	for (auto& s : currentScene->scripts)
		s->init();

	for (auto& e : currentScene->entities)
		if (e.action) {
			if (e.actionArgs && e.actionArgs->has_value()) {
				e.action(e, std::move(*e.actionArgs));
			} else {
				e.action(e, nullptr);
			}
		}
}

void VectorEngine::run()
{
	auto scriptsLag = sf::Time::Zero;
	auto systemsLag = sf::Time::Zero;

	debug_log("start game loop");
	running_ = true;
	while (window.isOpen()) {

		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::Resized: {
				auto[x, y] = window.getSize();
				auto aspectRatio = float(x) / float(y);
				view.setSize({ width * aspectRatio, (float)height });
				//window.setView(view);
			}	break;
			default:
				for (auto& s : currentScene->systems)
					s->handleEvent(event);
				for (auto& s : currentScene->scripts)
					s->handleEvent(event);
				break;
			}
		}
		window.clear(backGroundColor);

		delta_time = clock.restart();

		for (auto& s : currentScene->scripts)
			if(s->entity()->isActive) // TODO: vector<bool> activeScripts; or a 'bool active' member of 'class Script'
				s->update();

		scriptsLag += deltaTime();
		while (scriptsLag >= frameTime) {
			scriptsLag -= frameTime;
			for (auto& s : currentScene->scripts)
				if(s->entity()->isActive)
					s->fixedUpdate();
		}

		for (auto& system : currentScene->systems)
			system->update();

		systemsLag += deltaTime();
		while (systemsLag >= frameTime) {
			systemsLag -= frameTime;
			for (auto& system : currentScene->systems)
				system->fixedUpdate();
		}

		for (auto& system : currentScene->systems)
			system->render(window);

		window.display();
	}
}
