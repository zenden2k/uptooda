#include "DefaultLogger.h"

#include <boost/format.hpp>
#include "atlheaders.h"
#include "Gui/Dialogs/LogWindow.h"

thread_local bool DefaultLogger::insideWriteFunction_  = false;

void DefaultLogger::write(LogMsgType MsgType, const std::string& Sender, const std::string& Msg, const std::string& Info, const std::string&  FileName) {
    if (insideWriteFunction_) { // Prevent recursion
        return;
    }
    insideWriteFunction_ = true;
    defer d([&] { // Run at function exit
        insideWriteFunction_ = false;
    });
    LogEntry entry;
    entry.MsgType = MsgType;
    entry.Msg = IuCoreUtils::Utf8ToWstring(Msg);
    entry.Info = IuCoreUtils::Utf8ToWstring(Info);
    entry.Sender = IuCoreUtils::Utf8ToWstring(Sender);
    entry.FileName = IuCoreUtils::Utf8ToWstring(FileName);

    SYSTEMTIME st;
    ::GetLocalTime(&st);

    entry.Time  = str(boost::wformat(L"%02d:%02d:%02d")% static_cast<int>(st.wHour) % static_cast<int>(st.wMinute) % static_cast<int>(st.wSecond));

    std::ostringstream oss;
    oss << "[" << Sender << "] ";
    if (!entry.FileName.empty()) {
        oss << "[" << FileName << "] ";
    }
    oss << std::endl;
    if (!entry.Info.empty()) {
        oss << Info << std::endl;
    }
    oss << Msg;

    switch (MsgType) {
    case LogMsgType::logWarning:
        LOG(WARNING) << oss.str();
        break;
    case LogMsgType::logError:
        LOG(ERROR) << oss.str();
        break;
    case LogMsgType::logInformation:
        LOG(INFO) << oss.str();
        break;
    default:
        LOG(INFO) << oss.str();
    }


    size_t itemIndex;
    {
        std::lock_guard<std::mutex> lk(entriesMutex_);
        entries_.push_back(entry);
        itemIndex = entries_.size() - 1;
    }
   
    for (auto* listener : listeners_) {
        listener->onItemAdded(itemIndex, entry);
    }
}

#pragma optimize("", off)

void DefaultLogger::write(LogMsgType MsgType, const wchar_t* Sender, const wchar_t* Msg, const wchar_t* Info, const wchar_t*  FileName) {
    if (insideWriteFunction_) { // Prevent recursion
        return;
    }
    insideWriteFunction_ = true;
    defer d([&] { // Run at function exit
        insideWriteFunction_ = false;
    });
    LogEntry entry;
    entry.MsgType = MsgType;
    entry.Msg = Msg;
    entry.Info = Info;
    entry.Sender = Sender;
    entry.FileName = FileName;
    SYSTEMTIME st;
    ::GetLocalTime(&st);

    entry.Time = str(boost::wformat(L"%02d:%02d:%02d") % static_cast<int>(st.wHour) % static_cast<int>(st.wMinute) % static_cast<int>(st.wSecond));

    std::wstringstream oss;
    oss << "[" << Sender << "] ";
    if (!entry.FileName.empty()) {
        oss << "[" << FileName << "] ";
    }
    oss << std::endl;
    if (!entry.Info.empty()) {
        oss << Info << std::endl;
    }
    oss << Msg;
    std::string utf8String = IuCoreUtils::WstringToUtf8(oss.str());

    switch (MsgType) {
        case LogMsgType::logWarning:
            LOG(WARNING) << utf8String;
            break;
        case LogMsgType::logError:
            LOG(ERROR) << utf8String;
            break;
        case LogMsgType::logInformation:
            LOG(INFO) << utf8String;
            break;
        default:
            LOG(INFO) << utf8String;
    }

    insideWriteFunction_ = false;

    size_t itemIndex;
    {
        std::lock_guard<std::mutex> lk(entriesMutex_);
        entries_.push_back(entry);
        itemIndex = entries_.size() - 1;
    }

    for (auto* listener : listeners_) {
        listener->onItemAdded(itemIndex, entry);
    }
}
#pragma optimize("", on)
void DefaultLogger::addListener(Listener* listener) {
    listeners_.push_back(listener);
}

void DefaultLogger::removeListener(const Listener* listener) {
    for (size_t i = 0; i < listeners_.size(); i++) {
        if (listeners_[i] == listener) {
            listeners_[i] = listeners_[listeners_.size() - 1];
            listeners_.pop_back();
            break;
        }
    }
}
size_t DefaultLogger::entryCount() const {
    return entries_.size();
}

void DefaultLogger::getEntry(size_t itemIndex, LogEntry* out) {
    std::lock_guard<std::mutex> lk(entriesMutex_);
    *out = entries_[itemIndex];
}

std::mutex& DefaultLogger::getEntryMutex() {
    return entriesMutex_;
}

std::vector<DefaultLogger::LogEntry>::const_iterator DefaultLogger::begin() const {
    return entries_.begin();
}

std::vector<DefaultLogger::LogEntry>::const_iterator DefaultLogger::end() const {
    return entries_.end();
}

void DefaultLogger::clear() {
    std::lock_guard<std::mutex> lk(entriesMutex_);
    entries_.clear();
}
