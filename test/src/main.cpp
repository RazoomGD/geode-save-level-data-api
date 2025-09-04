#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <razoom.save_level_data_api/include/SaveLevelDataApi.hpp>


using namespace geode::prelude;


class $modify(EditorUI) {

	bool init(LevelEditorLayer* editorLayer) {

		// getting value
		geode::Result<matjson::Value> result = SaveLevelDataAPI::getSavedValue(
			editorLayer->m_level,
			"my-value-1",
			false,
			true
		);

		int value = result.unwrapOrDefault().asInt().unwrapOr(0);
		log::info("old value (obj): {}", value);


		result = SaveLevelDataAPI::getSavedValue(
			editorLayer->m_level,
			"my-value-1",
			true,
			false
		);

		value = result.unwrapOrDefault().asInt().unwrapOr(0);
		log::info("old value (file): {}", value);
		
		if (!EditorUI::init(editorLayer)) return false;

		// saving value
		SaveLevelDataAPI::setSavedValue(
			editorLayer->m_level,
			"my-value-1",
			70000000,
			true,
			true
		);

		// getting value
		result = SaveLevelDataAPI::getSavedValue(
			editorLayer->m_level,
			"my-value-1",
			true,
			true
		);

		value = result.unwrapOrDefault().asInt().unwrapOr(0);
		log::info("value: {}", value);

		return true;
	}
};


