#include <gtest/gtest.h>

#include "Core/UploadEngineList.h"
#include "Tests/TestHelpers.h"

class UploadEngineListTest : public ::testing::Test {
public:
    UploadEngineListTest() : fileName(TestHelpers::resolvePath("servers.xml")) {

    }
protected:
    const std::string fileName;
};

TEST_F(UploadEngineListTest, loadFromFile)
{
    CUploadEngineList list;
    ServerSettingsMap settings;
    bool res = list.loadFromFile(fileName, settings);
    // List is sorted alphabetically
    EXPECT_TRUE(res);
    EXPECT_EQ(5, list.count());
    const CUploadEngineData* engineData = list.byIndex(2);
    ASSERT_TRUE(engineData != nullptr);
    EXPECT_EQ("radikal.ru", engineData->Name);
    EXPECT_EQ(10000000, engineData->MaxFileSize);
    EXPECT_EQ(false, engineData->Debug);
    EXPECT_EQ(1, engineData->NeedAuthorization);
    EXPECT_EQ(CUploadEngineData::TypeImageServer, engineData->TypeMask);
    EXPECT_EQ("radikal", engineData->PluginName);

    engineData = list.byName("fastpic.ru");
    ASSERT_TRUE(engineData != nullptr);
    EXPECT_EQ("fastpic.ru", engineData->Name);
    EXPECT_EQ(10737418240, engineData->MaxFileSize);
    EXPECT_EQ(true, engineData->Debug);
    EXPECT_EQ(0, engineData->NeedAuthorization);
    EXPECT_EQ("", engineData->PluginName);
    EXPECT_EQ(CUploadEngineData::TypeImageServer, engineData->TypeMask);
    EXPECT_EQ("$(Image)", engineData->ImageUrlTemplate);
    EXPECT_EQ("$(Thumb)", engineData->ThumbUrlTemplate);
    EXPECT_EQ("$(DownloadUrl)", engineData->DownloadUrlTemplate);
    EXPECT_EQ(1, engineData->Actions.size());
    const UploadAction& action = engineData->Actions[0];
    EXPECT_EQ("http://fastpic.ru/upload?api=1", action.Url);
    EXPECT_EQ("method=file;file1=%filename%;check_thumb=size;uploading=1;orig_rotate=0;thumb_size=$(_THUMBWIDTH)", action.PostParams);
    EXPECT_EQ(3, action.FunctionCalls.size());
    {
        const ActionFunc& regexp = action.FunctionCalls[0];
        EXPECT_EQ("<imagepath>(.*?)<\\/imagepath>([\\s\\S]*?)<thumbpath>(.*?)<\\/thumbpath>", regexp.getArg(1));
        EXPECT_EQ("Image:0;Thumb:2", regexp.AssignVars);
        EXPECT_EQ(true, regexp.Required);
    }
    {
        const ActionFunc& regexp = action.FunctionCalls[1];
        EXPECT_EQ("<viewurl>(.*?)<\\/viewurl>", regexp.getArg(1));
        EXPECT_EQ(false, regexp.Required);
        EXPECT_EQ("DownloadUrl:0;", regexp.AssignVars);
    }

    // function calls within action
    {
        const ActionFunc& functionCall = action.FunctionCalls[2];
        EXPECT_EQ("json", functionCall.Func);
        EXPECT_EQ(1, functionCall.Arguments.size());
        EXPECT_EQ(1, functionCall.Variables.size());
    }

    engineData = list.byName("8b.kz");
    ASSERT_TRUE(engineData != nullptr);
    EXPECT_EQ("8b.kz", engineData->Name);
    EXPECT_EQ(false, engineData->Debug);
    EXPECT_EQ(0, engineData->NeedAuthorization);
    EXPECT_EQ(CUploadEngineData::TypeUrlShorteningServer | CUploadEngineData::TypeImageServer, engineData->TypeMask);
    EXPECT_EQ("http://8b.kz/$(surl)", engineData->ImageUrlTemplate);
    {
        EXPECT_EQ(1, engineData->Actions.size());
        const UploadAction& action = engineData->Actions[0];
        EXPECT_EQ("http://8b.kz/ajax/new_url", action.Url);
        EXPECT_EQ("post", action.Type);
        EXPECT_EQ("custom=;url=$(_ORIGINALURL|urlencode)", action.PostParams);
        EXPECT_EQ("http://8b.kz/", action.Referer);
        EXPECT_EQ(1, action.FunctionCalls.size());
        {
            const ActionFunc& regexp = action.FunctionCalls[0];
            EXPECT_EQ("\"key\":\"((\\\\\"|[^\"])*)\"", regexp.getArg(1));
            EXPECT_EQ(true, regexp.Required);
            EXPECT_EQ("surl:0", regexp.AssignVars);
        }
       
    }

    // Checking default servers
    EXPECT_EQ("fastpic.ru", list.getDefaultServerNameForType(CUploadEngineData::TypeImageServer));
    EXPECT_EQ("8b.kz", list.getDefaultServerNameForType(CUploadEngineData::TypeUrlShorteningServer));

    engineData = list.byName("Test directory");
    ASSERT_TRUE(engineData != nullptr);
    EXPECT_EQ("Test directory", engineData->Name);
    EXPECT_EQ(1, engineData->MaxThreads);

    engineData = list.byName("Test Hidden");
    ASSERT_TRUE(engineData == nullptr);
}

