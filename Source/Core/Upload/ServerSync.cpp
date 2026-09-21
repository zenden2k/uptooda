#include "ServerSync.h"

#include "Core/Logging.h"
#include "Core/ThreadSyncPrivate.h"
#include <map>
#include <atomic>

class ServerSyncPrivate: public ThreadSyncPrivate
{
public:
    std::mutex loginMutex_;
    std::atomic<bool> authPerformed_;
    std::atomic<bool> authPerformedSuccess_;
    std::map<std::string, std::string> constVars_;
    std::mutex  constVarsMutex_;
    std::mutex  folderMutex_;
    std::mutex refreshTokenMutex_;
};
ServerSync::ServerSync() : ThreadSync(new ServerSyncPrivate())
{
    MY_D(ServerSync);
    d->authPerformed_ = false;
    d->authPerformedSuccess_ = false;
}

bool ServerSync::beginAuth()
{
    MY_D(ServerSync);
    try {
        d->loginMutex_.lock();
    } catch (std::exception& ex) {
        LOG(ERROR) << "ServerSync::beginLogin exception: " << ex.what();
        return false;
    }
    if (d->authPerformed_ && !d->authPerformedSuccess_)
    {
        d->loginMutex_.unlock();
        throw ServerSyncException("Upload aborted: Authentication failed");
    }
    return true;
}

bool ServerSync::endAuth()
{
    MY_D(ServerSync);
    try
    {
        d->loginMutex_.unlock();
    } catch (std::exception& ex)
    {
        LOG(ERROR) << "ServerSync::endLogin exception: " << ex.what();
        return false;
    }
    
    return true;
}

void ServerSync::setAuthPerformed(bool success)
{
    MY_D(ServerSync);
    d->authPerformed_ = true;
    d->authPerformedSuccess_ = success;
}

bool ServerSync::isAuthPerformed()
{
    MY_D(ServerSync);
    return d->authPerformed_;
}

void ServerSync::resetAuthorization()
{
    MY_D(ServerSync);
    d->authPerformed_ = false;
    d->authPerformedSuccess_ = false;
}

void ServerSync::resetFailedAuthorization()
{
    MY_D(ServerSync);
//    std::lock_guard<std::mutex> lock(d_ptr->threadCountMutex_);
    if (!d_ptr->threadCount_ && d->authPerformed_ && !d->authPerformedSuccess_ )
    {
        d->authPerformed_ = false;
    }
}

void ServerSync::setConstVar(const std::string& name, const std::string& value)
{
    MY_D(ServerSync);
    std::lock_guard<std::mutex> lock(d->constVarsMutex_);
    d->constVars_[name] = value;
}

std::string ServerSync::getConstVar(const std::string& name)
{
    MY_D(ServerSync);
    std::lock_guard<std::mutex> lock(d->constVarsMutex_);
    auto it = d->constVars_.find(name);
    if (it != d->constVars_.end())
    {
        return it->second;
    }
    return {};
}

std::mutex& ServerSync::folderMutex() {
    MY_D(ServerSync);
    return d->folderMutex_;
}

std::mutex& ServerSync::refreshTokenMutex() {
    MY_D(ServerSync);
    return d->refreshTokenMutex_;
}

std::mutex& ServerSync::loginMutex() {
    MY_D(ServerSync);
    return d->loginMutex_;
}

bool ServerSync::isAuthPerformedSuccess() {
    MY_D(ServerSync);
    return d->authPerformedSuccess_;
}