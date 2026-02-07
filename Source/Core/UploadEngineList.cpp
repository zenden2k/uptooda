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

#include "UploadEngineList.h"

#include <algorithm>
#include <vector>
#include <optional>

#include "Core/Utils/SimpleXml.h"
#include "Core/Utils/StringUtils.h"
#include "AppParams.h"
#include "Core/BasicConstants.h"

namespace {

template <typename T>
void SplitAssignVarsString(T & reg) {
    std::vector<std::string> Vars;
    IuStringUtils::Split(reg.AssignVars, ";", Vars);

    for (auto it = Vars.begin(); it != Vars.end(); ++it) {
        std::vector<std::string> NameAndValue;
        IuStringUtils::Split(*it, ":", NameAndValue);
        if (NameAndValue.size() == 2) {
            ActionVariable AV;
            AV.Name = NameAndValue[0];
            AV.nIndex = atoi(NameAndValue[1].c_str());
            reg.Variables.push_back(AV);
        }
    }
}
}
CUploadEngineList::CUploadEngineList()
{
}

bool CUploadEngineList::loadFromFile(const std::string& filename, ServerSettingsMap& serversSettings) {
    SimpleXml xml;
    if (!xml.LoadFromFile(filename))
        return false;

    SimpleXmlNode root = xml.getRoot("Servers");

    std::vector<SimpleXmlNode> childs;
    root.GetChilds("Server", childs);
    root.GetChilds("Server2", childs);
    root.GetChilds("Server3", childs);

    auto versionInfo = AppParams::instance()->GetAppVersion();
    int majorVersion = versionInfo->Major;
    int minorVersion = versionInfo->Minor * 100 + versionInfo->Release;
    int build = versionInfo->Build;

    for (size_t i = 0; i < childs.size(); i++) {
        SimpleXmlNode& cur = childs[i];
        auto uploadEngineData = std::make_unique<CUploadEngineData>();
        CUploadEngineData& UE = *uploadEngineData;
        UE.NeedAuthorization = cur.AttributeInt("Authorize");

        if (UE.NeedAuthorization) {
            UE.addUserType(UserTypes::REGISTERED);
        }
        std::string needPassword = cur.Attribute("NeedPassword");
        UE.NeedPassword = needPassword.empty() ? true : (IuCoreUtils::StringToInt64(needPassword) != 0);
        UE.LoginLabel = cur.Attribute("LoginLabel");
        UE.PasswordLabel = cur.Attribute("PasswordLabel");
        std::string RetryLimit = cur.Attribute("RetryLimit");

        if (RetryLimit.empty()) {
            UE.RetryLimit = m_EngineNumOfRetries;
        } else {
            UE.RetryLimit = atoi(RetryLimit.c_str());
        }

        UE.Name = cur.Attribute("Name");
        UE.DisplayName = cur.Attribute("DisplayName");
//#ifdef NDEBUG
        std::string serverMinVersion = cur.Attribute("MinVersion");
        if (!serverMinVersion.empty()) {
            std::vector<std::string> tokens;
            IuStringUtils::Split(serverMinVersion, ".", tokens, 4);
            if (tokens.size() >= 3) {
                int serverMajorVersion = atoi(tokens[0].c_str());
                int serverMinorVersion = atoi(tokens[1].c_str()) * 100 + atoi(
                    tokens[2].c_str());
                int serverBuild = static_cast<int>(tokens.size() > 3 ? atoi(tokens[3].c_str()) : 0);
                if (!(majorVersion > serverMajorVersion || (majorVersion == serverMajorVersion && minorVersion >
                        serverMinorVersion)
                    || (majorVersion == serverMajorVersion && minorVersion == serverMinorVersion && (!serverBuild ||
                        build >= serverBuild))
                )) {
                    continue;
                }
            }
        }
//#endif
        UE.SupportsFolders = cur.AttributeBool("SupportsFolders");
        UE.RegistrationUrl = cur.Attribute("RegistrationUrl");
        UE.WebsiteUrl = cur.Attribute("WebsiteUrl");
        UE.UserAgent = cur.Attribute("UserAgent");
        UE.PluginName = cur.Attribute("Plugin");
        UE.Engine = cur.Attribute("Engine");
        std::string MaxThreadsStr = cur.Attribute("MaxThreads");
        UE.MaxThreads = atoi(MaxThreadsStr.c_str());

        if ((UE.PluginName == CORE_SCRIPT_FTP || UE.PluginName == CORE_SCRIPT_DIRECTORY) && MaxThreadsStr.empty()) {
            UE.MaxThreads = 1;
        }

        UE.UsingPlugin = !UE.PluginName.empty();
        UE.Debug = cur.AttributeBool("Debug");
        if (UE.Debug) {
            UE.MaxThreads = 1;
        }
        bool fileHost = cur.AttributeBool("FileHost");
        std::string maxFileSize = cur.Attribute("MaxFileSize");
        if (!maxFileSize.empty()) {
            try {
                UE.MaxFileSize = std::stoll(maxFileSize);
            } catch (const std::exception&) {
            }
        }

        UE.UploadToTempServer = cur.AttributeBool("UploadToTempServer");

        std::string typeString = cur.Attribute("Type");

        UE.TypeMask = 0;

        std::vector<std::string> types;

        std::string typesListString = cur.Attribute("Types");
        if (!typesListString.empty()) {
            IuStringUtils::Split(typesListString, " ", types, 10);
        }
        if (!typeString.empty()) {
            types.push_back(typeString);
        }
        if (types.empty()) {
            types.emplace_back(fileHost ? "file" : "image");
        }
        for (auto& it : types) {
            UE.TypeMask |= CUploadEngineData::ServerTypeFromString(it);
        }

        std::string defaultForTypes = cur.Attribute("DefaultForTypes");

        if (!defaultForTypes.empty()) {
            std::vector<std::string> serverTypes;
            IuStringUtils::Split(defaultForTypes, " ", serverTypes, 10);
            for (const auto& typeStr : serverTypes) {
                auto serverType = CUploadEngineData::ServerTypeFromString(typeStr);
                if (serverType != CUploadEngineData::TypeInvalid) {
                    m_defaultServersForType[serverType] = UE.Name;
                }
            }
        }

        SimpleXmlNode infoNode = cur.GetChild("Info", false);
        if (!infoNode.IsNull()) {
            SimpleXmlNode supportedFormatsNode = infoNode.GetChild("SupportedFormats", false);
            if (!supportedFormatsNode.IsNull()) {
                loadFormats(supportedFormatsNode, UE, UE.SupportedFormatGroups);
            }

            SimpleXmlNode forbiddenFormatsNode = infoNode.GetChild("ForbiddenFormats", false);
            if (!forbiddenFormatsNode.IsNull()) {
                loadFormats(forbiddenFormatsNode, UE, UE.ForbiddenFormatGroups);
            }

            SimpleXmlNode storageTimeInfoNode = infoNode.GetChild("StorageTimeInfo", false);
            if (!storageTimeInfoNode.IsNull()) {
                loadStorageTimeInfo(storageTimeInfoNode, UE, UE.StorageTimeInfo);
            }
        }

        std::vector<SimpleXmlNode> actions;
        cur["Actions"].GetChilds("Action", actions);

        for (size_t j = 0; j < actions.size(); j++) {
            SimpleXmlNode& actionNode = actions[j];
            UploadAction UA;
            UA.Index = j;

            std::string RetryLimit = actionNode.Attribute("RetryLimit");
            if (RetryLimit.empty()) {
                UA.RetryLimit = m_ActionNumOfRetries; //Settings.ActionRetryLimit;
            } else UA.RetryLimit = atoi(RetryLimit.c_str());

            UA.IgnoreErrors = actionNode.AttributeBool("IgnoreErrors");
            UA.Description = actionNode.Attribute("Description");

            UA.Type = actionNode.Attribute("Type");
            UA.Url = actionNode.Attribute("Url");
            UA.Referer = actionNode.Attribute("Referer");

            UA.PostParams = actionNode.Attribute("PostParams");
            UA.CustomHeaders = actionNode.Attribute("CustomHeaders");
            UA.OnlyOnce = actionNode.AttributeBool("OnlyOnce");

            ActionFunc funcCall(ActionFunc::FUNC_REGEXP);
            funcCall.setArg(1, actionNode.Attribute("RegExp"));
            funcCall.AssignVars = actionNode.Attribute("AssignVars");
            funcCall.Required = true;
            UA.FunctionCalls.push_back(funcCall);

            actionNode.each([&](int k, SimpleXmlNode& callNode) -> bool {
                if (callNode.Name() == "RegExp") {
                    ActionFunc newFuncCall(ActionFunc::FUNC_REGEXP);
                    newFuncCall.setArg(1, callNode.Attribute("Pattern"));
                    newFuncCall.AssignVars = callNode.Attribute("AssignVars");
                    newFuncCall.Required = callNode.AttributeBool("Required");
                    newFuncCall.setArg(0, callNode.Attribute("Data"));
                    UA.FunctionCalls.push_back(newFuncCall);
                } else if (callNode.Name() == "Call") {
                    ActionFunc newFuncCall;
                    newFuncCall.Func = callNode.Attribute("Function");
                    newFuncCall.AssignVars = callNode.Attribute("AssignVars");
                    newFuncCall.Required = callNode.AttributeBool("Required");
                    std::vector<std::string> argumentsAttributeNames;
                    callNode.GetAttributes(argumentsAttributeNames);
                    std::string prefix { "Arg" };
                    for (auto& name : argumentsAttributeNames) {
                        if (!name.compare(0, prefix.size(), prefix)) {
                            size_t index = std::stoi(name.substr(prefix.size()));
                            std::string val;
                            callNode.GetAttribute(name, val);
                            newFuncCall.setArg(index, val);
                        }
                    }
                    UA.FunctionCalls.push_back(newFuncCall);
                } else {
                    LOG(ERROR) << "Unknown function: " << callNode.Name();
                    return true;
                }
                return false;
            });

            for (auto& reg: UA.FunctionCalls) {
                SplitAssignVarsString(reg);
            }

            UE.Actions.push_back(UA);
        }

        SimpleXmlNode resultNode = cur["Result"];
        {
            UE.DownloadUrlTemplate = resultNode.Attribute("DownloadUrlTemplate");
            if (UE.DownloadUrlTemplate.empty()) {
                UE.DownloadUrlTemplate = resultNode.Attribute("DownloadUrl");
            }
            UE.ImageUrlTemplate = resultNode.Attribute("ImageUrlTemplate");
            if (UE.ImageUrlTemplate.empty()) {
                UE.ImageUrlTemplate = resultNode.Attribute("ImageUrl");
            }
            UE.ThumbUrlTemplate = resultNode.Attribute("ThumbUrlTemplate");
            if (UE.ThumbUrlTemplate.empty()) {
                UE.ThumbUrlTemplate = resultNode.Attribute("ThumbUrl");
            }
            UE.EditUrlTemplate = resultNode.Attribute("EditUrl");
            UE.DeleteUrlTemplate = resultNode.Attribute("DeleteUrl");
            std::string directUrlTemplate = resultNode.Attribute("DirectUrlTemplate");
            if (directUrlTemplate.empty()) {
                directUrlTemplate = resultNode.Attribute("DirectUrl");
            }
            if (!directUrlTemplate.empty()) {

                UE.ImageUrlTemplate = directUrlTemplate;
            }
        }
        UE.SupportThumbnails = !UE.ThumbUrlTemplate.empty();
        m_list.push_back(std::move(uploadEngineData));
    }

    sort();
    return true;
}

