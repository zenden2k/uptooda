#ifndef IU_CORE_IMAGES_ABSTRACTIMAGE_H_
#define IU_CORE_IMAGES_ABSTRACTIMAGE_H_

#include <functional>
#include <string>

#include "Core/Utils/CoreTypes.h"
#include <typeinfo> 

#ifdef IU_QT
    class QtImage;
#elif defined(_WIN32)
    class GdiPlusImage;
#endif

class AbstractImage
{
public:
    enum DataFormat { dfRGB888,
        dfBitmapRgb,
        dfRGB32bpp };
    AbstractImage();
    virtual ~AbstractImage() = default;
    virtual bool loadFromFile(const std::string& fileName);
    virtual bool saveToFile(const std::string& fileName) const = 0;
    virtual int getWidth() const;
    virtual int getHeight() const;
    virtual bool isNull() const = 0;
    virtual bool loadFromRawData(DataFormat dt, int width, int height, uint8_t* data, size_t dataSize, void* parameter) = 0;
    std::string getSrcFormat() const;
    template<class T> static void registerFactory(const char* name) {
        factory_ = [] () -> AbstractImage* {
            return new T();
        };
        factoryName_ = name;
    }

    template<class T> static void autoRegisterFactory() {
        #ifdef IU_QT
            registerFactory<QtImage>("QtImage");
        #elif defined(_WIN32)
            registerFactory<GdiPlusImage>("GdiPlusImage");
        #endif
    }
    static const std::string& factoryName();
    static AbstractImage* createImage();
    void setSrcFormat(const std::string& str);

protected:
    static std::function<AbstractImage*()> factory_;
    static std::string factoryName_;
    int width_;
    int height_;
    std::string srcFormat_;

    struct FactoryRegistrar {
        FactoryRegistrar() {
            autoRegisterFactory<void>();
        }
    };
    static FactoryRegistrar factoryRegistrar_;
    DISALLOW_COPY_AND_ASSIGN(AbstractImage);
};

#ifdef IU_QT
    #include "Video/QtImage.h"
#elif defined(_WIN32)
    #include "Core/Images/GdiPlusImage.h"
#endif


#endif
