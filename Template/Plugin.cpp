#include <iostream>
#include <Global.h>
#include <EventAPI.h>
#include <LoggerAPI.h>
#include <MC/Level.hpp>
#include <MC/BlockInstance.hpp>
#include <MC/Block.hpp>
#include <MC/BlockSource.hpp>
#include <MC/Actor.hpp>
#include <MC/Player.hpp>
#include <MC/ItemStack.hpp>
#include "Version.h"
#include <LLAPI.h>
#include <ServerAPI.h>
#include <DynamicCommandAPI.h>
#include <ScheduleAPI.h>
#include <Utils/StringHelper.h>
Logger logger(PLUGIN_NAME);

inline void getAllFiles(std::string strPath, std::vector<std::string>& vecFiles)
{
	char cEnd = *strPath.rbegin();
	if (cEnd == '\\' || cEnd == '/')
	{
		strPath = strPath.substr(0, strPath.length() - 1);
	}
	if (strPath.empty() || strPath == (".") || strPath == (".."))
		return;
	std::error_code ec;
	std::filesystem::path fsPath(strPath);
	if (!std::filesystem::exists(strPath, ec)) {
		return;
	}
	for (auto& itr : std::filesystem::directory_iterator(fsPath))
	{
		if (std::filesystem::is_directory(itr.status()))
		{
			getAllFiles(UTF82String(itr.path().u8string()), vecFiles);
		}
		else
		{
			vecFiles.push_back(UTF82String(itr.path().filename().u8string()));
		}
	}
}
namespace mce{
	
class Blob {
public:
	void* unk0;
	std::unique_ptr<unsigned char[]> buffer;
	size_t length = 0;


	inline Blob() {}
	inline Blob(Blob&& rhs) : buffer(std::move(rhs.buffer)), length(rhs.length) { rhs.length = 0; }
	inline Blob(size_t input_length) : buffer(std::make_unique<unsigned char[]>(input_length)), length(input_length) {}
	inline Blob(unsigned char const* input, size_t input_length) : Blob(input_length) {
		memcpy(buffer.get(), input, input_length);
	}

	inline Blob& operator=(Blob&& rhs) {
		if (&rhs != this) {
			buffer = std::move(rhs.buffer);
			length = rhs.length;
			rhs.length = 0;
		}
		return *this;
	}

	inline Blob clone() const { return { data(), size() }; }

	inline unsigned char* begin() { return buffer.get(); }
	inline unsigned char* end() { return buffer.get() + length; }
	inline unsigned char const* cbegin() const { return buffer.get(); }
	inline unsigned char const* cend() const { return buffer.get() + length; }

	inline unsigned char* data() { return buffer.get(); }
	inline unsigned char const* data() const { return buffer.get(); }

	inline bool empty() const { return length == 0; }
	inline size_t size() const { return length; }

	inline auto getSpan() const { return gsl::make_span(data(), size()); }

};

static_assert(sizeof(Blob) == 24);

enum class ImageFormat {
	NONE = 0,
	RGB = 1,
	RGBA = 2
};

enum class ImageUsage : int8_t {
	unknown = 0,
	sRGB = 1,
	data = 2
};

inline unsigned numChannels(ImageFormat format) {
	switch (format) {
	case ImageFormat::RGB:  return 3;
	case ImageFormat::RGBA: return 4;
	default:                return 0;
	}
}

class Image {
	inline Image(ImageFormat format, unsigned width, unsigned height, ImageUsage usage, Blob&& data)
		: format(format), width(width), height(height), usage(usage), data(std::move(data)) {}

public:
	ImageFormat format{}; // 0x0
	unsigned width{}, height{}; // 0x4, 0x8
	ImageUsage usage{}; // 0xC
	Blob data; // 0x10

	inline Image(Blob&& data) : data(std::move(data)) {}
	inline Image(unsigned width, unsigned height, ImageFormat format, ImageUsage usage)
		: format(format), width(width), height(height), usage(usage) {}
	inline Image() {}

	inline Image& operator=(Image&& rhs) {
		format = rhs.format;
		width = rhs.width;
		height = rhs.height;
		usage = rhs.usage;
		data = std::move(rhs.data);
		return *this;
	}

	inline Image clone() const { return { format, width, height, usage, data.clone() }; }

	inline void copyRawImage(Blob const& blob) { data = blob.clone(); }

	inline bool isEmpty() const { return data.empty(); }

