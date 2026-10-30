#include "NetworkDebugModel.h"

#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <locale>

#include <zlib.h>

#include "Core/ServiceLocator.h"
#include "Core/Network/NetworkDebugger.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/MyEngineList.h"
#include "Core/Utils/IOException.h"

namespace {

std::string CurlTypeToString(curl_infotype type) {
    switch (type) {
    case CURLINFO_TEXT:
        return "== Info: ";
    default:
        return "<unknown>";

    case CURLINFO_HEADER_OUT:
        return "=> Send header";

    case CURLINFO_DATA_OUT:
        return "=> Send data";

    case CURLINFO_SSL_DATA_OUT:
        return "=> Send SSL data";

    case CURLINFO_HEADER_IN:
        return "<= Recv header";

    case CURLINFO_DATA_IN:
        return "<= Recv data";

    case CURLINFO_SSL_DATA_IN:
        return "<= Recv SSL data";
    };
}

size_t StringSearch(const std::string& str1, const std::string& str2) {
    auto loc = std::locale();
    auto it = std::search(str1.begin(), str1.end(),
        str2.begin(), str2.end(), [&loc](char ch1, char ch2) -> bool {
            return std::toupper(ch1, loc) == std::toupper(ch2, loc);
    });
    if (it != str1.end()) {
        return it - str1.begin();
    } else {
        return std::string::npos; // not found
    }
}

}

