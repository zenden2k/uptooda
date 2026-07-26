#include "UserFilter.h"

#include "Core/Settings/BasicSettings.h"
#include "Core/Scripting/UploadFilterScript.h"
#include "Core/ServiceLocator.h"

UserFilter::UserFilter(ScriptsManager* scriptsManager)
{
    scriptsManager_ = scriptsManager;
}

bool UserFilter::PreUpload(UploadTask* task)
{
    auto settings = ServiceLocator::instance()->basicSettings();
    if (settings->ExecuteScript && !settings->ScriptFileName.empty())
    {
        auto script = std::dynamic_pointer_cast<UploadFilterScript>(scriptsManager_->getScript(settings->ScriptFileName, ScriptsManager::ScriptType::TypeUploadFilterScript));
        if (!script)
        {
            return false;
        }
        return script->preUpload(task);
    }
    return true;
}

bool UserFilter::PostUpload(UploadTask* task)
{
    auto settings = ServiceLocator::instance()->basicSettings();
    if (settings->ExecuteScript && !settings->ScriptFileName.empty())
    {
        auto script = std::dynamic_pointer_cast<UploadFilterScript>(scriptsManager_->getScript(settings->ScriptFileName, ScriptsManager::ScriptType::TypeUploadFilterScript));
        if (!script)
        {
            return false;
        }
        return script->postUpload(task);
    }
    return true;
}