bool CUploadEngineList::compareEngines(const std::unique_ptr<CUploadEngineData>& elem1, const std::unique_ptr<CUploadEngineData>& elem2)
{
    return IuStringUtils::stricmp(elem1->Name.c_str(), elem2->Name.c_str()) < 0;
}

void CUploadEngineList::loadFormats(SimpleXmlNode& node, CUploadEngineData& UE, std::vector<FileFormatGroup>& out)
{
    node.each([&](int k, SimpleXmlNode& groupNode) -> bool {
        if (groupNode.Name() == "FormatGroup") {
            FileFormatGroup group;
            std::string userTypesStr = groupNode.Attribute("UserTypes");
            if (!userTypesStr.empty()) {
                std::vector<std::string> userTypes;

                IuStringUtils::Split(userTypesStr, ",", userTypes);
                std::optional<int> minUserRank;

                for (const auto& userType : userTypes) {
                    int userTypeId = UE.addUserType(userType);
                    if (!minUserRank.has_value() || userTypeId < minUserRank) {
                        minUserRank = userTypeId;
                    }
                    group.UserTypeIds.insert(userTypeId);
                    group.UserTypes.insert(userType);
                }
                if (minUserRank.has_value()) {
                    group.MinUserRank = *minUserRank;
                }
            }

            group.MaxFileSize = groupNode.AttributeInt64("MaxFileSize");
            group.MinFileSize = groupNode.AttributeInt64("MinFileSize");

            groupNode.each([&](int k, SimpleXmlNode& formatNode) -> bool {
                if (formatNode.Name() == "Format") {
                    FileFormat format;
                    IuStringUtils::Split(formatNode.Attribute("MimeType"), ",", format.MimeTypes);
                    std::string listStr = formatNode.Text();
                    IuStringUtils::Split(listStr, ",", format.FileNameWildcards);
                    for (const auto& wildcard : format.FileNameWildcards) {
                        if (wildcard.length() > 2 && wildcard.rfind("*.", 0) == 0 && std::count(wildcard.begin(), wildcard.end(), '*') == 1
                            && std::count(wildcard.begin(), wildcard.end(), '?') == 0) {
                            group.Extensions.insert(wildcard.substr(2));
                        }
                    }
                    format.MaxFileSize = formatNode.AttributeInt64("MaxFileSize");
                    format.MinFileSize = formatNode.AttributeInt64("MinFileSize");
                    group.Formats.push_back(std::move(format));
                }
                return false;
            });
            out.push_back(std::move(group));
        }
        return false;
    });

    std::stable_sort(out.begin(), out.end(), [](const FileFormatGroup& a, const FileFormatGroup& b) {
        return a.MinUserRank < b.MinUserRank;
    });
}

