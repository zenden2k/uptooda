#include "ServerListView.h"

#include <strsafe.h>

#include "Gui/Models/ServerListModel.h"
#include "Core/Utils/CoreUtils.h"
#include "Gui/Helpers/DPIHelper.h"
#include "Core/WinServerIconCache.h"
#include "Core/i18n/Translator.h"

UINT CServerListView::columns[1] = { ServerListModel::tcMaxFileSize };

CServerListView::CServerListView(ServerListModel* model, WinServerIconCache* serverIconCache)
    : model_(model)
    , serverIconCache_(serverIconCache) {
    model_->setRowChangedCallback([this](auto&& PH1) { onRowChanged(PH1); });
    model_->setItemCountChangedCallback([this](size_t size) {
        SetItemCount(static_cast<int>(size));
    });

    model_->setIconsChangedCallback([this] {
        createResources();
    });
}

BOOL CServerListView::SubclassWindow(HWND hWnd) {
    BOOL res = TParent::SubclassWindow(hWnd);
    Init();
    return res;
}

void CServerListView::Init() {
    AddColumn(TR("Server"), ServerListModel::tcServerName);
    AddColumn(TR("Max. file size"), ServerListModel::tcMaxFileSize);
    AddColumn(TR("Storage time (days)"), ServerListModel::tcStorageTime);
    AddColumn(TR("Account"), ServerListModel::tcAccount, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_CENTER);
    AddColumn(TR("File formats"), ServerListModel::tcFileFormats);

    /*LVTILEVIEWINFO tileViewInfo = { 0 };

    tileViewInfo.cbSize = sizeof(tileViewInfo);
    tileViewInfo.dwMask = LVTVIM_COLUMNS;
    tileViewInfo.cLines = 2;
    ListView_SetTileViewInfo(m_hWnd, &tileViewInfo);*/

    setColumnWidths();
    createResources();

    SetItemCount(static_cast<int>(model_->getCount()));
}

int CServerListView::SetView(DWORD dwView) {
    int result = CListViewCtrl::SetView(dwView);
    createResources();
    Arrange(LVA_DEFAULT);
    return result;
}

LRESULT CServerListView::OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* pDispInfo = reinterpret_cast<LV_DISPINFO*>(pnmh);
    LV_ITEM* pItem = &(pDispInfo)->item;

    if (pItem->mask & LVIF_TEXT)  {
        std::string str = model_->getItemText(pItem->iItem, pItem->iSubItem);
        std::wstring wstr = IuCoreUtils::Utf8ToWstring(str);
        StringCchCopy(pItem->pszText, pItem->cchTextMax, wstr.c_str());
    }

    if (pItem->mask & LVIF_IMAGE) {
        const auto& serverName = model_->getDataByIndex(pItem->iItem)->ued->Name;
        auto it = serverIconImageListIndexes_.find(serverName);

         pItem->iImage = serverIconImageListIndexes_[serverName];
        
    }

    if (pItem->mask & LVIF_COLUMNS) {
        pItem->cColumns = 1;
        pItem->puColumns = columns;
    }

    return 0;
}

LRESULT CServerListView::OnOdFindItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pnmh);

    // Получаем информацию о поиске
    LVFINDINFO* pfi = &pFindInfo->lvfi;
    int startIndex = pFindInfo->iStart;

    // Тип поиска определяется флагами в pfi->flags
    if (pfi->flags & LVFI_STRING) {
        // Поиск по строке
        return FindItemByString(pfi->psz, startIndex, pfi->flags);
    } else if (pfi->flags & LVFI_PARTIAL) {
        // Частичный поиск по строке
        return FindItemByPartialString(pfi->psz, startIndex);
    } else if (pfi->flags & LVFI_PARAM) {
        // Поиск по lParam
        return FindItemByParam(pfi->lParam, startIndex);
    }

    // Элемент не найден
    return -1;
}


LRESULT CServerListView::OnListViewNMCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    auto* lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);

    switch (lplvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
        lplvcd->clrText = model_->getItemColor(lplvcd->nmcd.dwItemSpec);
        return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}

LRESULT CServerListView::OnMyDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    setColumnWidths();
    createResources();
    return 0;
}

void CServerListView::onRowChanged(size_t index) {
    PostMessage(LVM_REDRAWITEMS, index, index);
}


void CServerListView::setColumnWidths() {
    int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    SetColumnWidth(ServerListModel::tcServerName, MulDiv(140, dpi, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(ServerListModel::tcMaxFileSize, MulDiv(115, dpi, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(ServerListModel::tcStorageTime, MulDiv(110, dpi, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(ServerListModel::tcAccount, MulDiv(60, dpi, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(ServerListModel::tcFileFormats, MulDiv(200, dpi, USER_DEFAULT_SCREEN_DPI));
}

void CServerListView::createResources() {
    int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    bool smallIcons = GetView() == LV_VIEW_DETAILS;
    auto iconsWithIndexes = serverIconCache_->getImageList(dpi, smallIcons);
    serverIconImageList_ = iconsWithIndexes.first;
    serverIconImageListIndexes_ = std::move(iconsWithIndexes.second);
    ModifyStyle(0, LVS_SHAREIMAGELISTS, LVS_SHAREIMAGELISTS);
    SetImageList(serverIconImageList_, smallIcons ? LVSIL_SMALL : LVSIL_NORMAL);
}

int CServerListView::FindItemByString(LPCWSTR searchText, int startIndex, DWORD flags) {
    if (!searchText || !model_) {
        return -1;
    }

    std::wstring searchStr(searchText);
    bool caseSensitive = !(flags & LVFI_STRING); 

    if (!caseSensitive) {
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::towlower);
    }

    int itemCount = model_->getCount();

    // Поиск начинается с указанного индекса
    for (int i = startIndex; i < itemCount; ++i) {
        std::string itemText = model_->getItemText(i, 0); 
        std::wstring wItemText = IuCoreUtils::Utf8ToWstring(itemText);

        if (!caseSensitive) {
            std::transform(wItemText.begin(), wItemText.end(), wItemText.begin(), ::towlower);
        }

        if (wItemText == searchStr) {
            return i; 
        }
    }

    if (flags & LVFI_WRAP) {
        for (int i = 0; i < startIndex; ++i) {
            std::string itemText = model_->getItemText(i, 0);
            std::wstring wItemText = IuCoreUtils::Utf8ToWstring(itemText);

            if (!caseSensitive) {
                std::transform(wItemText.begin(), wItemText.end(), wItemText.begin(), ::towlower);
            }

            if (wItemText == searchStr) {
                return i;
            }
        }
    }

    return -1; 
}

int CServerListView::FindItemByPartialString(LPCWSTR searchText, int startIndex) {
    if (!searchText || !model_) {
        return -1;
    }

    std::wstring searchStr(searchText);
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::towlower);

    int itemCount = model_->getCount();

    for (int i = startIndex; i < itemCount; ++i) {
        std::string itemText = model_->getItemText(i, 0);
        std::wstring wItemText = IuCoreUtils::Utf8ToWstring(itemText);
        std::transform(wItemText.begin(), wItemText.end(), wItemText.begin(), ::towlower);

        if (wItemText.find(searchStr) == 0) {
            return i;
        }
    }

    return -1;
}

int CServerListView::FindItemByParam(LPARAM searchParam, int startIndex) {
    return -1;
}
