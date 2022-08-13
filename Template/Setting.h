#pragma once
#include "Global.h"
#include <Nlohmann/json.hpp>


namespace Settings {
    namespace DownloadImg {
        extern bool allowmember;
    }
    namespace LocalImg {
        extern bool allowmember;
    }


    nlohmann::json globaljson();
    void initjson(nlohmann::json json);
    void WriteDefaultConfig(const std::string& fileName);
    void LoadConfigFromJson(const std::string& fileName);
    void reloadJson(const std::string& fileName);
}