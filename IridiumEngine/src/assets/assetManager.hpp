namespace Iridium {
	enum class asset_type {
		none,
		shader,
		mesh,
		image,
		text,
		any,
	};

	class asset { //asset base class
	friend class asset_manager;
	public:
		virtual asset_type getAssetType() { return asset_type::none; }
	};

	class asset_manager {
	public:
		
	private:
		
	};

	asset_manager* getAssetManager();
}