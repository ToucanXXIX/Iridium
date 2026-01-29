#include "appinfo.hpp"
#include "entryPoint.hpp"

class demo : public Iridium::application {
	const Iridium::appinfo& getAppinfo() override {
		static Iridium::appinfo info{
			.name = "Demo"
		};
		return info;
	}
};

void init() {
}