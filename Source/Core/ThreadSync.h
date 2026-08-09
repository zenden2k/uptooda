#ifndef IU_CORE_UPLOAD_THREADSYNC_H
#define IU_CORE_UPLOAD_THREADSYNC_H

#pragma once
#include <memory>
#include "Core/Utils/CoreTypes.h"

class CurlShare;
class ThreadSyncPrivate;

/**
    @brief ThreadSync class contains script-specific data and tools for syncronization 
    threads of the same script
*/
class ThreadSync
{
    public:
        ThreadSync();
        explicit ThreadSync(ThreadSyncPrivate* priv);
        virtual ~ThreadSync() = default;
        void setValue(const std::string& name, const std::string& value);
        std::string getValue(const std::string& name);
        /*! @cond PRIVATE */
        CurlShare* getCurlShare();
        void incrementThreadCount();
        void decrementThreadCount();
        /* @endcond */
    protected:
        std::shared_ptr<ThreadSyncPrivate> d_ptr; // ServerSync should not be copied, but we need to use pimpl idiom for Squirrel binding
        MY_DECLARE_PRIVATE_PTR(ThreadSync);
};

#endif
