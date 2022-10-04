#include <fstream>
#include <llapi/LoggerAPI.h>

#define JSON2(key1,key2,val)                                \
if (json.find(key1) != json.end()) {                      \
    if (json.at(key1).find(key2) != json.at(key1).end()) {\
    const nlohmann::json& out2 = json.at(key1).at(key2);  \
         out2.get_to(val);}}                              \

#define JSON1(key,val)                                         \
if (json.find(key) != json.end()) {                          \
    const nlohmann::json& out = json.at(key);                \
    out.get_to(val);}                                         \

namespace Settings {

    int memberRateLimit = 60;
    namespace DownloadImg {
        bool allowmember = false;
    }
    namespace LocalImg {
        bool allowmember = false;
    }
    namespace ImgSize {
		int maxWidth = 1408;
		int maxHeight = 1408;
        int maxFileSize = 15;
    }

    nlohmann::json globaljson() {
        nlohmann::json json;
        json["MemberRateLimit"] = memberRateLimit;
        json["DownloadImg"]["Allow-Member"] = DownloadImg::allowmember;
        json["LocalImg"]["Allow-Member"] = LocalImg::allowmember;
        json["ImgSize"]["maxWidth"] = ImgSize::maxWidth;
        json["ImgSize"]["maxHeight"] = ImgSize::maxHeight;
        json["ImgSize"]["maxFileSize"] = ImgSize::maxFileSize;
        return json;
    }

    void initjson(nlohmann::json json) {
        JSON1("MemberRateLimit", memberRateLimit);
        JSON2("DownloadImg", "Allow-Member", DownloadImg::allowmember);
        JSON2("LocalImg", "Allow-Member", LocalImg::allowmember);
        JSON2("ImgSize", "maxWidth", ImgSize::maxWidth);
        JSON2("ImgSize", "maxHeight", ImgSize::maxHeight);
        JSON2("ImgSize", "maxFileSize", ImgSize::maxFileSize);
    }

    void WriteDefaultConfig(const std::string& fileName) {
        std::ofstream file(fileName);
        if (!file.is_open()) {
            Logger("CustomMapX").error("Can't open file ",fileName);
            return;
        }
        auto json = globaljson();
        file << json.dump(4);
        file.close();
    }

    void LoadConfigFromJson(const std::string& fileName) {
        std::ifstream file(fileName);
        if (!file.is_open()) {
            Logger("CustomMapX").error("Can't open file ", fileName);
            return;
        }
        nlohmann::json json;
        file >> json;
        file.close();
        initjson(json);
        WriteDefaultConfig(fileName);
    }

    void reloadJson(const std::string& fileName) {
        std::ofstream file(fileName);
        if (file)
        {
            file << globaljson().dump(4);
        }
        else
        {
            Logger("CustomMapX").error("Configuration File Creation failed!");
        }
        file.close();
    }
} // namespace Settings


