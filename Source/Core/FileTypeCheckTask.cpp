#include "FileTypeCheckTask.h"

#include <boost/format.hpp>
#include <utility>

#include "Core/i18n/Translator.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Upload/Filters/ImageConverterFilter.h"

constexpr auto MAX_BAD_ITEMS = std::numeric_limits<size_t>::max();

FileTypeCheckTask::FileTypeCheckTask(IFileList* fileList, ServerProfileGroup sessionImageServer, ServerProfileGroup sessionFileServer, ImageUploadParams defaultImageUploadParams) :
    fileList_(fileList)
    , sessionImageServer_(std::move(sessionImageServer))
    , sessionFileServer_(std::move(sessionFileServer))
    , defaultImageUploadParams_(std::move(defaultImageUploadParams)) {

}

BackgroundTaskResult FileTypeCheckTask::doJob()
{
    message_.clear();
    errors_.clear();
    auto* settings = ServiceLocator::instance()->basicSettings();

    size_t n = fileList_->getFileCount();
    size_t badItems = 0;
    onProgress(this, 0, n, _("Checking file formats..."));

    for (size_t i = 0; i < n; i++) {
        if (isCanceled()) {
            return BackgroundTaskResult::Canceled;
        }
        onProgress(this, i, n, _("Checking file formats..."));
        auto* item = fileList_->getFile(i);
        std::string utf8Name = item->getFileName();
        
        int64_t size = IuCoreUtils::GetFileSize(utf8Name);
        ServerProfileGroup& profileGroup = item->isImage() ? sessionImageServer_ : sessionFileServer_;
        std::string mimeType;

        mimeType = IuCoreUtils::GetFileMimeType(utf8Name);
        item->setMimeType(mimeType);
        
        for (const auto& serverProfile : profileGroup.getItems()) {
            SupposedFormat sf;
            sf.fileName = utf8Name;
            sf.mimeType = mimeType;
            
            
            if (item->isImage()) {
                ImageConverterFilter::supposedOutputFormat(sf, serverProfile, defaultImageUploadParams_);
            }
            if (sf.fileSize < 0) {
                sf.fileSize = size;
            }
            std::string onlyName = IuCoreUtils::ExtractFileName(sf.fileName);
            std::string extension = IuCoreUtils::ExtractFileExt(sf.fileName);

            CUploadEngineData* uploadEngineData = serverProfile.uploadEngineData();
            ServerSettingsStruct* sss = settings->getServerSettings(serverProfile, false);
            bool isAuthorized = !serverProfile.profileName().empty() && sss && sss->authData.DoAuth && !sss->authData.Login.empty();
            std::string userType { isAuthorized ? UserTypes::REGISTERED : UserTypes::ANONYMOUS };
            if (uploadEngineData && !uploadEngineData->supportsFileFormat(IuStringUtils::ToLower(onlyName), sf.mimeType, sf.fileSize, userType)) {
                badItems++;
                if (badItems > MAX_BAD_ITEMS) {
                    message_ += "\n..."; 
                    goto exitLoop;
                }
                BadFileFormat bff;
                bff.uploadProfile = &serverProfile;
                bff.fileName = onlyName;
                bff.mimeType = sf.mimeType;
                bff.fileSize = sf.fileSize;
                bff.index = i;
                bff.message = str(IuStringUtils::FormatNoExcept(extension.empty() ?
                    _("\"%1%\": server %2% doesn't support this type of file (%3% %4%)\n")
                    : _("\"%1%\": server %2% doesn't support this type of file (%3% %4% with '.%5%' extension)\n"))
                    % onlyName % uploadEngineData->Name % sf.mimeType % IuCoreUtils::FileSizeToString(size) % extension);
                errors_.push_back(std::move(bff));
            }
        }
    }

exitLoop:

    onProgress(this, n, n, "");

    if (isCanceled()) {
        return BackgroundTaskResult::Canceled;
    }

    return BackgroundTaskResult::Success;
}


std::string FileTypeCheckTask::message() const {
    return message_;
}

std::vector<BadFileFormat>& FileTypeCheckTask::errors() {
    return errors_;
}
