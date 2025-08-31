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

	void setVal(matjson::Value &mainObj, std::string_view key1, std::string_view key2, matjson::Value val) {
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
	struct Fields {
		Ref<TextGameObject> m_saveObject = nullptr;
	};

	bool init(GJGameLevel* p0, bool p1) {
		if (!LevelEditorLayer::init(p0, p1)) return false;
		if (m_objects) {
			for (int i = 0; i < m_objects->count(); i++) {
				auto obj = static_cast<GameObject*>(m_objects->objectAtIndex(i));
				if (obj->m_objectID != 914) continue;
				auto textObj = static_cast<TextGameObject*>(obj);
				auto object = matjson::parse(textObj->m_text).unwrapOrDefault();
				if (object.contains("save_level_data_api")) {
					m_fields->m_saveObject = textObj;
				}
			}
		}
		return true;
	}

	void touchTextObject() {
		if (m_fields->m_saveObject == nullptr || m_fields->m_saveObject->getParent() == nullptr) {
			short tmp = m_currentLayer;
			m_currentLayer = -1;
			m_fields->m_saveObject = static_cast<TextGameObject*>(createObject(914, ccp(0,0), true));
			m_currentLayer = tmp;
			removeObjectFromSection(m_fields->m_saveObject);
			m_fields->m_saveObject->updateTextObject("{\"save_level_data_api\": {}}", false);
		}
		// todo
		m_fields->m_saveObject->setPosition(ccp(0,0));
		m_fields->m_saveObject->setRotation(0.f);
		m_fields->m_saveObject->setScale(1.f);
	}
	

	template <class T>
	void saveValue(std::string_view modId, std::string_view key, T const &value) {
		touchTextObject();
		matjson::Value mainObj = matjson::parse(m_fields->m_saveObject->m_text).unwrapOrDefault()["save_level_data_api"];
		if (!mainObj.isObject()) mainObj = matjson::Value();
		json_utils::setVal(mainObj, modId, key, matjson::Value(value));
		m_fields->m_saveObject->updateTextObject(matjson::makeObject({{"save_level_data_api", mainObj}}).dump(), false);
	}


	template <class T>
	geode::Result<T> getSavedValue(std::string_view modId, std::string_view key) {
		touchTextObject();
		matjson::Value mainObj = matjson::parse(m_fields->m_saveObject->m_text).unwrapOrDefault()["save_level_data_api"];
		if (!mainObj.isObject()) return Err("Text object seems to be corrupted");
		auto result = json_utils::getVal(mainObj, modId, key);
		if (result.isErr()) return Err("Not found");
		return result.unwrap().template as<T>();
	}


	void removeSaveObjectDuplicates() {
		if (m_objects && m_fields->m_saveObject) {
			for (int i = 0; i < m_objects->count(); i++) {
				auto obj = static_cast<GameObject*>(m_objects->objectAtIndex(i));
				if (obj->m_objectID != 914) continue;
				auto textObj = static_cast<TextGameObject*>(obj);
				auto object = matjson::parse(textObj->m_text).unwrapOrDefault();
				if (object.contains("save_level_data_api")) {
					if (textObj != m_fields->m_saveObject) {
						removeSpecial(textObj);
						log::warn("Removed duplicated save object");
					}
				}
			}
		}
	}
};


class $modify(EditorPauseLayer) {
	void saveLevel() {
		reinterpret_cast<SaveLevelDataApiLEL*>(m_editorLayer)->removeSaveObjectDuplicates();
		EditorPauseLayer::saveLevel();
	}
};


template <class T>
void SaveLevelDataAPI::setSavedValue(
	GJGameLevel* level,
	std::string_view key,
	T const &value,
	bool saveInTextObject,
	bool saveInSaveFile,
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
		json_utils::setVal(levels, levelId, key, matjson::Value(value));
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
		reinterpret_cast<SaveLevelDataApiLEL*>(lel)->saveValue(mod->getID(), key, value);
	}
};


template <class T>
geode::Result<T> SaveLevelDataAPI::getSavedValue(
	GJGameLevel* level,
	std::string_view key,
	bool checkTextObject,
	bool checkSaveFile,
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
		auto result = json_utils::getVal(levels, levelId, key);
		if (result.isOk()) {
			return result.unwrap().template as<T>();
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
		return reinterpret_cast<SaveLevelDataApiLEL*>(lel)->template getSavedValue<T>(mod->getID(), key);
	}

	return Err("Not found");
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





