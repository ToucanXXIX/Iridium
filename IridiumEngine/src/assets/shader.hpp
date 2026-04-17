#pragma once
#include "assetManager.hpp"

#include <cstdint>
#include <filesystem>

namespace Iridium {
	enum class shader_type {
		none,
		vertex,
		tesselation,
		geometry,
		fragment,
		compute,
	};

	class shader_asset : public asset {
	public:
		asset_type getAssetType() { return asset_type::shader; }

		shader_asset();
		shader_asset(const char* path, shader_type type);
		shader_asset(std::filesystem::path path, shader_type type);
	private:
		uint32_t* m_spirv;
		shader_type m_type;
	};
}