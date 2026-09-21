#ifndef IU_CORE_SCRIPTING_SCRIPT_H
#define IU_CORE_SCRIPTING_SCRIPT_H

#pragma once

#include <string>
#include <thread>
#include <memory>

#include "Squirrelnc.h"
#include "Core/ThreadSync.h"
#include "Core/Network/INetworkClient.h"

class ServerSync;
class NetworkClient;
class Script {
    public:
        Script(const std::string& fileName, std::shared_ptr<ThreadSync> serverSync, std::shared_ptr<INetworkClientFactory> networkClientFactory, bool load = true);
        virtual ~Script();
        Sqrat::SqratVM& getVM();
        bool isLoaded() const;
        void InitScriptEngine();
        static void DestroyScriptEngine();
        time_t getCreationTime() const;

        /**
        Using multiple Squirrel VMs in the SAME thread is not supported. 
        This function should be called each time current threads's script is changed.
        */
        void switchToThisVM();

        /**
         * Set currently processed file (used for log filtering)
         */
        void setCurrentTopLevelFileName(const std::string& fileName);
    protected:
        virtual void PrintCallback(const std::string& output);
        void checkCallingThread() const;
        bool load(const std::string& fileName);
        void FlushSquirrelOutput();
        virtual bool preLoad();
        virtual bool postLoad();
        std::string fileName_;
        Sqrat::SqratVM vm_;
        std::unique_ptr<Sqrat::Script> m_SquirrelScript;
        time_t m_CreationTime;
        bool m_bIsPluginLoaded;
        std::thread::id owningThread_;
        std::shared_ptr<ThreadSync> sync_;
        std::string topLevelFileName_;
        std::unique_ptr<INetworkClient> networkClient_;
        std::shared_ptr<INetworkClientFactory> networkClientFactory_;
        static void CompilerErrorHandler(HSQUIRRELVM vm, const SQChar * desc, const SQChar * source, SQInteger line, SQInteger column);
    private:
        DISALLOW_COPY_AND_ASSIGN(Script);
};

#endif