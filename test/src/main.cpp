#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <razoom.save_level_data_api/include/SaveLevelDataApi.hpp>


using namespace geode::prelude;


class $modify(EditorUI) {

	bool init(LevelEditorLayer* editorLayer) {
		
		if (!EditorUI::init(editorLayer)) return false;

		// saving value
		SaveLevelDataAPI::setSavedValue(
			m_editorLayer->m_level,	// GJGameLevel*
			"my-value-1",    		// key
			90000000,				// value (can be any json-serializable type)
			true, 					// save to the save file (default: true)
			true					// save to the text object inside the level (default: false)
		);

		// getting value
		geode::Result<matjson::Value> result = SaveLevelDataAPI::getSavedValue(
			m_editorLayer->m_level,	// GJGameLevel*
			"my-value-1",    		// key
			true,					// get value from the save file (default: true)
			true					// get value from the text object if wasn't found in the save file (default: false)
		);

		int value = result.unwrapOrDefault().asInt().unwrapOr(0);
		log::info("value: {}", value); // value: 90000000

		return true;
	}
};


