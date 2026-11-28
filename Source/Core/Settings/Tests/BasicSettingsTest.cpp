#include <gtest/gtest.h>

#include "Core/BasicConstants.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Utils/CryptoUtils.h"

namespace {

class BasicSettingsTest : public BasicSettings {
public:
    using BasicSettings::LoadAccounts;
    using BasicSettings::SaveAccounts;
};

} // namespace

TEST(ServerSettingsXmlTest, MigratesLegacyAccountsAndParamsOnSave) {
    SimpleXml legacyXml;
    auto legacyRoot = legacyXml.getRoot("ServersParams");
    auto legacyServer = legacyRoot.CreateChild("Server");
    legacyServer.SetAttribute("Name", std::string("Example"));
    legacyServer.SetAttributeBool("Auth", true);
    legacyServer.SetAttribute("Login", CEncodedPassword("legacy-login").toEncodedData());
    legacyServer.SetAttribute("Password", CEncodedPassword("legacy-password").toEncodedData());
    legacyServer.SetAttribute("EncodedParam_ApiToken", CEncodedPassword("legacy-token").toEncodedData());
    legacyServer.SetAttribute("_Custom", std::string("ENC:literal plaintext"));
    // Generated with Utils/encrypt_env_value.py using the decoded ENV_A_E_K.
    legacyServer.SetAttribute(
        "EncodedParam_FromPython",
        std::string("ENC:jkI0J/HY8OS+xczPveOqQZB+mJ1Jk8jObqpuGrLImuF4f+65DRq1V9FwsXDdRFi/bVvI3MyP7Dn35df0/PevKw=="));

    BasicSettingsTest settings;
    ASSERT_TRUE(settings.LoadAccounts(legacyRoot));
    auto& account = settings.ServersSettings.at("Example").at("legacy-login");
    EXPECT_TRUE(account.authData.DoAuth);
    EXPECT_EQ("legacy-login", account.authData.Login);
    EXPECT_EQ("legacy-password", account.authData.Password);
    EXPECT_EQ("legacy-token", account.params.at("ApiToken"));
    EXPECT_EQ("ENC:literal plaintext", account.params.at("Custom"));
    EXPECT_EQ("Python-generated AES-CBC fixture", account.params.at("FromPython"));
    account.setParam("Empty", "");

    SimpleXml encryptedXml;
    ASSERT_TRUE(settings.SaveAccounts(encryptedXml.getRoot("ServersParams")));
    SimpleXml reloadedXml;
    ASSERT_TRUE(reloadedXml.LoadFromString(encryptedXml.ToString()));
    auto encryptedRoot = reloadedXml.getRoot("ServersParams");
    auto encryptedServer = encryptedRoot.GetChild("Server", false);
    ASSERT_FALSE(encryptedServer.IsNull());
    EXPECT_TRUE(encryptedServer.Attribute("_Custom").empty());

    const auto key = IuCoreUtils::CryptoUtils::Base64Decode(ENV_A_E_K);
    for (const auto* name :
         { "Login", "Password", "EncodedParam_ApiToken", "EncodedParam_Custom", "EncodedParam_Empty" }) {
        EXPECT_TRUE(encryptedServer.Attribute(name).starts_with("ENC:")) << name;
    }
    EXPECT_EQ("legacy-token",
              IuCoreUtils::CryptoUtils::DecryptAES(encryptedServer.Attribute("EncodedParam_ApiToken").substr(4), key));

    BasicSettingsTest reloaded;
    ASSERT_TRUE(reloaded.LoadAccounts(encryptedRoot));
    const auto& reloadedAccount = reloaded.ServersSettings.at("Example").at("legacy-login");
    EXPECT_TRUE(reloadedAccount.authData.DoAuth);
    EXPECT_EQ(account.authData.Login, reloadedAccount.authData.Login);
    EXPECT_EQ(account.authData.Password, reloadedAccount.authData.Password);
    EXPECT_EQ(account.params, reloadedAccount.params);
}
