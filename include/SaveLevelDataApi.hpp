#pragma once

#include <Geode/Geode.hpp>

#ifdef GEODE_IS_WINDOWS
    #ifdef RAZOOM_SAVE_LEVEL_DATA_API_EXPORTING
        #define SAVE_LEVEL_DATA__API_DLL __declspec(dllexport)
    #else
        #define SAVE_LEVEL_DATA__API_DLL __declspec(dllimport)
    #endif
#else
    #define SAVE_LEVEL_DATA__API_DLL __attribute__((visibility("default")))
#endif


class SAVE_LEVEL_DATA__API_DLL SaveLevelDataAPI {
public:

    static void setSavedValue(
        GJGameLevel* level, 
        std::string_view key, 
        matjson::Value const &value, 
        bool saveInSaveFile = true,
        bool saveInTextObject = false,
        geode::Mod* mod = geode::getMod()
    );

    static geode::Result<matjson::Value> getSavedValue(
        GJGameLevel* level, 
        std::string_view key, 
        bool checkSaveFile = true,
        bool checkTextObject = false,
        geode::Mod* mod = geode::getMod()
    );

};

