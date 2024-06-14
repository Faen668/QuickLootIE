#include "GFxItem.h"

#undef GetModuleHandle

static const char* strIcons[] = {
	"none",  // 00
	"default_weapon",
	"weapon_sword",
	"weapon_greatsword",
	"weapon_daedra",
	"weapon_dagger",
	"weapon_waraxe",
	"weapon_battleaxe",
	"weapon_mace",
	"weapon_hammer",
	"weapon_staff",  // 10
	"weapon_bow",
	"weapon_arrow",
	"weapon_pickaxe",
	"weapon_woodaxe",
	"weapon_crossbow",
	"weapon_bolt",
	"default_armor",
	"lightarmor_body",
	"lightarmor_head",
	"lightarmor_hands",  // 20
	"lightarmor_forearms",
	"lightarmor_feet",
	"lightarmor_calves",
	"lightarmor_shield",
	"lightarmor_mask",
	"armor_body",
	"armor_head",
	"armor_hands",
	"armor_forearms",
	"armor_feet",  // 30
	"armor_calves",
	"armor_shield",
	"armor_mask",
	"armor_bracer",
	"armor_daedra",
	"clothing_body",
	"clothing_robe",
	"clothing_head",
	"clothing_pants",
	"clothing_hands",  // 40
	"clothing_forearms",
	"clothing_feet",
	"clothing_calves",
	"clothing_shoes",
	"clothing_shield",
	"clothing_mask",
	"armor_amulet",
	"armor_ring",
	"armor_circlet",
	"default_scroll",  // 50
	"default_book",
	"default_book_read",
	"book_tome",
	"book_tome_read",
	"book_journal",
	"book_note",
	"book_map",
	"default_food",
	"food_wine",
	"food_beer",  // 60
	"default_ingredient",
	"default_key",
	"key_house",
	"default_potion",
	"potion_health",
	"potion_stam",
	"potion_magic",
	"potion_poison",
	"potion_frost",
	"potion_fire",  // 70
	"potion_shock",
	"default_misc",
	"misc_artifact",
	"misc_clutter",
	"misc_lockpick",
	"misc_soulgem",
	"soulgem_empty",
	"soulgem_partial",
	"soulgem_full",
	"soulgem_grandempty",  // 80
	"soulgem_grandpartial",
	"soulgem_grandfull",
	"soulgem_azura",
	"misc_gem",
	"misc_ore",
	"misc_ingot",
	"misc_hide",
	"misc_strips",
	"misc_leather",
	"misc_wood",  // 90
	"misc_remains",
	"misc_trollskull",
	"misc_torch",
	"misc_goldsack",
	"misc_gold",
	"misc_dragonclaw"
};

using namespace RE;

struct CompletionistRequestEE
{
	RE::FormID m_formId;
};

struct CompletionistResponseEE
{
	RE::FormID m_formId;
	std::string m_displayname;
};

static std::optional<CompletionistResponseEE> comp_response{ std::nullopt };
static bool comp_installed{};

namespace Items
{
	GFxItem::GFxItem(std::ptrdiff_t a_count, bool a_stealing, SKSE::stl::observer<RE::InventoryEntryData*> a_item)
		: _src(a_item)
		, _count(a_count)
		, _stealing(a_stealing)
	{
		assert(a_item != nullptr);
	}

	GFxItem::GFxItem(std::ptrdiff_t a_count, bool a_stealing, std::span<const RE::ObjectRefHandle> a_items)
		: _src(a_items)
		, _count(a_count)
		, _stealing(a_stealing)
	{}

	int GFxItem::Compare(const GFxItem& a_rhs) const
	{
		if (IsQuestItem() != a_rhs.IsQuestItem()) {
			return IsQuestItem() ? -1 : 1;
		} else if (IsKey() != a_rhs.IsKey()) {
			return IsKey() ? -1 : 1;
		} else if (IsNote() != a_rhs.IsNote()) {
			return IsNote() ? -1 : 1;
		} else if (IsBook() != a_rhs.IsBook()) {
			return IsBook() ? -1 : 1;
		} else if (IsGold() != a_rhs.IsGold()) {
			return IsGold() ? -1 : 1;
		} else if (IsAmmo() != a_rhs.IsAmmo()) {
			return IsAmmo() ? -1 : 1;
		} else if (IsLockpick() != a_rhs.IsLockpick()) {
			return IsLockpick() ? -1 : 1;
		} else if (GetValue() != a_rhs.GetValue()) {
			return GetValue() > a_rhs.GetValue() ? -1 : 1;
		} else if (const auto alphabetical = _stricmp(GetDisplayName().c_str(), a_rhs.GetDisplayName().c_str());
				   alphabetical != 0) {
			return alphabetical < 0 ? -1 : 1;
		} else if (GetFormID() != a_rhs.GetFormID()) {
			return GetFormID() < a_rhs.GetFormID() ? -1 : 1;
		} else {
			return 0;
		}
	}