	inline void resizeImageBytesToFitImageDescription() { data = Blob{ width * height * numChannels(format) }; }

	inline void setImageDescription(unsigned width, unsigned height, ImageFormat format, ImageUsage usage) {
		this->width = width;
		this->height = height;
		this->format = format;
		this->usage = usage;
	}

	inline void setRawImage(Blob&& buffer) { data = std::move(buffer); }

	Image(const Image& a1) {
		format = a1.format;
		width = a1.width;
		height = a1.height;
		usage = a1.usage;
		data = a1.data.clone();
	}
};

static_assert(offsetof(Image, data) == 0x10);
static_assert(offsetof(Image, format) == 0x0);
static_assert(offsetof(Image, usage) == 0xC);
static_assert(sizeof(Image) == 40);


}; // namespace mce

class Image2D {
public:
	vector<mce::Color> rawColor;
	
	unsigned width = 0, height = 0;

	Image2D(unsigned w, unsigned h, vector<mce::Color> c) : width(w), height(h) , rawColor(c) {
		
	}
};

#include "../Header/FreeImage/FreeImage.h"
#include "lodepng.h"
#include <Global.h>
namespace Helper {
	void jpg2png(const std::string& srcPath, const std::string& outPath) {

		FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
		FIBITMAP* dib(nullptr);
		fif = FreeImage_GetFileType(srcPath.c_str(), 0);
		if (fif == FIF_UNKNOWN)
			fif = FreeImage_GetFIFFromFilename(srcPath.c_str());
		if (fif == FIF_UNKNOWN)
			return;
		if (FreeImage_FIFSupportsReading(fif))
			dib = FreeImage_Load(fif, srcPath.c_str());
		if (!dib)
			return;
		FIBITMAP* dib32 = FreeImage_ConvertTo32Bits(dib);
		FIBITMAP* dibResized = FreeImage_Rescale(dib32, FreeImage_GetWidth(dib), FreeImage_GetHeight(dib), FILTER_CATMULLROM);
		FreeImage_Unload(dib);
		FreeImage_Save(FIF_PNG, dibResized, outPath.c_str());
		FreeImage_Unload(dibResized);
		FreeImage_Unload(dib32);
		FreeImage_DeInitialise();
	}
	
	std::tuple<std::vector<unsigned char>, unsigned, unsigned> Png2Pix(string path) {
		std::vector<unsigned char> png, image;
		unsigned w, h;
		jpg2png(path, path);
		unsigned error = lodepng::load_file(png,path);
		if (!error) error = lodepng::decode(image, w, h, png);

		if (error) {
			logger.warn("Failed to load skin image(2): " + path);
			return {};
		}
		return std::make_tuple(image, w, h);
	}
	
