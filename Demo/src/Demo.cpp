#include <thread.hpp>

#include "appinfo.hpp"
#include "entryPoint.hpp"
#include "renderer/window.hpp"
#include "inputHandler.hpp"
#include "renderer/renderer.hpp"
#include "utils.hpp"
#include "log.hpp"

//#include "glm/glm.hpp"


class demo final : public Iridium::application {
public:
	glm::vec3 pos{};
	size_t counter = 0;

	Iridium::appinfo& getAppinfo() override {
		static Iridium::appinfo info{
			.name = "Demo",
			.version = {1,0,0}
		};
		return info;
	}

	void onTick(float dt) override {		
		windowManager->pollEvents();
		glm::vec3 moveVector{};
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_W)) {
			moveVector += glm::vec3(1.0, 0.0, 0.0);
		}
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_S)) {
			moveVector += glm::vec3(-1.0, 0.0, 0.0);
		}
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_A)) {
			moveVector += glm::vec3(0.0, 1.0, 0.0);
		}
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_D)) {
			moveVector += glm::vec3(0.0, -1.0, 0.0);
		}
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_SPACE)) {
			moveVector += glm::vec3(0.0, 0.0, 1.0);
		}
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_RCONTROL) || Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_LCONTROL)) {
			moveVector += glm::vec3(0.0, 0.0, -1.0);
		}
		if(glm::length(moveVector)) {
			moveVector = glm::normalize(moveVector);
		}

		static bool heldR = false;
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_R)) {
			if(!heldR)
				renderer->drawWireframe = !renderer->drawWireframe;
			heldR = true;
		} else {
			heldR = false;
		}

		static bool heldEnt = false;
		if(Iridium::getInputHandler()->isKeyPressed(Iridium::KEY_ENTER)) {
			if(!heldEnt) {
				auto unicodeText = Iridium::getInputHandler()->getTextInputAndClear();
				std::vector<char> utf8Text = Iridium::unicodeToUTF8(unicodeText);
				utf8Text.push_back('\0');
				ENGINE_LOG_ERROR("{}", (char*)utf8Text.data());
			}
			heldEnt = true;
		} else {
			heldEnt = false;
		}

		moveVector *= 0.001f * dt;
		pos += moveVector;
		renderer->setCameraPos(pos);
		std::this_thread::sleep_for(std::chrono::milliseconds(50)); // No sleep causes weird issue
		}

	demo() : Iridium::application(getAppinfo()) {

	}
};

Iridium::application& createApp() {
	demo* demo_app = ::new demo();
	return *demo_app;
}

void init() {
}