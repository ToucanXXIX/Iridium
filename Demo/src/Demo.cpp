#include "appinfo.hpp"
#include "entryPoint.hpp"


class demo final : public Iridium::application {
public:
	Iridium::appinfo& getAppinfo() override {
		static Iridium::appinfo info{
			.name = "Demo",
			.version = {1,0,0}
		};
		return info;
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