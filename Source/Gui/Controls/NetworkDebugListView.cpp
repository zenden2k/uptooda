#include "NetworkDebugListView.h"

#include <cassert>
#include <strsafe.h>

#include "Gui/Models/NetworkDebugModel.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/i18n/Translator.h"
#include "Core/TaskDispatcher.h"

CNetworkDebugListView::CNetworkDebugListView(NetworkDebugModel* model)
    : model_(model)
{
    using namespace std::placeholders;
    model_->setOnRowChangedCallback([this](auto&& PH1) { onRowChanged(PH1); });
    model_->setOnItemCountChangedCallback([this](auto&& PH1) { onItemCountChanged(PH1); });
}

void CNetworkDebugListView::Init() {
    SetItemCount(model_->getCount());
    columns_ = {
        { NetworkDebugModelNS::COLUMN_N, TR("N"), HDFT_ISSTRING, 25 },
        { NetworkDebugModelNS::COLUMN_THREAD_ID, TR("Thread"), HDFT_ISSTRING  /* HDFT_ISNUMBER*/, 55 },
        { NetworkDebugModelNS::COLUMN_TIME, TR("Time"), HDFT_ISSTRING, 160 },
        { NetworkDebugModelNS::COLUMN_TYPE, TR("Type"), HDFT_ISSTRING, 130 },
        { NetworkDebugModelNS::COLUMN_TEXT, TR("Text"), HDFT_ISSTRING, 320 }
    };

    assert(columns_.size() == NetworkDebugModelNS::COLUMN_COUNT);

    for (const auto& column : columns_) {
        AddColumn(column.title, column.index);
    }

    CHeaderCtrl hdr = GetHeader();
    hdr.ModifyStyle(0, HDS_FILTERBAR);

    CClientDC dc(m_hWnd);
    int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    //int dpiY = dc.GetDeviceCaps(LOGPIXELSY);

    for (const auto& column : columns_) {
        SetColumnWidth(column.index, MulDiv(column.width, dpiX, USER_DEFAULT_SCREEN_DPI));

        HDITEM hdItem {};
        hdItem.mask = HDI_FILTER;
        hdItem.type = column.filterType;
        hdr.SetItem(column.index, &hdItem);
    }

    hdr.SetFilterChangeTimeout(1000);
}

LRESULT CNetworkDebugListView::OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* pDispInfo = reinterpret_cast<LV_DISPINFO*>(pnmh);
    LV_ITEM* pItem = &(pDispInfo)->item;

    if (pItem->mask & LVIF_TEXT)  {
        std::string str = model_->getItemText(pItem->iItem, pItem->iSubItem);
        std::wstring wstr = IuCoreUtils::Utf8ToWstring(str);
        StringCchCopy(pItem->pszText, pItem->cchTextMax, wstr.c_str());
    }

    return 0;
}

LRESULT CNetworkDebugListView::OnListViewNMCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    auto* lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);

    switch (lplvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
    {
        auto color = model_->getItemColor(lplvcd->nmcd.dwItemSpec);

        if (color) {
            lplvcd->clrText = color;
            return CDRF_NEWFONT;
        }
    }
    break;
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
        //lplvcd->clrText = RGB(255, 0, 0);
        return CDRF_NEWFONT;
    }
    return 0;
}


LRESULT CNetworkDebugListView::OnHeaderFilterChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* pNMHeader = reinterpret_cast<LPNMHEADER>(pnmh);
    applyFilters();
    return 0;
}

// This callback is called in the working thread
void CNetworkDebugListView::onRowChanged(size_t index) {
    PostMessage(LVM_REDRAWITEMS, index, static_cast<LPARAM>(index));
}

// This callback is called in the working thread
void CNetworkDebugListView::onItemCountChanged(size_t index) {
    PostMessage(LVM_SETITEMCOUNT, index, 0L);
}

void CNetworkDebugListView::applyFilters() {
    std::vector<std::string> filters(NetworkDebugModelNS::COLUMN_COUNT);

    bool allEmpty = true;

    for (const auto& column: columns_) {
        HDITEM item {};
        item.mask = HDI_FILTER;
        int val = 0;
        TCHAR buffer[MAX_PATH] { '\0' };
        HDTEXTFILTER filter {};
        item.type = column.filterType;

        // Get values from filter edit boxes 
        if (column.filterType == HDFT_ISSTRING) {
            filter.pszText = buffer;
            filter.cchTextMax = std::size(buffer);
            item.pvFilter = &filter;
            if (GetHeader().GetItem(column.index, &item)) {
                filters[column.index] = W2U(buffer);
            }
        } else if (column.filterType == HDFT_ISNUMBER) {
            // HDFT_ISNUMBER type is not working properly 
            item.pvFilter = &val;
            if (GetHeader().GetItem(column.index, &item) && val) {
                filters[column.index] = std::to_string(val);
            }
        }
        if (!filters[column.index].empty()) {
            allEmpty = false;
        }
    }
    if (allEmpty) {
        filters.clear();
    }
    model_->applyFilter(filters);
}