bool NetworkDebugModelData::acceptFilter(const std::vector<std::string>& fields) const {
    if (fields.size() < NetworkDebugModelNS::COLUMN_COUNT) {
        return true;
    }
    if (!fields[NetworkDebugModelNS::COLUMN_THREAD_ID].empty() && threadId_ != fields[NetworkDebugModelNS::COLUMN_THREAD_ID]) {
        return false;
    }
    if (!fields[NetworkDebugModelNS::COLUMN_TYPE].empty()) {
        std::string val = CurlTypeToString(type);
        if (StringSearch(val, fields[NetworkDebugModelNS::COLUMN_TYPE]) == std::string::npos) {
            return false;
        }
    }
    if (!fields[NetworkDebugModelNS::COLUMN_TIME].empty()) {
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_), "%F %T");
        if (StringSearch(ss.str(), fields[NetworkDebugModelNS::COLUMN_TIME]) == std::string::npos) {
            return false;
        }
    }
    if (!fields[NetworkDebugModelNS::COLUMN_TEXT].empty()) {
        if (type == CURLINFO_HEADER_IN || type == CURLINFO_HEADER_OUT || type == CURLINFO_TEXT) {
            if (StringSearch(data.substr(0, data.find_first_of("\r\n")), fields[NetworkDebugModelNS::COLUMN_TEXT]) == std::string::npos) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

std::string NetworkDebugModelData::getDecoded() {
    const auto CHUNK_SIZE = 0x4000;
    if (!decoded_.empty()) {
        return decoded_;
    }
    unsigned char in[CHUNK_SIZE];
    unsigned char out[CHUNK_SIZE];
    z_stream strm = { 0 };
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = in;
    strm.avail_in = 0;
    const auto windowBits = 15;
    int st = inflateInit2(&strm, 16 + MAX_WBITS);
    if (st < 0) {
        LOG(WARNING) << "Failed to decompress gzip";
        return {};
    }
    size_t inPos = 0;
    while (1) {
        int bytesRead;
        int zlibStatus;

        bytesRead = std::min<size_t>(CHUNK_SIZE, data.size() - inPos);
        strm.avail_in = bytesRead;
        strm.next_in = reinterpret_cast<Bytef*>(&data[inPos]);
        do {
            unsigned have;
            strm.avail_out = CHUNK_SIZE;
            strm.next_out = out;
            zlibStatus = inflate(&strm, Z_NO_FLUSH);
            switch (zlibStatus) {
                case Z_OK:
                case Z_STREAM_END:
                case Z_BUF_ERROR:
                    break;

                default:
                    inflateEnd(&strm);
                    LOG(WARNING) << "zlib error " << zlibStatus;
                    return {};
                }
                have = CHUNK_SIZE - strm.avail_out;
                decoded_.append((const char*)out, have);
                inPos += bytesRead;
        } while (strm.avail_out == 0);
        if (inPos >= data.size()) {
            inflateEnd(&strm);
            break;
        }
    }
    return decoded_;
}

NetworkDebugModel::NetworkDebugModel() {
    std::shared_ptr<NetworkDebugger> debugger = ServiceLocator::instance()->networkDebugger();
    debugger->onMessage.connect([this](INetworkClient*, curl_infotype type, char* data, size_t length) -> void {
        NetworkDebugModelData item;
        item.type = type;
        item.time_ = std::time(nullptr);
        item.data.assign(data, length);
        item.threadId_ = IuCoreUtils::ThreadIdToString(std::this_thread::get_id());
        size_t index;
        auto it = items_.push_back(std::move(item));
        index = std::distance(items_.begin(), it);
        if (item.acceptFilter(filter_)) {
            filteredItemsIndexes_.push_back(index);
            notifyCountChanged(filteredItemsIndexes_.size());
        }
    });
}

NetworkDebugModel::~NetworkDebugModel() {
}

std::string NetworkDebugModel::getItemText(int row, int column) const {
    //std::lock_guard<std::mutex> lk(itemsMutex_);
    const auto& modelData = *getDataByIndex(row);
    return getCell(modelData, row, column);
}

uint32_t NetworkDebugModel::getItemColor(int row) const {
    //std::lock_guard<std::mutex> lk(itemsMutex_);
    const auto* modelData = getDataByIndex(row);

    switch (modelData->type) {
        case CURLINFO_TEXT: 
            return RGB(160, 160, 160);
        default:
            return GetSysColor(COLOR_WINDOWTEXT);
    } 
    return modelData->color;
}

size_t NetworkDebugModel::getCount() const {
    if (filter_.empty()) {
        return items_.size();
    }
    return filteredItemsIndexes_.size();
}

void NetworkDebugModel::notifyRowChanged(size_t row) {
    if (row < items_.size() && rowChangedCallback_) {
        rowChangedCallback_(row);
    }
}


void NetworkDebugModel::notifyCountChanged(size_t row)
{
    if (itemCountChangedCallback_) {
        itemCountChangedCallback_(row);
    }
}

const NetworkDebugModelData* NetworkDebugModel::getDataByIndex(size_t row) const {
    if (!filter_.empty()) {
        if (row >= filteredItemsIndexes_.size()) {
            return nullptr;
        }
        row = filteredItemsIndexes_[row];
    }
    if (row >= items_.size()) {
        return nullptr;
    }
    return &items_[row];
}

void NetworkDebugModel::setOnRowChangedCallback(std::function<void(size_t)> callback) {
    rowChangedCallback_ = std::move(callback);
}


void NetworkDebugModel::setOnItemCountChangedCallback(std::function<void(size_t)> callback) {
    itemCountChangedCallback_ = std::move(callback);
}

void NetworkDebugModel::clear() {
    items_.clear();
    filteredItemsIndexes_.clear();
    notifyCountChanged(0);
}

void NetworkDebugModel::applyFilter(const std::vector<std::string>& fields) {
    filter_ = fields;
    filteredItemsIndexes_.clear();
    size_t i = 0;
    for (const auto& item : items_) {
        if (item.acceptFilter(filter_)) {
            filteredItemsIndexes_.push_back(i);
        }
        i++;
    }
    notifyCountChanged(getCount());
}

void NetworkDebugModel::saveToFile(const std::string& fileName) {
    std::ofstream out(IuCoreUtils::Utf8ToPath(fileName));
    if (!out) {
        throw IOException("Failed to save debug log to file", fileName);
    }
    auto writeFunc = [&out, this](const NetworkDebugModelData& row, size_t i) {
        for (int j = 0; j < NetworkDebugModelNS::COLUMN_COUNT; j++) {
            out << getCell(row, i, j) << "  ";
        }
        out << std::endl;
    };
    if (!filter_.empty()) {
        for (const auto& i : filteredItemsIndexes_) {
            const auto& row = items_[i];
            writeFunc(row, i);
        }
    } else {
        size_t i = 0;
        for (const auto& row : items_) {
            writeFunc(row, i);
            ++i;
        }
    }
}

std::string NetworkDebugModel::getCell(const NetworkDebugModelData& modelData, int /*row*/, int column) const
{
    if (column == NetworkDebugModelNS::COLUMN_N) {
        // return std::to_string(row + 1);
    } else if (column == NetworkDebugModelNS::COLUMN_THREAD_ID) {
        return modelData.threadId_;
    } else if (column == NetworkDebugModelNS::COLUMN_TIME) {
        std::stringstream ss;
        ss << std::put_time(std::localtime(&modelData.time_), "%F %T");
        return ss.str();
    } else if (column == NetworkDebugModelNS::COLUMN_TYPE) {
        return CurlTypeToString(modelData.type);
    } else if (column == NetworkDebugModelNS::COLUMN_TEXT) {
        if (modelData.type == CURLINFO_HEADER_IN || modelData.type == CURLINFO_HEADER_OUT || modelData.type == CURLINFO_TEXT) {
            return modelData.data.substr(0, modelData.data.find_first_of("\r\n"));
        }
    }
    return {};
}
