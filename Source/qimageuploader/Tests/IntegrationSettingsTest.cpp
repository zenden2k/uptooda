#include <QCoreApplication>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include "Core/Settings/QtGuiSettings.h"
#include "Core/UploadEngineList.h"

#ifdef _WIN32
// Required by the existing WTL helpers linked from iu_core.
CAppModule _Module;
#endif

TEST(IntegrationSettings, ServerGroupsRoundTrip) {
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const std::string path = directory.path().toStdString() + "/";
    QtGuiSettings original;
    CUploadEngineList engines;
    original.setEngineList(&engines);
    original.LoadSettings(path, "settings.xml");
    ServerProfile first;
    first.setServerName("first.example");
    first.setProfileName("account");
    first.setFolderId("folder-123");
    first.setFolderTitle("Screenshots");
    first.setFolderUrl("https://first.example/folder-123");
    original.ServersSettings["first.example"]["account"].authData.Login = "account";
    ServerProfile second;
    second.setServerName("second.example");
    ServerProfileGroup group(first);
    group.addItem(second);
    const QString id = QString::fromUtf8("group-\xD0\xA2\xD0\xB5\xD1\x81\xD1\x82");
    original.ServerProfileGroups[id] = group;
    original.ServerProfileGroups[QStringLiteral("single")] = ServerProfileGroup(second);
    original.ServerProfiles[QStringLiteral("legacy")] = first;
#ifdef _WIN32
    original.ExplorerContextMenu = true;
    original.QuickUpload = true;
    original.SendToContextMenu = true;
#endif
    ASSERT_TRUE(original.SaveSettings());

    QtGuiSettings restored;
    restored.setEngineList(&engines);
    ASSERT_TRUE(restored.LoadSettings(path, "settings.xml"));
    ASSERT_EQ(restored.ServerProfileGroups.size(), 2u);
    ASSERT_EQ(restored.ServerProfileGroups.at(id).getCount(), 2u);
    auto& restoredFirst = restored.ServerProfileGroups.at(id).getByIndex(0);
    EXPECT_EQ(restoredFirst.serverName(), "first.example");
    EXPECT_EQ(restoredFirst.profileName(), "account");
    EXPECT_EQ(restoredFirst.folderId(), "folder-123");
    EXPECT_EQ(restoredFirst.folderTitle(), "Screenshots");
    EXPECT_EQ(restoredFirst.folderUrl(), "https://first.example/folder-123");
    EXPECT_EQ(restored.ServerProfileGroups.at(id).getByIndex(1).serverName(), "second.example");
    EXPECT_EQ(restored.ServerProfiles.at(QStringLiteral("legacy")).serverName(), "first.example");
#ifdef _WIN32
    EXPECT_TRUE(restored.ExplorerContextMenu);
    EXPECT_TRUE(restored.QuickUpload);
    EXPECT_TRUE(restored.SendToContextMenu);
#endif
    // Repeated saves must not append duplicate profiles, and reloading must discard removed groups.
    restored.ServerProfileGroups.erase(QStringLiteral("single"));
    ASSERT_TRUE(restored.SaveSettings());
    ASSERT_TRUE(original.LoadSettings(path, "settings.xml"));
    EXPECT_EQ(original.ServerProfileGroups.size(), 1u);
    EXPECT_EQ(original.ServerProfileGroups.at(id).getCount(), 2u);
}

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
