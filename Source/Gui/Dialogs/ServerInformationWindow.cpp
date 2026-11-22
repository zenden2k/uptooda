#include "ServerInformationWindow.h"

#include <algorithm>
#include <set>
#include <vector>

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/i18n/Translator.h"
#include "Func/WinUtils.h"
#include "Gui/Models/ServerListModel.h"

namespace {

CString FormatStorageTime(const StorageTime* storage) {
    if (!storage || storage->Time == 0) {
        return U2WC(_c("server_information.storage_time", "Unknown"));
    }
    if (storage->Time == StorageTime::TIME_INFINITE) {
        return U2WC(_c("server_information.storage_time", "Unlimited"));
    }

    const bool inDays = storage->Time % 24 == 0;
    const int count = inDays ? storage->Time / 24 : storage->Time;
    const auto duration = inDays ? _n("%d day", "%d days", count) : _n("%d hour", "%d hours", count);
    const auto time = str(IuStringUtils::FormatNoExcept(duration) % count);
    if (storage->AfterLastDownload) {
        return U2WC(str(IuStringUtils::FormatNoExcept(_("%s since the last download")) % time));
    }
    return U2WC(time);
}

CString AccountName(const std::string& userType) {
    if (userType == UserTypes::ANONYMOUS) {
        return TR("Anonymous user");
    }
    if (userType == UserTypes::REGISTERED) {
        return TR("Registered user");
    }
    if (userType == "premium") {
        return TR("Premium");
    }
    const CString name = U2WC(userType);
    CString firstCharacter = name.Left(1);
    firstCharacter.MakeUpper();
    return firstCharacter + name.Mid(1);
}

} // namespace

ServerInformationWindow::ServerInformationWindow(const ServerData& server) : server_(server) {
}

void ServerInformationWindow::showModal(HWND parent) {
    const HMODULE richEdit = ::LoadLibraryExW(L"Msftedit.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (richEdit) {
        DoModal(parent);
        ::FreeLibrary(richEdit);
    }
}

