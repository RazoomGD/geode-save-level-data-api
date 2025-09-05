#include <Geode/Geode.hpp>
#include "../include/SaveLevelDataApi.hpp"
#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <matjson.hpp>

#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>


using namespace geode::prelude;


namespace json_utils {
	Result<matjson::Value> getVal(matjson::Value const &mainObj, std::string_view key1, std::string_view key2) {
		if (mainObj.contains(key1)) {
			auto v1 = mainObj.get(key1).unwrap();
			if (v1.isObject() && v1.contains(key2)) {
				return Ok(v1.get(key2).unwrap());
			}
		}
		return Err("Not found");
	}

	void setVal(matjson::Value &mainObj, std::string_view key1, std::string_view key2, matjson::Value const &val) {
		if (mainObj.contains(key1)) {
			auto v1 = mainObj.get(key1).unwrap();
			if (!v1.isObject()) {
				mainObj.erase(key1);
			}
		}
		mainObj[key1][key2] = val;
	}
};


class $modify(SaveLevelDataApiLEL, LevelEditorLayer) {

	static void onModify(auto& self) {
		// first after original
		if (!self.setHookPriority("LevelEditorLayer::createObjectsFromSetup", 50000)) {
			log::warn("Failed to set hook priority");
		}
	}


	struct Fields {
		matjson::Value m_saveObject{};
		bool m_textObjectLoaded = false;
	};


	void createObjectsFromSetup(gd::string& p0) {
		LevelEditorLayer::createObjectsFromSetup(p0);
		if (!m_fields->m_textObjectLoaded) {
			loadTextObject();
			m_fields->m_textObjectLoaded = true;
		}
	}

	
	void saveValue(std::string_view modId, std::string_view key, matjson::Value const &value) {
		if (!m_fields->m_textObjectLoaded) {
			log::error("Objects aren't loaded yet! Can't save the value!");
			return;
		}
		json_utils::setVal(m_fields->m_saveObject, modId, key, value);
	}
	
	
	geode::Result<matjson::Value> getSavedValue(std::string_view modId, std::string_view key) {
		if (!m_fields->m_textObjectLoaded) {
			log::error("Objects aren't loaded yet! Can't get saved value!");
			return Err("Not loaded yet!");
		}
		return json_utils::getVal(m_fields->m_saveObject, modId, key);
	}


	void loadTextObject() {
		if (!m_objects) return;
		for (int i = 0; i < m_objects->count(); i++) {
			auto obj = static_cast<GameObject*>(m_objects->objectAtIndex(i));
			if (obj->m_objectID != 914) continue;
			auto textObject = static_cast<TextGameObject*>(obj);

			if (textObject->m_text.size() < 2) continue;
			if (textObject->m_text.at(0) != '{') continue;

			auto parsedJson = matjson::parse(textObject->m_text);
			if (parsedJson.isErr()) continue;
			auto body = parsedJson.unwrap().get("save_level_data_api");
			if (body.isErr() || !body.unwrap().isObject()) continue;

			m_fields->m_saveObject = body.unwrap();

			if (Mod::get()->getSettingValue<bool>("debug-mode")) {
				textObject->setPosition(ccp(0, 0));
				textObject->setRotation(0.f);
				textObject->setScale(0.5f);
				auto keyCount = m_fields->m_saveObject.size();
				log::info("Text object successfully loaded! Found saved values of {} mod(s)", keyCount);
			} else {
				textObject->setPosition(ccp(-9999, -9999));
				textObject->setRotation(0.f);
				textObject->setScale(0.1f);
			}
			break;
		}
	}


	void writeTextObject() {
		if (!m_objects) return;

		TextGameObject* textObject = nullptr;

		auto toRemove = CCArray::create();
		for (int i = 0; i < m_objects->count(); i++) {
			auto obj = static_cast<GameObject*>(m_objects->objectAtIndex(i));
			if (obj->m_objectID != 914) continue;
			auto object = matjson::parse(static_cast<TextGameObject*>(obj)->m_text).unwrapOrDefault();
			if (object.contains("save_level_data_api")) {
				if (textObject) {
					toRemove->addObject(obj); // remove duplicates
				} else {
					textObject = static_cast<TextGameObject*>(obj);
				}
			}
		}

		for (int i = 0; i < toRemove->count(); i++) {
			auto obj = static_cast<GameObject*>(toRemove->objectAtIndex(i));
			removeObject(obj, true); // true - noUndo
		}

		if (!textObject) {
			if (m_fields->m_saveObject.size() == 0) return;
			short tmp = m_currentLayer;
			m_currentLayer = -1;
			textObject = static_cast<TextGameObject*>(createObject(914, ccp(0,0), true));
			m_currentLayer = tmp;
			if (!textObject) return;
		}

		if (m_fields->m_saveObject.size() == 0) {
			removeObject(textObject, true); // true - noUndo
			return;
		}

		if (Mod::get()->getSettingValue<bool>("debug-mode")) {
			textObject->setPosition(ccp(0, 0));
			textObject->setRotation(0.f);
			textObject->setScale(0.5f);
			log::info("Text object successfully saved!");
		} else {
			textObject->setPosition(ccp(-9999, -9999));
			textObject->setRotation(0.f);
			textObject->setScale(0.1f);
		}

		textObject->updateTextObject(
			matjson::makeObject(
				{{"save_level_data_api", m_fields->m_saveObject}}
			).dump(), false
		);
	}
};


