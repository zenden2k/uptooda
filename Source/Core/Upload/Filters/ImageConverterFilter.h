#ifndef IU_CORE_IMAGECONVERTERFILTER_H
#define IU_CORE_IMAGECONVERTERFILTER_H

#include "Core/Upload/UploadFilter.h"
#include "Core/Images/ImageConverter.h"
#include "Core/Upload/ServerProfile.h"

class ImageConverterFilter : public UploadFilter
{
public:
    bool PreUpload(UploadTask* task) override;
    bool PostUpload(UploadTask* task) override;

    static bool supposedOutputFormat(SupposedFormat& fileFormat, ServerProfile serverProfile, const ImageUploadParams& defaultImageUploadParams);
};
#endif
