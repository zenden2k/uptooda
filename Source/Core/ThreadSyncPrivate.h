#ifndef IU_CORE_UPLOAD_THREADSYNCPRIVATE_H
#define IU_CORE_UPLOAD_THREADSYNCPRIVATE_H

#pragma once
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <map>

#include "Network/CurlShare.h"

class ThreadSyncPrivate
{
public:
	std::map<std::string, std::string> data_;
	std::mutex dataMutex_;
    CurlShare curlShare_;
    std::atomic<int> threadCount_;

    ThreadSyncPrivate() : threadCount_(0)
    {
    }

     virtual ~ThreadSyncPrivate() = default;
};

#endif