class $modify(EditorPauseLayer) {
	static void onModify(auto& self) {
		// last before original
		if (!self.setHookPriority("EditorPauseLayer::saveLevel", 50000)) {
			log::warn("Failed to set hook priority");
		}
	}
	void saveLevel() {
		reinterpret_cast<SaveLevelDataApiLEL*>(m_editorLayer)->writeTextObject();
		EditorPauseLayer::saveLevel();
	}
};


class $modify(GJGameLevel) {
	// copy saved level data with level info
	void copyLevelInfo(GJGameLevel* oldLvl) {
		GJGameLevel::copyLevelInfo(oldLvl);

		if (!oldLvl) return;

		auto newId = std::to_string(EditorIDs::getID(this));
		auto oldId = std::to_string(EditorIDs::getID(oldLvl));

		for (auto mod : Loader::get()->getAllMods()) {
			if (!mod->depends("razoom.save_level_data_api")) continue;
			
			auto levels = mod->getSavedValue<matjson::Value>("save_level_data_api");
			if (!levels.isObject()) continue;

			auto oldVal = levels.get(oldId);
			if (oldVal.isOk()) {
				levels[newId] = oldVal.unwrap();
				mod->setSavedValue("save_level_data_api", levels);
			} else {
				if (levels.contains(newId)) {
					levels[newId] = matjson::Value();
					mod->setSavedValue("save_level_data_api", levels);
				}
			}
		}
	}
};


void SaveLevelDataAPI::setSavedValue(
	GJGameLevel* level,
	std::string_view key,
	matjson::Value const &value,
	bool saveInSaveFile,
	bool saveInTextObject,
	geode::Mod* mod
) {
	// checks
	if (!level) {
		log::error("SaveLevelDataAPI::setSavedValue - GJGameLevel* is nullptr");
		return;
	}
	if (!mod) {
		log::error("SaveLevelDataAPI::setSavedValue - geode::Mod* is nullptr");
		return;
	}
	if (!saveInTextObject && !saveInSaveFile) {
		log::warn("SaveLevelDataAPI::setSavedValue - saveInTextObject and saveInSaveFile are both false. Nowhere to save!");
		return;
	}

	// save in mod save file
	if (saveInSaveFile) {
		auto levelId = std::to_string(EditorIDs::getID(level));
		matjson::Value levels = mod->getSavedValue<matjson::Value>("save_level_data_api");
		if (!levels.isObject()) levels = matjson::Value();
		json_utils::setVal(levels, levelId, key, value);
		mod->setSavedValue("save_level_data_api", levels);
	}

	// save to text object
	if (saveInTextObject) {
		auto lel = LevelEditorLayer::get();
		if (lel == nullptr) {
			log::error("SaveLevelDataAPI::setSavedValue - You can't use 'saveInTextObject' option if LevelEditorLayer::get() == nullptr");
			log::error("But if you REALLY need it, I can add this functionality. (ask me in GitHub issues)");
			return;
		}
		if (lel->m_level != level) {
			log::error("SaveLevelDataAPI::setSavedValue - You can't use 'saveInTextObject' option for a level that is not opened in the editor");
			log::error("LevelEditorLayer::get()->m_level != level");
			log::error("But if you REALLY need it, I can add this functionality. (ask me in GitHub issues)");
			return;
		}
		reinterpret_cast<SaveLevelDataApiLEL*>(lel)->saveValue(mod->getID(), key, value);
	}
};


geode::Result<matjson::Value> SaveLevelDataAPI::getSavedValue(
	GJGameLevel* level,
	std::string_view key,
	bool checkSaveFile,
	bool checkTextObject,
	geode::Mod* mod
) {
	// checks
	if (!level) {
		log::error("SaveLevelDataAPI::getSavedValue - GJGameLevel* is nullptr");
		return Err("GJGameLevel* is nullptr");
	}
	if (!mod) {
		log::error("SaveLevelDataAPI::getSavedValue - geode::Mod* is nullptr");
		return Err("mod* is nullptr");
	}
	if (!checkTextObject && !checkSaveFile) {
		log::warn("SaveLevelDataAPI::getSavedValue - checkTextObject and checkSaveFile are both false. Nothing to check!");
		return Err("Nothing to check");
	}

	// get from save file
	if (checkSaveFile) {
		auto levelId = std::to_string(EditorIDs::getID(level));
		matjson::Value levels = mod->getSavedValue<matjson::Value>("save_level_data_api");
		if (!levels.isObject()) return Err("save_file_corrupted");
		auto res = json_utils::getVal(levels, levelId, key);
		if (res.isOk()) {
			return res;
		}
	}

	// check text object
	if (checkTextObject) {
		auto lel = LevelEditorLayer::get();
		if (lel == nullptr) {
			log::error("SaveLevelDataAPI::getSavedValue - You can't use 'checkTextObject' option if LevelEditorLayer::get() == nullptr");
			log::error("But if you REALLY need it, I can add this functionality. (ask me in GitHub issues)");
			return Err("LevelEditorLayer::get() is nullptr");
		}
		if (lel->m_level != level) {
			log::error("SaveLevelDataAPI::getSavedValue - You can't use 'checkTextObject' option for a level that is not opened in the editor");
			log::error("LevelEditorLayer::get()->m_level != level");
			log::error("But if you REALLY need it, I can add this functionality. (ask me in GitHub issues)");
			return Err("LevelEditorLayer::get()->m_level != level");
		}
		auto res = reinterpret_cast<SaveLevelDataApiLEL*>(lel)->getSavedValue(mod->getID(), key);
		if (res.isOk()) {
			return res;
		}
	}

	return Err("Not found");
};




