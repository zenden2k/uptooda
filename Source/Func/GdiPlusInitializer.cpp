#include "GdiPlusInitializer.h"

#include "3rdpart/GdiplusH.h"
#include <stdexcept>

GdiPlusInitializer::GdiPlusInitializer()
	:token_(0)
{
	Gdiplus::GdiplusStartupInput input;
	//Gdiplus::GdiplusStartupOutput output;
	if(Gdiplus::Ok != Gdiplus::GdiplusStartup(&token_, &input, nullptr))
		throw std::runtime_error("Unable to initialize GDI+");
}

GdiPlusInitializer::~GdiPlusInitializer()
{
	Gdiplus::GdiplusShutdown(token_);
}
