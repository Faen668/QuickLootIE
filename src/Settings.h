#pragma once
namespace QuickLoot
{
	enum AnchorPoint
	{
		kTopLeft,
		kCenterLeft,
		kBottomLeft,
		kTopCenter,
		kCenter,
		kBottomCenter,
		kTopRight,
		kCenterRight,
		kBottomRight,
	};

	class Settings
	{
	public:
		Settings(Settings const&) = delete;
		Settings(Settings const&&) = delete;
		Settings operator=(Settings&) = delete;
		Settings operator=(Settings&&) = delete;

		static bool CloseInCombat();
		static bool CloseWhenEmpty();
		static bool DispelInvisibility();
		static bool OpenWhenContainerUnlocked();
		static bool DisableForAnimals();
		static bool DisableForMonsters();

		static bool ShowBookRead();
		static bool ShowEnchanted();
		static bool ShowDBMDisplayed();
		static bool ShowDBMFound();
		static bool ShowDBMNew();

		static bool ShowCompNeeded();
		static bool ShowCompCollected();
		
		static AnchorPoint GetAnchorPoint();
		static int32_t GetWindowX();
		static int32_t GetWindowY();
		static float GetWindowScale();

		static int32_t GetMinLines();
		static int32_t GetMaxLines();

		static float GetNormalWindowTransparency();
		static float GetEmptyWindowTransparency();

		static std::shared_ptr<std::vector<std::string>> GetUserDefinedSortPriority();

	private:
		Settings() = default;
		~Settings() = default;
	};
}