	vector<Image2D> CuttingImages(std::vector<mce::Color> image, int width, int height) {
		vector<Image2D> images;
		auto wcut = width / 128;
		auto hcut = height / 128;
		if (width % 128 != 0) {
			wcut++;
		}

		if (height % 128 != 0) {
			hcut++;
		}
		for(auto h = 0; h < hcut; h++ ){
			for (auto w = 0; w < wcut; w++) {
				vector<mce::Color> img;
				for (auto a = h * 128; a < h * 128 + 128; a++) {
					for (auto  b = w * 128; b< w * 128 + 128; b++) {
						if (a * width + b < image.size() &&  b * height + a < image.size()) {
							img.push_back(image[a * width + b]);
						}
						else {
							img.push_back(mce::Color(0xff, 0xff, 0xff, 0));
						}
					}
				}
				images.push_back(Image2D(128, 128, img));
			}
		}
		return images;
	}
}
#include <MC/MapItem.hpp>
#include <MC/ServerPlayer.hpp>
#include <MC/Container.hpp>
void RegCommand()
{
    using ParamType = DynamicCommand::ParameterType;

	vector<string> out;
	getAllFiles(".\\plugins\\CustomMapX\\picture", out);

    auto command = DynamicCommand::createCommand("map", "custommap", CommandPermissionLevel::GameMasters);

	auto& MapEnum = command->setEnum("MapEnum", { "reload","help"});
	auto& MapAddEnum = command->setEnum("MapAddEnum", { "add" });
	
    command->mandatory("MapsEnum", ParamType::Enum, MapEnum, CommandParameterOption::EnumAutocompleteExpansion);
	command->mandatory("MapsEnum", ParamType::Enum, MapAddEnum, CommandParameterOption::EnumAutocompleteExpansion);
    command->mandatory("MapSoftEnum", ParamType::SoftEnum, command->setSoftEnum("MapENameList", out));
	
    command->addOverload({ MapAddEnum,"MapSoftEnum"});
	command->addOverload({ MapEnum });

	command->setCallback([](DynamicCommand const& command, CommandOrigin const& origin, CommandOutput& output, std::unordered_map<std::string, DynamicCommand::Result>& results) {
		auto action = results["MapsEnum"].get<std::string>();
		string str = "";
		ServerPlayer* sp = origin.getPlayer();
			switch (do_hash(action.c_str()))
			{
			case do_hash("add"): {
				if (sp) {
					auto picfile = results["MapSoftEnum"].get<std::string>();
					if (!picfile.empty()) {
						auto [data, w, h] = Helper::Png2Pix(".\\plugins\\CustomMapX\\picture\\" + picfile);
						if (data.size() == 0) return;
						vector<mce::Color> Colorlist;
						for (int y = 0; y < h; y++)
						{
							for (int x = 0; x < w; x++)
							{
								mce::Color img((float)data[(y * w + x) * 4 + 0] / 255, (float)data[(y * w + x) * 4 + 1] / 255, (float)data[(y * w + x) * 4 + 2] / 255, (float)data[(y * w + x) * 4 + 3] / 255);
								Colorlist.push_back(img);
							}
						}
						auto datalist = Helper::CuttingImages(Colorlist, w, h);
						int xtemp = 0;
						int ytemp = 0;
						for (auto data : datalist) {
							auto mapitem = ItemStack::create("minecraft:filled_map");
							auto MapIndex = sp->getMapIndex();
							sp->setMapIndex(MapIndex + 1);
							MapItem::setMapNameIndex(*mapitem, MapIndex);
							auto& mapdate = Global<Level>->_createMapSavedData(MapIndex);
							mapdate.setLocked();
							for (int x = 0; x < 128; x++)
								for (int y = 0; y < 128; y++) {
									mapdate.setPixel(data.rawColor[y + x * 128].toABGR(), y, x);
								}
							mapdate.save(*Global<LevelStorage>);
							MapItem::setItemInstanceInfo(*mapitem, mapdate);
							auto sizetest = sqrt(datalist.size());
							mapitem->setCustomName(picfile + "-" + std::to_string(xtemp) + "_" + std::to_string(ytemp));
							Level::spawnItem(sp->getPos(), sp->getDimensionId(), mapitem);
							delete mapitem;
							ytemp++;
							if (ytemp == sizetest) {
								xtemp++;
								ytemp = 0;
							}
						}
						output.success("§l§6[CustomMapX] §aAdd Map Success!(" + std::to_string(datalist.size()) + ")");
					}
				}
				break;
			}
			case do_hash("reload"): {
				vector<string> out;
				getAllFiles(".\\plugins\\CustomMapX\\picture", out);
				command.getInstance()->setSoftEnum("MapENameList", out);
				break;
			}
			case do_hash("help"): {
				output.success(
					"§l§e>§6CustomMapX§e<\n"
					"§b/map add §l§a<mapfile> §l§gAdd maps\n"
					"§b/map reload §l§gRefresh picture path\n"
					"§b/map help\n"
					"§l§e>§6CustomMapX§e<");
				break;
			}
			default:
				break;
			}
     });
	DynamicCommand::setup(std::move(command));
}

void PluginInit()
{
	logger.info("Loaded");
	if (!std::filesystem::exists("plugins/CustomMapX"))
		std::filesystem::create_directories("plugins/CustomMapX");
	if (!std::filesystem::exists("plugins/CustomMapX/picture"))
		std::filesystem::create_directories("plugins/CustomMapX/picture");
	RegCommand();
}

inline vector<string> split(string a1,char a) {
	vector<string> out;
	std::stringstream ss(a1);
	string temp;
	while (getline(ss, temp, a)) {
		out.push_back(temp);
	}
	return out;
}


bool isNextImage1(string name,string newname) {
	
	auto oldoutlist = split(name,'-');
	auto newoutlist = split(newname, '-');
	if (oldoutlist.empty() || newoutlist.empty()) return false;
	if (newname.find(oldoutlist[0]) != newname.npos) {
		auto oldnumlist = split(oldoutlist[1], '_');
		auto newnumlist = split(newoutlist[1], '_');

		auto oldfir = atoi(oldnumlist[0].c_str());
		auto oldsec = atoi(oldnumlist[1].c_str());

		auto newfir = atoi(newnumlist[0].c_str());
		auto newsec = atoi(newnumlist[1].c_str());
		if (oldsec + 1 == newsec) {
			if (oldfir == newfir) {
				return true;
			}
		}
	}
	return false;
}

