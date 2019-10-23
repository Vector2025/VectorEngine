#pragma once

#include <thread>

#include "Engine.hpp"
#include "Util.hpp"
#include "ResourceManager.hpp"
#include "System.hpp"

class FpsCounterSystem : public System, public Renderer {
	sf::Time updateElapsed;
	sf::Text text;
	int updateFPS = 0;

public:

	FpsCounterSystem() : System(typeid(FpsCounterSystem)) {
		text.setFont(*Resources::load<sf::Font>("KeepCalm-Medium.ttf"));
		text.setCharacterSize(15);
		text.setFillColor(sf::Color::White);
	}


private:
	void update() override {
		updateElapsed += ArkEngine::deltaTime();
		updateFPS += 1;
		if (updateElapsed.asMilliseconds() >= 1000) {
			updateElapsed -= sf::milliseconds(1000);
			//log("fps: %d", updateFPS);
			text.setString("FPS:" + std::to_string(updateFPS));
			updateFPS = 0;
		}
	}

	void render(sf::RenderTarget& target) override {
		target.draw(text);
	}
};