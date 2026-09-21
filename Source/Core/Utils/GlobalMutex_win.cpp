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

#include "GlobalMutex.h"

#include <thread>
#include "Core/Utils/CoreUtils.h"
#include <windows.h>

namespace IuCoreUtils {

class ZGlobalMutexPrivate {
public:
    HANDLE mutex_;
};

ZGlobalMutex::ZGlobalMutex(const std::string& name) : d_ptr(new ZGlobalMutexPrivate)
{
    d_ptr->mutex_ = ::CreateMutexA(NULL, FALSE, name.c_str());

    if (!d_ptr->mutex_  && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        d_ptr->mutex_ = OpenMutexA(0, 0, name.c_str());
    }
    lock();
}

void ZGlobalMutex::lock()
{
    WaitForSingleObject(d_ptr->mutex_, INFINITE);
}

void ZGlobalMutex::unlock()
{
    ReleaseMutex(d_ptr->mutex_);
}

ZGlobalMutex::~ZGlobalMutex()
{
    unlock();
    CloseHandle(d_ptr->mutex_);
    delete d_ptr;
}
}
