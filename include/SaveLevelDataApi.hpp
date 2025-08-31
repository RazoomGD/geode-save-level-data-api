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

    template <class T>
    static void setSavedValue(
        GJGameLevel* level, 
        std::string_view key, 
        T const &value, 
        bool saveInTextObject = false,
        bool saveInSaveFile = true,
        geode::Mod* mod = geode::getMod()
    );

    template <class T>
    static geode::Result<T> getSavedValue(
        GJGameLevel* level, 
        std::string_view key, 
        bool checkTextObject = false,
        bool checkSaveFile = true,
        geode::Mod* mod = geode::getMod()
    );

};

