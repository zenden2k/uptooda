/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

namespace IuCoreUtils {
using namespace boost::interprocess;
class ZGlobalMutexPrivate {
public:
    named_mutex* mutex_;
};

ZGlobalMutex::ZGlobalMutex(const std::string& name) : d_ptr(new ZGlobalMutexPrivate)
{
    d_ptr->mutex_ = new named_mutex(open_or_create, name.c_str());
    /*scoped_lock<named_mutex> lock(mutex);

    boost::filesystem::path temp = boost::filesystem::unique_path();
    const std::string tempstr = temp.native();  // optional
    d_ptr->key_ = ftok(tempstr.c_str(), 1);
    d_ptr->sid_ = semget(key_, 1, IPC_CREAT | 0660));*/
    lock();
}

void ZGlobalMutex::lock()
{
    d_ptr->mutex_->lock();
}

void ZGlobalMutex::unlock()
{
    d_ptr->mutex_->unlock();
}

ZGlobalMutex::~ZGlobalMutex()
{
    unlock();
    delete d_ptr->mutex_;
    delete d_ptr;
}
}