void CUploadEngineList::loadStorageTimeInfo(SimpleXmlNode& node, CUploadEngineData& UE, std::vector<StorageTime>& out) {
    node.each([&](int k, SimpleXmlNode& groupNode) -> bool {
        if (groupNode.Name() == "StorageTime") {
            StorageTime group;
            std::string userTypesStr = groupNode.Attribute("UserTypes");

            if (!userTypesStr.empty()) {
                std::vector<std::string> userTypes;

                IuStringUtils::Split(userTypesStr, ",", userTypes);
                std::optional<int> minUserRank;

                for (const auto& userType : userTypes) {
                    int userTypeId = UE.addUserType(userType);
                    if (!minUserRank.has_value() || userTypeId < minUserRank) {
                        minUserRank = userTypeId;
                    }
                    group.UserTypeIds.insert(userTypeId);
                    group.UserTypes.push_back(userType);
                }
                if (minUserRank.has_value()) {
                    group.MinUserRank = *minUserRank;
                }
            }

            try {
                group.Time = std::stoi(groupNode.Text());
            } catch (const std::exception& ex) {
                LOG(WARNING) << "loadStorageTimeInfo exception, line no. " << groupNode.GetLineNumber() << ": " << ex.what();
            }
            group.AfterLastDownload = groupNode.AttributeBool("AfterLastDownload");

            out.push_back(std::move(group));
        }
        return false;
    });

    std::stable_sort(out.begin(), out.end(), [](const StorageTime& a, const StorageTime& b) {
        return a.MinUserRank < b.MinUserRank;
    });
}

void CUploadEngineList::sort() {
    std::sort(m_list.begin(), m_list.end(), compareEngines);

    serverNameToIndex_.clear();
    for (size_t i = 0; i < m_list.size(); i++) {
        serverNameToIndex_[IuStringUtils::ToLower(m_list[i]->Name)] = i;
    }
}

void CUploadEngineList::setNumOfRetries(int Engine, int Action)
{
    m_EngineNumOfRetries = Engine;
    m_ActionNumOfRetries = Action;
}

bool CUploadEngineList::addServer(const CUploadEngineData& data)
{
    auto uploadEngineData = std::make_unique<CUploadEngineData>(data);
    m_list.push_back(std::move(uploadEngineData));
    sort();
    return true;
}
