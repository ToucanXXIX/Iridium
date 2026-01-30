#include "appinfo.hpp"
#include "entryPoint.hpp"

class demo final : public Iridium::application {
	const Iridium::appinfo& getAppinfo() override {
		static Iridium::appinfo info{
			.name = "Demo",
			.version = {1,0,0}
		};
		return info;
	}
};

void init() {
}