void ServerInformationWindow::populateText() {
    const auto& engine = *server_.ued;
    CString text = U2WC(server_.getServerDisplayName());
    const CHARRANGE titleRange = { 0, text.GetLength() };
    std::vector<CHARRANGE> accountHeadings;
    std::vector<CHARRANGE> fieldLabels;
    // Rich Edit counts each paragraph break as a single character.
    text += L"\r\r";
    const auto appendField = [&text, &fieldLabels](LPCWSTR label, LPCWSTR value) {
        const LONG labelStart = text.GetLength();
        text.AppendFormat(L"%s:", label);
        fieldLabels.push_back({ labelStart, text.GetLength() });
        text.AppendFormat(L" %s\r", value);
    };

    CString serverTypes;
    const auto appendType = [&engine, &serverTypes](CUploadEngineData::ServerType type, LPCWSTR name) {
        if (engine.hasType(type)) {
            if (!serverTypes.IsEmpty()) {
                serverTypes += L", ";
            }
            serverTypes += name;
        }
    };
    appendType(CUploadEngineData::TypeImageServer, U2WC(_c("server_information.server_type", "Image hosting")));
    appendType(CUploadEngineData::TypeFileServer, U2WC(_c("server_information.server_type", "File hosting")));
    appendType(CUploadEngineData::TypeVideoServer, U2WC(_c("server_information.server_type", "Video hosting")));
    appendType(CUploadEngineData::TypeTextServer, U2WC(_c("server_information.server_type", "Text hosting")));
    appendType(CUploadEngineData::TypeUrlShorteningServer,
               U2WC(_c("server_information.server_type", "URL shortening")));
    appendType(CUploadEngineData::TypeSearchByImageServer,
               U2WC(_c("server_information.server_type", "Search by image")));
    if (!serverTypes.IsEmpty()) {
        appendField(U2WC(_c("server_information.server_type", "Server type")), serverTypes);
    }

    if (!engine.WebsiteUrl.empty()) {
        appendField(TR("Website"), U2WC(engine.WebsiteUrl));
    }
    if (!engine.RegistrationUrl.empty()) {
        appendField(TR("Registration URL"), U2WC(engine.RegistrationUrl));
    }
    if (engine.NeedAuthorization == CUploadEngineData::naObligatory) {
        text += TR("An account is required.");
        text += L"\r";
    }

    for (unsigned int userTypeId = 0; userTypeId < engine.userTypes.size(); ++userTypeId) {
        const auto& userType = engine.userTypes[userTypeId];
        if (engine.NeedAuthorization == CUploadEngineData::naObligatory && userType == UserTypes::ANONYMOUS) {
            continue;
        }

        const StorageTime* storage = nullptr;
        for (const auto& entry : engine.StorageTimeInfo) {
            if (entry.UserTypeIds.empty() || entry.UserTypeIds.contains(userTypeId)) {
                storage = &entry;
            }
        }

        std::set<std::string> extensions;
        int64_t maxFileSize = 0;
        for (const auto& group : engine.SupportedFormatGroups) {
            if (!group.UserTypeIds.empty() && !group.UserTypeIds.contains(userTypeId)) {
                continue;
            }
            extensions.insert(group.Extensions.begin(), group.Extensions.end());
            if (maxFileSize != CUploadEngineData::MAX_FILE_SIZE_UNLIMITED) {
                maxFileSize = group.MaxFileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED
                    ? group.MaxFileSize
                    : std::max(maxFileSize, group.MaxFileSize);
            }
        }
        if (maxFileSize == 0) {
            maxFileSize = engine.MaxFileSize;
        }

        CString size = U2WC(_c("server_information.max_file_size", "Unknown"));
        if (maxFileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED) {
            size = U2WC(_c("server_information.max_file_size", "Unlimited"));
        } else if (maxFileSize > 0) {
            size = U2WC(IuCoreUtils::FileSizeToString(maxFileSize));
        }

        text += L"\r";
        const LONG headingStart = text.GetLength();
        text += AccountName(userType);
        accountHeadings.push_back({ headingStart, text.GetLength() });
        text += L"\r\r";
        appendField(TR("Storage time"), FormatStorageTime(storage));
        appendField(TR("Max. file size"), size);
        appendField(TR("Supported file formats"),
                    extensions.empty() ? CString(U2WC(_c("server_information.file_formats", "Unknown")))
                                       : CString(U2WC(IuStringUtils::Join(extensions, ", "))));
    }
    textControl_.SetWindowText(text);

    const auto applyFormat = [this](const CHARRANGE& range, const CHARFORMATW& format) {
        textControl_.SendMessage(EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
        textControl_.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
    };

    CHARFORMATW format = { sizeof(format) };
    textControl_.SendMessage(EM_GETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&format));
    const LONG bodyHeight = format.yHeight;
    constexpr LONG TWIPS_PER_POINT = 20;
    format.dwMask = CFM_SIZE | CFM_BOLD;
    format.dwEffects = CFE_BOLD;
    format.yHeight = bodyHeight + 4 * TWIPS_PER_POINT;
    applyFormat(titleRange, format);

    format.yHeight = bodyHeight + 2 * TWIPS_PER_POINT;
    for (const auto& heading : accountHeadings) {
        applyFormat(heading, format);
    }

    format.dwMask = CFM_BOLD;
    for (const auto& label : fieldLabels) {
        applyFormat(label, format);
    }
    textControl_.SendMessage(EM_SETSEL, 0, 0);
    textControl_.SendMessage(EM_SCROLLCARET);
}

LRESULT ServerInformationWindow::onInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    SetWindowText(TR("Server information"));
    SetDlgItemText(IDCANCEL, TR("Close"));
    DlgResize_Init();
    CenterWindow(GetParent());

    textControl_ = GetDlgItem(IDC_SERVERINFORMATIONTEXT);
    textControl_.SendMessage(EM_SETBKGNDCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
    textControl_.SendMessage(EM_AUTOURLDETECT, TRUE);
    textControl_.SendMessage(EM_SETEVENTMASK, 0, ENM_LINK);
    populateText();
    GetDlgItem(IDCANCEL).SetFocus();
    return FALSE;
}

LRESULT ServerInformationWindow::onSystemColorChange(UINT, WPARAM, LPARAM, BOOL& handled) {
    textControl_.SendMessage(EM_SETBKGNDCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
    handled = FALSE;
    return 0;
}

LRESULT ServerInformationWindow::onLink(int, LPNMHDR notification, BOOL& handled) {
    const auto& link = *reinterpret_cast<const ENLINK*>(notification);
    if (link.msg == WM_LBUTTONUP) {
        CString url;
        const LONG length = link.chrg.cpMax - link.chrg.cpMin;
        TEXTRANGEW range = { link.chrg, url.GetBuffer(length) };
        textControl_.SendMessage(EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range));
        url.ReleaseBuffer();
        const auto& engine = *server_.ued;
        if ((url.Left(7).CompareNoCase(L"http://") == 0 || url.Left(8).CompareNoCase(L"https://") == 0)
            && (url == U2WC(engine.WebsiteUrl) || url == U2WC(engine.RegistrationUrl))) {
            WinUtils::ShellOpenFileOrUrl(url, m_hWnd);
        }
        return 1;
    }

    handled = FALSE;
    return 0;
}

LRESULT ServerInformationWindow::onClose(WORD, WORD, HWND, BOOL&) {
    EndDialog(IDCANCEL);
    return 0;
}
