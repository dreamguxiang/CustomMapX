#pragma once
#include "godef.h"

namespace GolangFunc {
    namespace FuncDef {
        typedef GoSlice<char> (*png2PixelArr)(GoString path);
        typedef GoInt32 (*getPngWidth)(GoString path);
		typedef GoInt32 (*getPngHeight)(GoString path);
    } // namespace FuncDef
    FuncDef::png2PixelArr png2PixelArr;
	FuncDef::getPngWidth getPngWidth;
	FuncDef::getPngHeight getPngHeight;
} // namespace GolangFunc