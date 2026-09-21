#include "ThreadSync.h"

#include <map>

#include "ThreadSyncPrivate.h"

ThreadSync::ThreadSync() : d_ptr(new ThreadSyncPrivate())
{
}

ThreadSync::ThreadSync(ThreadSyncPrivate* priv) : d_ptr(priv)
{

}

void ThreadSync::setValue(const std::string& name, const std::string& value)
{
    std::lock_guard<std::mutex> lock(d_ptr->dataMutex_);
    d_ptr->data_[name] = value;
}

std::string ThreadSync::getValue(const std::string& name)
{
    std::lock_guard<std::mutex> lock(d_ptr->dataMutex_);
    auto it = d_ptr->data_.find(name);
    if (it == d_ptr->data_.end()) {
        return {};
    } 
    return it->second;
}

CurlShare* ThreadSync::getCurlShare()
{
    return &d_ptr->curlShare_;
}

void ThreadSync::incrementThreadCount()
{
    ++d_ptr->threadCount_;
}

void ThreadSync::decrementThreadCount()
{
    --d_ptr->threadCount_;
}