TEST_F(UploadEngineListTest, getByIndex)
{
    CUploadEngineList list;
    ServerSettingsMap settings;
    bool res = list.loadFromFile(fileName, settings);
    // List is sorted alphabetically
    EXPECT_TRUE(res);
    EXPECT_EQ(5, list.count());
    const CUploadEngineData* ued = list.byName("fastpic.ru");
    ASSERT_TRUE(ued != nullptr);
    EXPECT_EQ("fastpic.ru", ued->Name);
    const CUploadEngineData* ued2 = list.firstEngineOfType(CUploadEngineData::TypeImageServer);
    ASSERT_TRUE(ued2 != nullptr);
    EXPECT_EQ("8b.kz", ued2->Name);

    const CUploadEngineData* ued3 = list.firstEngineOfType(CUploadEngineData::TypeFileServer);
    EXPECT_TRUE(ued3 == nullptr);

    int index = list.getRandomImageServer();
    EXPECT_TRUE(index >= 0 && index < list.count());
    const CUploadEngineData* ued4 = list.byIndex(index);
    ASSERT_TRUE(ued4 != nullptr);
    ASSERT_TRUE(ued4->hasType(CUploadEngineData::TypeImageServer));

    int index2 = list.getRandomFileServer();
    EXPECT_EQ(-1, index2);

    int index3 = list.getUploadEngineIndex("fastpic.ru");
    EXPECT_TRUE(index3 >= 0 && index3 < list.count());
}

TEST_F(UploadEngineListTest, addServer)
{
    CUploadEngineList list;
    CUploadEngineData uploadEngineData;
    uploadEngineData.Name = "test";
    list.addServer(uploadEngineData);

    EXPECT_EQ(1, list.count());
}

TEST_F(UploadEngineListTest, formats)
{
    CUploadEngineList list;
    ServerSettingsMap settings;
    bool res = list.loadFromFile(fileName, settings);
    EXPECT_TRUE(res);
    const CUploadEngineData* engineData = list.byIndex(2);

    engineData = list.byName("radikal.ru");
    ASSERT_TRUE(engineData != nullptr);
    EXPECT_EQ("radikal.ru", engineData->Name);

    ASSERT_EQ(2, engineData->SupportedFormatGroups.size());
    const auto& format = engineData->SupportedFormatGroups[0].Formats[0];
    EXPECT_EQ("image/jpeg", format.MimeTypes[0]);
    EXPECT_EQ(3000000, format.MaxFileSize);
}