bool isNextImage2(string name, string newname) {

	auto oldoutlist = split(name, '-');
	auto newoutlist = split(newname, '-');
	if (oldoutlist.empty() || newoutlist.empty()) return false;
	if (newname.find(oldoutlist[0]) != newname.npos) {
		auto oldnumlist = split(oldoutlist[1], '_');
		auto newnumlist = split(newoutlist[1], '_');

		auto oldfir = atoi(oldnumlist[0].c_str());
		auto oldsec = atoi(oldnumlist[1].c_str());

		auto newfir = atoi(newnumlist[0].c_str());
		auto newsec = atoi(newnumlist[1].c_str());
		if(oldfir+1 == newfir) {
			if (newsec == 0) {
				return true;
			}
		}
	}
	return false;
}

bool UseItemSupply(Player* sp, ItemStackBase& item, string itemname, short aux) {
	auto& plinv = sp->getSupplies();
	auto slotnum = dAccess<int, 16>(&plinv);
	auto& uid = sp->getUniqueID();
	if (item.getCount() == 0) {
		auto& inv = sp->getInventory();
		bool isgive = 0;
		for (int i = 0; i <= inv.getSize(); i++) {
			auto& item = inv.getItem(i);
			if (!item.isNull()) {
				if (item.getItem()->getSerializedName() == "minecraft:filled_map") {
					if (i == slotnum) continue;
					bool isgive = 0;
					if (isNextImage1(itemname, item.getCustomName())) {
						isgive = 1;
					}
					if (isgive) {
						auto snbt = const_cast<ItemStack*>(&item)->getNbt()->toSNBT();

			
						Schedule::delay([snbt, uid, slotnum, i] {
							auto newitem = ItemStack::create(CompoundTag::fromSNBT(snbt));
							auto sp = Global<Level>->getPlayer(uid);
							if (sp) {
								if (sp->getHandSlot()->isNull()) {
									auto& inv = sp->getInventory();
									inv.setItem(i, ItemStack::EMPTY_ITEM);
									auto& plinv = sp->getSupplies();
									inv.setItem(slotnum, *newitem);
									sp->refreshInventory();
								}
							}
							delete newitem;
							}, 1);
					}
					
				}
			}
		}
		Schedule::delay([isgive, uid, slotnum, itemname] {
			auto sp = Global<Level>->getPlayer(uid);
			auto& inv = sp->getInventory();
			if (!isgive) {
				for (int i = 0; i <= inv.getSize(); i++) {
					auto& item = inv.getItem(i);
					if (!item.isNull()) {
						if (item.getItem()->getSerializedName() == "minecraft:filled_map") {

							if (i == slotnum) continue;
							bool isgive2 = 0;
							if (isNextImage2(itemname, item.getCustomName())) {
								isgive2 = 1;
							}
							if (isgive2) {
								auto snbt = const_cast<ItemStack*>(&item)->getNbt()->toSNBT();

								auto& uid = sp->getUniqueID();
								Schedule::delay([snbt, uid, slotnum, i] {
								auto newitem = ItemStack::create(CompoundTag::fromSNBT(snbt));
								auto sp = Global<Level>->getPlayer(uid);
								if (sp) {
									if (sp->getHandSlot()->isNull()) {
										auto& inv = sp->getInventory();
										inv.setItem(i, ItemStack::EMPTY_ITEM);
										auto& plinv = sp->getSupplies();
										inv.setItem(slotnum, *newitem);
										sp->refreshInventory();
									}
								}
								delete newitem;
								}, 1);
							}

						}
					}
				}
			}
			},1);
	}
}


TInstanceHook(void, "?useItem@Player@@UEAAXAEAVItemStackBase@@W4ItemUseMethod@@_N@Z", Player, ItemStackBase& item, int a2, bool a3)
{
	auto itemname = item.getCustomName();
	auto itemname2 = item.getItem()->getSerializedName();
	auto aux = item.getAuxValue();
	original(this, item, a2, a3);
	try {
		if (itemname2 == "minecraft:filled_map") {
			UseItemSupply(this, item, itemname, aux);
		}
	}
	catch (...) {
		return;
	}
}

