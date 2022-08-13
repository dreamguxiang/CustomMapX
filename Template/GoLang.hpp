#pragma once
#include "godef.h"

namespace GolangFunc {
    namespace FuncDef {
        typedef GoSlice<char> (*png2PixelArr)(GoString);
        typedef GoSlice<char>(*getUrlPngData)(GoString);
    } // namespace FuncDef
    FuncDef::png2PixelArr png2PixelArr;
	FuncDef::getUrlPngData getUrlPngData;
} // namespace GolangFunc