	const std::string& GFxItem::GetDisplayName() const
	{
		if (comp_installed) {
			comp_response = std::nullopt;

			if (auto* messageInterface = SKSE::GetMessagingInterface()) {
				CompletionistRequestEE request{ GetFormID() };
				messageInterface->Dispatch(1, &request, sizeof(request), "Completionist");
				logger::info("Completionist is installed, Message Sent For - {}"sv, GetFormID());

				if (comp_response && comp_response->m_formId == GetFormID() && comp_response->m_displayname != "") {
					logger::info("Completionist Message Receieved With Matching FormID and Valid Name");
					return comp_response->m_displayname;
				}
			}
		}

		if (_cache[kDisplayName]) {
			return _cache.DisplayName();
		}

		std::string result;
		switch (_src.index()) {
		case kInventory: 
		{
			const char* display_name = std::get<kInventory>(_src)->GetDisplayName();
			result = display_name ? display_name : ""sv;
			break;
		}
		case kGround:
			result = ""sv;
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				if (item && item->GetDisplayFullName()) {
					result = item->GetDisplayFullName();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.DisplayName(std::move(result));
		return _cache.DisplayName();
	}

	double GFxItem::GetEnchantmentCharge() const
	{
		if (_cache[kEnchantmentCharge]) {
			return _cache.EnchantmentCharge();
		}

		double result = -1.0;
		switch (_src.index()) {
		case kInventory:
			result =
				std::get<kInventory>(_src)
					->GetEnchantmentCharge()
					.value_or(-1.0);
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				if (item) {
					const auto charge = item->GetEnchantmentCharge();
					if (charge) {
						result = charge.value_or(-1.0);
						break;
					}
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.EnchantmentCharge(result);
		return result;
	}

	bool GFxItem::IsEnchanted() const
	{
		if (!_cache[kIsEnchanted]) {
			SetupEnchantmentFlags();
		}
		return _cache.IsEnchanted();
	}

	bool GFxItem::IsKnownEnchanted() const
	{
		if (!_cache[kIsKnownEnchanted]) {
			SetupEnchantmentFlags();
		}
		return _cache.IsKnownEnchanted();
	}

	bool GFxItem::IsSpecialEnchanted() const
	{
		if (!_cache[kIsSpecialEnchanted]) {
			SetupEnchantmentFlags();
		}
		return _cache.IsSpecialEnchanted();
	}

	RE::TESForm* GFxItem::GetObject() const
	{
		switch (_src.index()) {
		case kInventory:
			return std::get<kInventory>(_src)->GetObject();
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					return obj;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		return nullptr;
	}

	RE::FormID GFxItem::GetFormID() const
	{
		if (_cache[kFormID]) {
			return _cache.FormID();
		}

		auto result = std::numeric_limits<RE::FormID>::max();
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->GetFormID();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->GetFormID();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.FormID(result);
		return result;
	}

	kType GFxItem::GetItemType() const
	{
		if (_cache[kItemType]) {
			return _cache.ItemType();
		}

		auto result = kType::None;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = GetItemType(obj);
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = GetItemType(obj);
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.ItemType(result);
		return result;
	}

	std::ptrdiff_t GFxItem::GetValue() const
	{
		if (_cache[kValue]) {
			return _cache.Value();
		}

		auto result = std::numeric_limits<std::ptrdiff_t>::min();
		switch (_src.index()) {
		case kInventory:
			result = std::get<kInventory>(_src)->GetValue() * _count;
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->GetGoldValue() * _count;
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Value(result);
		return result;
	}

	double GFxItem::GetWeight() const
	{
		if (_cache[kWeight]) {
			return _cache.Weight();
		}

		double result = 0.0;
		switch (_src.index()) {
		case kInventory:
			result = std::get<kInventory>(_src)->GetWeight() * _count;
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->GetWeight() * _count;
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Weight(result);
		return result;
	}

	RE::SOUL_LEVEL GFxItem::GetSoulSize() const
	{
		RE::SOUL_LEVEL result = RE::SOUL_LEVEL::kNone;
		const RE::InventoryEntryData* item_inventory_entry = nullptr;
		const RE::TESForm* item_form = nullptr;
		RE::TESObjectREFRPtr item_refr;

		switch (_src.index()) {
		case kInventory:
		{
			item_inventory_entry = std::get<kInventory>(_src);
			item_form = item_inventory_entry ? item_inventory_entry->GetObject() : nullptr;
			break;
		}
		case kGround:
		{
			for (const auto& handle : std::get<kGround>(_src)) {
				if (!handle.get())
					continue;
				item_refr = handle.get();
				item_form = item_refr.get();
			}
			break;
		}
		default:
			assert(false);
		}

		if (item_inventory_entry) {
			result = item_inventory_entry->GetSoulLevel();
		} else {
			auto extraSoul = item_refr ? item_refr->extraList.GetByType<RE::ExtraSoul>() : nullptr;
			if (extraSoul) {
				result = extraSoul->GetContainedSoul();
			} else {
				auto soulGem = item_form ? item_form->As<RE::TESSoulGem>() : nullptr;
				if (soulGem) {
					result = soulGem->GetContainedSoul();
				}
			}
		}

		return result;
	}

	bool GFxItem::IsAmmo() const
	{
		if (_cache[kAmmo]) {
			return _cache.Ammo();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->IsAmmo();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->IsAmmo();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Ammo(result);
		return result;
	}

	bool GFxItem::IsBook() const
	{
		if (_cache[kBook]) {
			return _cache.Book();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->IsBook();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->IsBook();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Book(result);
		return result;
	}

	bool GFxItem::IsRead() const
	{
		if (!IsBook()) {
			return false;
		}

		if (_cache[kIsRead]) {
			return _cache.IsRead();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = static_cast<RE::TESObjectBOOK*>(std::get<kInventory>(_src)->GetObject()); obj) {
				result = obj->IsRead();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? static_cast<RE::TESObjectBOOK*>(item->GetObjectReference()) : nullptr;
				if (obj) {
					result = obj->IsRead();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.IsRead(result);
		return result;
	}

	bool GFxItem::IsGold() const
	{
		if (_cache[kGold]) {
			return _cache.Gold();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->IsGold();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->IsGold();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Gold(result);
		return result;
	}

	bool GFxItem::IsKey() const
	{
		if (_cache[kKey]) {
			return _cache.Key();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->IsKey();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->IsKey();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Key(result);
		return result;
	}

	bool GFxItem::IsLockpick() const
	{
		if (_cache[kLockpick]) {
			return _cache.Lockpick();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->IsLockpick();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->IsLockpick();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Lockpick(result);
		return result;
	}

	bool GFxItem::IsNote() const
	{
		if (_cache[kNote]) {
			return _cache.Note();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			if (const auto obj = std::get<kInventory>(_src)->GetObject(); obj) {
				result = obj->IsNote();
			}
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				const auto obj = item ? item->GetObjectReference() : nullptr;
				if (obj) {
					result = obj->IsNote();
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.Note(result);
		return result;
	}

	bool GFxItem::IsQuestItem() const
	{
		if (_cache[kQuestItem]) {
			return _cache.QuestItem();
		}

		bool result = false;
		switch (_src.index()) {
		case kInventory:
			result = std::get<kInventory>(_src)->IsQuestObject();
			break;
		case kGround:
			for (const auto& handle : std::get<kGround>(_src)) {
				const auto item = handle.get();
				if (item && item->HasQuestObject()) {
					result = true;
					break;
				}
			}
			break;
		default:
			assert(false);
			break;
		}

		_cache.QuestItem(result);
		return result;
	}

	bool GFxItem::IsStolen() const
	{
		if (_cache[kStolen]) {
			return _cache.Stolen();
		}

		bool result = false;
		auto player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			switch (_src.index()) {
			case kInventory:
				result = !std::get<kInventory>(_src)->IsOwnedBy(player, !_stealing);
				break;
			case kGround:
				for (const auto& handle : std::get<kGround>(_src)) {
					const auto item = handle.get();
					if (item && item->IsCrimeToActivate()) {
						result = true;
						break;
					}
				}
				break;
			default:
				assert(false);
				break;
			}
		}

		_cache.Stolen(result);
		return result;
	}

	bool GFxItem::ItemIsNew() const
	{
		if (_cache[kIsDBMNew]) {
			return _cache.IsDBMNew();
		}

		bool result = LOTD::IsItemNew(GetFormID());
		_cache.IsDBMNew(result);
		return result;
	}

	bool GFxItem::ItemIsFound() const
	{
		if (_cache[kIsDBMFound]) {
			return _cache.IsDBMFound();
		}

		bool result = LOTD::IsItemFound(GetFormID());
		_cache.IsDBMFound(result);
		return result;
	}

	bool GFxItem::ItemIsDisplayed() const
	{
		if (_cache[kIsDBMDisplayed]) {
			return _cache.IsDBMDisplayed();
		}

		bool result = LOTD::IsItemDisplayed(GetFormID());
		_cache.IsDBMDisplayed(result);
		return result;
	}

	RE::GFxValue GFxItem::GFxValue(RE::GFxMovieView& a_view) const
	{
		RE::GFxValue value;
		a_view.CreateObject(std::addressof(value));
		auto obj = GetObject();
		auto formType = obj ? obj->GetFormType() : RE::FormType::None;
		value.SetMember("formType", formType);
		value.SetMember("formId", obj ? obj->GetFormID() : 0);

		switch (formType) {
		case RE::FormType::Armor:
		{
			RE::TESObjectARMO* armor = skyrim_cast<RE::TESObjectARMO*>(obj);
			if (armor) {
				value.SetMember("partMask", armor->bipedModelData.bipedObjectSlots.underlying());
				value.SetMember("weightClass", armor->bipedModelData.armorType.underlying());
			}
			break;
		}
		case RE::FormType::Ammo:
		{
			RE::TESAmmo* ammo = skyrim_cast<RE::TESAmmo*>(obj);
			if (ammo) {
				value.SetMember("flags", ammo->data.flags.underlying());
			}
			break;
		}
		case RE::FormType::Weapon:
		{
			RE::TESObjectWEAP* weapon = skyrim_cast<RE::TESObjectWEAP*>(obj);
			if (weapon) {
				value.SetMember("subType", weapon->weaponData.animationType.underlying());
				value.SetMember("weaponType", weapon->weaponData.animationType.underlying());
				value.SetMember("speed", weapon->weaponData.speed);
				value.SetMember("reach", weapon->weaponData.reach);
				value.SetMember("stagger", weapon->weaponData.staggerValue);
				value.SetMember("critDamage", weapon->criticalData.damage);
				value.SetMember("minRange", weapon->weaponData.minRange);
				value.SetMember("maxRange", weapon->weaponData.maxRange);
				value.SetMember("baseDamage", weapon->attackDamage);
				RE::BGSEquipSlot* equipSlot = weapon->equipSlot;
				if (equipSlot) {
					value.SetMember("equipSlot", equipSlot->formID);
				}
			}
			break;
		}
		case RE::FormType::SoulGem:
		{
			RE::TESSoulGem* soulGem = skyrim_cast<RE::TESSoulGem*>(obj);
			if (soulGem) {
				RE::SOUL_LEVEL currentSoul = GetSoulSize();
				value.SetMember("gemSize", soulGem->soulCapacity.underlying());
				value.SetMember("soulSize", currentSoul);
				if (currentSoul == RE::SOUL_LEVEL::kNone) {
					value.SetMember("status", 0);
				} else if (currentSoul >= soulGem->soulCapacity) {
					value.SetMember("status", 2);
				} else {
					value.SetMember("status", 1);
				}
			}
			break;
		}
		case RE::FormType::AlchemyItem:
		{
			RE::AlchemyItem* alchemy = skyrim_cast<RE::AlchemyItem*>(obj);
			if (alchemy) {
				value.SetMember("flags", alchemy->data.flags.underlying());
			}
			break;
		}
		case RE::FormType::Book:
		{
			RE::TESObjectBOOK* book = skyrim_cast<RE::TESObjectBOOK*>(obj);
			if (book) {
				value.SetMember("flags", book->data.flags.underlying());
				value.SetMember("bookType", book->data.type.underlying());
				if (book->data.flags.all(RE::OBJ_BOOK::Flag::kAdvancesActorValue)) {
					value.SetMember("teachesSkill", book->data.teaches.actorValueToAdvance);
				} else if (book->data.flags.all(RE::OBJ_BOOK::Flag::kTeachesSpell)) {
					double formID = -1;
					if (auto spell = book->data.teaches.spell; spell) {
						formID = spell->GetFormID();
					}

					value.SetMember("teachesSpell", formID);
				}
			}
			break;
		}
		}

		value.SetMember("displayName", { static_cast<std::string_view>(GetDisplayName()) });
		value.SetMember("count", { _count });
		value.SetMember("stolen", { IsStolen() });
		value.SetMember("value", { GetValue() });
		value.SetMember("iconLabel", { GetItemIconLabel(GetItemType()) });

		const auto weight = GetWeight();
		if (weight >= 0)
			value.SetMember("weight", { GetWeight() });

		if (Settings::ShowEnchanted()) {
			value.SetMember("enchanted", { IsEnchanted() });
			value.SetMember("knownEnchanted", { IsKnownEnchanted() });
			value.SetMember("specialEnchanted", { IsSpecialEnchanted() });
		}

		if (Settings::ShowDBMNew())
			value.SetMember("dbmNew", { ItemIsNew() });

		if (Settings::ShowDBMFound())
			value.SetMember("dbmFound", { ItemIsFound() });

		if (Settings::ShowDBMDisplayed())
			value.SetMember("dbmDisp", { ItemIsDisplayed() });

		if (Settings::ShowBookRead())
			value.SetMember("isRead", { IsRead() });

		return value;
	}

	static kType GetItemTypeWeapon(TESObjectWEAP* weap)
	{
		kType type = kType::DefaultWeapon;

		switch (weap->GetWeaponType()) {
		case RE::WEAPON_TYPE::kOneHandSword:
			type = kType::WeaponSword;
			break;
		case RE::WEAPON_TYPE::kOneHandDagger:
			type = kType::WeaponDagger;
			break;
		case RE::WEAPON_TYPE::kOneHandAxe:
			type = kType::WeaponWarAxe;
			break;
		case RE::WEAPON_TYPE::kOneHandMace:
			type = kType::WeaponMace;
			break;
		case RE::WEAPON_TYPE::kTwoHandSword:
			type = kType::WeaponGreatSword;
			break;
		case RE::WEAPON_TYPE::kTwoHandAxe:
			type = kType::WeaponBattleAxe;
			break;
		case RE::WEAPON_TYPE::kBow:
			type = kType::WeaponBow;
			break;
		case RE::WEAPON_TYPE::kStaff:
			type = kType::WeaponStaff;
			break;
		case RE::WEAPON_TYPE::kCrossbow:
			type = kType::WeaponCrossbow;
			break;
		}

		return type;
	}

	static kType GetItemTypeArmor(TESObjectARMO* armor)
	{
		static kType types[] = {
			kType::LightArmorBody,  // 0
			kType::LightArmorHead,
			kType::LightArmorHands,
			kType::LightArmorForearms,
			kType::LightArmorFeet,
			kType::LightArmorCalves,
			kType::LightArmorShield,
			kType::LightArmorMask,

			kType::ArmorBody,  // 8
			kType::ArmorHead,
			kType::ArmorHands,
			kType::ArmorForearms,
			kType::ArmorFeet,
			kType::ArmorCalves,
			kType::ArmorShield,
			kType::ArmorMask,

			kType::ClothingBody,  // 16
			kType::ClothingHead,
			kType::ClothingHands,
			kType::ClothingForearms,
			kType::ClothingFeet,
			kType::ClothingCalves,
			kType::ClothingShield,
			kType::ClothingMask,

			kType::ArmorAmulet,  // 24
			kType::ArmorRing,
			kType::Circlet,

			kType::DefaultArmor  // 27
		};

		UINT32 index = 0;

		if (armor->IsLightArmor()) {
			index = 0;
		} else if (armor->IsHeavyArmor()) {
			index = 8;
		} else {
			if (armor->HasKeywordID(0x08F95B)) {  // VendorItemClothing
				index = 16;
			} else if (armor->HasKeywordID(0x08F95A)) {  // VendorItemJewelry
				if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kAmulet))
					index = 24;
				else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kRing))
					index = 25;
				else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kCirclet))
					index = 26;
				else
					index = 27;
			} else {
				index = 27;
			}
		}

		if (index >= 24)
			return types[index];

		if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kBody) || armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kTail))
			index += 0;  // body
		else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kHead) || armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kHair) || armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kLongHair)) {
			index += 1;  // head
			if (armor->formID >= 0x061C8B && armor->formID < 0x061CD7)
				index += 6;  // mask
		} else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kHands))
			index += 2;  // hands
		else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kForearms))
			index += 3;  // forarms
		else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kFeet))
			index += 4;  // forarms
		else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kCalves))
			index += 5;  // calves
		else if (armor->HasPartOf(BGSBipedObjectForm::BipedObjectSlot::kShield))
			index += 6;  // shield
		else
			index = 27;

		return types[index];
	}

	static kType GetItemTypePotion(AlchemyItem* potion)
	{
		kType type = kType::DefaultPotion;

		if (potion->IsFood()) {
			type = kType::DefaultFood;

			const static UINT32 ITMPosionUse = 0x000B6435;
			if (potion->data.consumptionSound && potion->data.consumptionSound->formID == ITMPosionUse)
				type = kType::FoodWine;
		} else if (potion->IsPoison()) {
			type = kType::PotionPoison;
		} else {
			type = kType::DefaultPotion;

			//MagicItem::EffectItem* pEffect = CALL_MEMBER_FN(potion, GetCostliestEffectItem)(5, false);
			Effect* pEffect = potion->GetCostliestEffectItem(RE::MagicSystem::Delivery::kTotal, false);
			if (pEffect && pEffect->baseEffect) {
				ActorValue primaryValue = pEffect->baseEffect->GetMagickSkill();

				if (primaryValue == ActorValue::kNone) {
					primaryValue = pEffect->baseEffect->data.primaryAV;
				}

				switch (primaryValue) {
				case ActorValue::kHealth:
					type = kType::PotionHealth;
					break;
				case ActorValue::kMagicka:
					type = kType::PotionMagic;
					break;
				case ActorValue::kStamina:
					type = kType::PotionStam;
					break;
				case ActorValue::kResistFire:
					type = kType::PotionFire;
					break;
				case ActorValue::kResistShock:
					type = kType::PotionShock;
					break;
				case ActorValue::kResistFrost:
					type = kType::PotionFrost;
					break;
				}
			}
		}

		return type;
	}

	static kType GetItemTypeMisc(TESObjectMISC* misc)
	{
		kType type = kType::DefaultMisc;

		static const UINT32 LockPick = 0x00000A;
		static const UINT32 Gold = 0x00000F;
		static const UINT32 Leather01 = 0x000DB5D2;
		static const UINT32 LeatherStrips = 0x000800E4;

		static const UINT32 VendorItemAnimalHideFormId = 0x0914EA;
		static const UINT32 VendorItemDaedricArtifactFormId = 0x000917E8;
		static const UINT32 VendorItemGemFormId = 0x000914ED;
		static const UINT32 VendorItemToolFormId = 0x000914EE;
		static const UINT32 VendorItemAnimalPartFormId = 0x000914EB;
		static const UINT32 VendorItemOreIngotFormId = 0x000914EC;
		static const UINT32 VendorItemClutterFormId = 0x000914E9;
		static const UINT32 VendorItemFirewoodFormId = 0x000BECD7;

		static const UINT32 RubyDragonClaw = 0x04B56C;
		static const UINT32 IvoryDragonClaw = 0x0AB7BB;
		static const UINT32 GlassCraw = 0x07C260;
		static const UINT32 EbonyCraw = 0x05AF48;
		static const UINT32 EmeraldDragonClaw = 0x0ED417;
		static const UINT32 DiamondClaw = 0x0AB375;
		static const UINT32 IronClaw = 0x08CDFA;
		static const UINT32 CoralDragonClaw = 0x0B634C;
		static const UINT32 E3GoldenClaw = 0x0999E7;
		static const UINT32 SapphireDragonClaw = 0x0663D7;
		static const UINT32 MS13GoldenClaw = 0x039647;

		if (misc->formID == LockPick)
			type = kType::MiscLockPick;
		else if (misc->formID == Gold)
			type = kType::MiscGold;
		else if (misc->formID == Leather01)
			type = kType::MiscLeather;
		else if (misc->formID == LeatherStrips)
			type = kType::MiscStrips;
		else if (misc->HasKeywordID(VendorItemAnimalHideFormId))
			type = kType::MiscHide;
		else if (misc->HasKeywordID(VendorItemDaedricArtifactFormId))
			type = kType::MiscArtifact;
		else if (misc->HasKeywordID(VendorItemGemFormId))
			type = kType::MiscGem;
		else if (misc->HasKeywordID(VendorItemAnimalPartFormId))
			type = kType::MiscRemains;
		else if (misc->HasKeywordID(VendorItemOreIngotFormId))
			type = kType::MiscIngot;
		else if (misc->HasKeywordID(VendorItemClutterFormId))
			type = kType::MiscClutter;
		else if (misc->HasKeywordID(VendorItemFirewoodFormId))
			type = kType::MiscWood;
		else if (misc->formID == RubyDragonClaw || misc->formID == IvoryDragonClaw || misc->formID == GlassCraw || misc->formID == EbonyCraw || misc->formID == EmeraldDragonClaw || misc->formID == DiamondClaw || misc->formID == IronClaw || misc->formID == CoralDragonClaw || misc->formID == E3GoldenClaw || misc->formID == SapphireDragonClaw || misc->formID == MS13GoldenClaw)
			type = kType::MiscDragonClaw;

		return type;
	}

	static kType GetItemTypeSoulGem(TESSoulGem* gem)
	{
		kType type = kType::MiscSoulGem;

		const static UINT32 DA01SoulGemAzurasStar = 0x063B27;
		const static UINT32 DA01SoulGemBlackStar = 0x063B29;

		if (gem->formID == DA01SoulGemBlackStar || gem->formID == DA01SoulGemAzurasStar) {
			type = kType::SoulGemAzura;
		} else {
			if (gem->GetMaximumCapacity() < SOUL_LEVEL::kGrand) {
				if (gem->GetContainedSoul() == SOUL_LEVEL::kNone)
					type = kType::SoulGemEmpty;
				else if (gem->GetContainedSoul() >= gem->GetMaximumCapacity())
					type = kType::SoulGemFull;
				else
					type = kType::SoulGemPartial;
			} else {
				if (gem->GetContainedSoul() == SOUL_LEVEL::kNone)
					type = kType::SoulGemGrandEmpty;
				else if (gem->GetContainedSoul() >= gem->GetMaximumCapacity())
					type = kType::SoulGemGrandFull;
				else
					type = kType::SoulGemGrandPartial;
			}
		}

		return type;
	}

	const kType GetItemTypeBook(TESObjectBOOK* book)
	{
		kType type = kType::DefaultBook;

		const static UINT32 VendorItemRecipeFormID = 0x000F5CB0;
		const static UINT32 VendorItemSpellTomeFormID = 0x000937A5;

		if (book->data.type.underlying() == 0xFF || book->HasKeywordID(VendorItemRecipeFormID)) {
			type = kType::BookNote;
		} else if (book->HasKeywordID(VendorItemSpellTomeFormID)) {
			type = kType::BookTome;
		}

		return type;
	}
	kType GFxItem::GetItemType(TESForm* form) const
	{
		kType type = kType::None;

		switch (form->formType.get()) {
		case FormType::Scroll:
			type = kType::DefaultScroll;
			break;
		case FormType::Armor:
			type = GetItemTypeArmor(static_cast<TESObjectARMO*>(form));
			break;
		case FormType::Book:
			type = GetItemTypeBook(static_cast<TESObjectBOOK*>(form));
			break;
		case FormType::Ingredient:
			type = kType::DefaultIngredient;
			break;
		case FormType::Light:
			type = kType::MiscTorch;
			break;
		case FormType::Misc:
			type = GetItemTypeMisc(static_cast<TESObjectMISC*>(form));
			break;
		case FormType::Weapon:
			type = GetItemTypeWeapon(static_cast<TESObjectWEAP*>(form));
			break;
		case FormType::Ammo:
			type = (static_cast<TESAmmo*>(form)->IsBolt()) ? kType::WeaponBolt : kType::WeaponArrow;
			break;
		case FormType::KeyMaster:
			type = kType::DefaultKey;
			break;
		case FormType::AlchemyItem:
			type = GetItemTypePotion(static_cast<AlchemyItem*>(form));
			break;
		case FormType::SoulGem:
			type = GetItemTypeSoulGem(static_cast<TESSoulGem*>(form));
			break;
		}

		return type;
	}

	// Straight up copied from MoreHudSE. https://github.com/ahzaab/moreHUDSE
	static bool MagicDisallowEnchanting(const RE::BGSKeywordForm* pKeywords)
	{
		if (pKeywords) {
			for (uint32_t k = 0; k < pKeywords->numKeywords; k++) {
				if (pKeywords->keywords[k]) {
					auto keyword = pKeywords->GetKeywordAt(k).value_or(nullptr);
					if (keyword) {
						// Had to add this check because https://www.nexusmods.com/skyrimspecialedition/mods/34175?
						// sets the editor ID for 'MagicDisallowEnchanting' to null (╯°□°）╯︵ ┻━┻
						auto asCstr = keyword->GetFormEditorID();
						std::string keyWordName = asCstr ? asCstr : "";
						if (keyWordName == "MagicDisallowEnchanting") {
							return true;  // Is enchanted, but cannot be enchanted by player
						}
					}
				}
			}
		}
		return false;
	}

	EnchantmentType GFxItem::GetEnchantmentType() const
	{
		EnchantmentType result = EnchantmentType::None;
		const RE::InventoryEntryData* item_inventory_entry = nullptr;
		const RE::TESForm* item_form = nullptr;
		RE::TESObjectREFR* item_refr = nullptr;

		switch (_src.index()) {
		case kInventory:
		{
			item_inventory_entry = std::get<kInventory>(_src);
			item_form = item_inventory_entry ? item_inventory_entry->GetObject() : nullptr;
			break;
		}
		case kGround:
		{
			for (const auto& handle : std::get<kGround>(_src)) {
				if (!handle.get())
					continue;
				item_refr = handle.get().get();
				item_form = item_refr;
			}
			break;
		}
		default:
			assert(false);
		};

		if (!item_form) 
		{
			return result;
		}

		const auto item_form_type = item_form->GetFormType();

		if (item_form_type != RE::FormType::Weapon 
			&& item_form_type != RE::FormType::Armor 
			&& item_form_type != RE::FormType::Ammo 
			&& item_form_type != RE::FormType::Projectile)
		{
			return result;
		}

		RE::EnchantmentItem* enchantment = nullptr;
		auto keyWordForm = item_form->As<RE::BGSKeywordForm>();
		auto enchantable = item_form->As<RE::TESEnchantableForm>();

		bool wasExtra = false;
		if (enchantable) {  // Check the item for a base enchantment
			enchantment = enchantable->formEnchanting;
		}

		if (item_refr) {
			if (auto extraEnchant = static_cast<RE::ExtraEnchantment*>(item_refr->extraList.GetByType(RE::ExtraDataType::kEnchantment))) {
				wasExtra = true;
				enchantment = extraEnchant->enchantment;
			}
		}

		if (enchantment) {
			// It has an enchantment be it may not be known.
			result = EnchantmentType::Unknown;

			if ((enchantment->formFlags & RE::TESForm::RecordFlags::kKnown) == RE::TESForm::RecordFlags::kKnown) {
				return MagicDisallowEnchanting(enchantment) ? EnchantmentType::CannotDisenchant : EnchantmentType::Known;
			} 
			
			if (MagicDisallowEnchanting(enchantment)) {
				return EnchantmentType::CannotDisenchant;
			} 
			
			auto baseEnchantment = static_cast<RE::EnchantmentItem*>(enchantment->data.baseEnchantment);
			if (baseEnchantment) {
				if ((baseEnchantment->formFlags & RE::TESForm::RecordFlags::kKnown) == RE::TESForm::RecordFlags::kKnown) {
					return MagicDisallowEnchanting(baseEnchantment) ? EnchantmentType::CannotDisenchant : EnchantmentType::Known;
				} 
				
				if (MagicDisallowEnchanting(baseEnchantment)) {
					return EnchantmentType::CannotDisenchant;
				}
			}
		}

		// Its safe to assume that if it not a base enchanted item, that it was enchanted by the player and therefore, they
		// know the enchantment
		if (wasExtra) {
			return EnchantmentType::Known;
		} 
		
		if (enchantable) {
			return MagicDisallowEnchanting(keyWordForm) ? EnchantmentType::CannotDisenchant : result;
		}

		return result;
	}

	// Almost straight up copied from MoreHudSE. Had to change some things to work with this. https://github.com/ahzaab/moreHUDSE
	void GFxItem::SetupEnchantmentFlags() const
	{
		EnchantmentType ench_type = GetEnchantmentType();

		_cache.IsEnchanted(ench_type != EnchantmentType::None);
		_cache.IsKnownEnchanted(ench_type == EnchantmentType::Known);
		_cache.IsSpecialEnchanted(ench_type == EnchantmentType::CannotDisenchant);
	}

	const char* GFxItem::GetItemIconLabel(kType form) const
	{
		size_t form_num = static_cast<size_t>(form);
		if (form_num < sizeof(strIcons) / sizeof(strIcons[0]))
			return strIcons[form_num];

		return strIcons[0];
	}
}

namespace Completionist_Integration
{
	void RegisterListener()
	{
		if (WinAPI::GetModuleHandleA("Completionist")) {
			logger::info("Completionist is installed, attempting to register listener"sv);
			auto* messageInterface = SKSE::GetMessagingInterface();
			comp_installed = messageInterface->RegisterListener("Completionist", CompletionistResponse);
		}

		if (comp_installed) {
			logger::info("Completionist listener registered successfully"sv);
			return;
		}

		logger::info("Unable to register Completionist listener"sv);
	}

	void CompletionistResponse(SKSE::MessagingInterface::Message* a_msg)
	{
		logger::info("Recieved message from Completionist.");

		if (!a_msg) {
			logger::info("message is Null");
			return;
		}

		if (a_msg->type != 2) {
			logger::info("message type is not 2");
			return;
		}

		if (!a_msg->data) {
			logger::info("message has no data");
			return;
		}

		comp_response = *static_cast<CompletionistResponseEE*>(a_msg->data);

		if (comp_response.has_value()) {
			logger::info("completionist messagge received with data: {}, {}", std::to_string(comp_response.value().m_formId), comp_response.value().m_displayname);
		}
	}
}
