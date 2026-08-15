#include "ServerCheckerListView.h"

#include <strsafe.h>

#include "ServersCheckerModel.h"
#include "Core/Utils/CoreUtils.h"

namespace ServersListTool {

CServerCheckerListView::CServerCheckerListView(ServersCheckerModel* model) : model_(model){
    using namespace std::placeholders;
    model_->setOnRowChangedCallback([this](auto index) { onRowChanged(index); });
}

void CServerCheckerListView::Init() {
    SetItemCount(static_cast<int>(model_->getCount()));

    AddColumn(_T("N"), 0);
    AddColumn(_T("Server"), 1);
    AddColumn(_T("Status"), 2);
    AddColumn(_T("Direct URL"), 3);
    AddColumn(_T("Thumb URL"), 4);
    AddColumn(_T("View URL"), 5);
    AddColumn(_T("Time"), 6);

    CClientDC dc(m_hWnd);
    int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    //int dpiY = dc.GetDeviceCaps(LOGPIXELSY);

    SetColumnWidth(0, MulDiv(25, dpiX, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(1, MulDiv(130, dpiX, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(2, MulDiv(80, dpiX, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(3, MulDiv(170, dpiX, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(4, MulDiv(170, dpiX, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(5, MulDiv(170, dpiX, USER_DEFAULT_SCREEN_DPI));
    SetColumnWidth(6, MulDiv(40, dpiX, USER_DEFAULT_SCREEN_DPI));
}

LRESULT CServerCheckerListView::OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* pDispInfo = reinterpret_cast<LV_DISPINFO*>(pnmh);
    LV_ITEM* pItem = &pDispInfo->item;

    if (pItem->mask & LVIF_TEXT)  {
        std::string str = model_->getItemText(pItem->iItem, pItem->iSubItem);
        std::wstring wstr = IuCoreUtils::Utf8ToWstring(str);
        StringCchCopy(pItem->pszText, pItem->cchTextMax, wstr.c_str());
    }

    return 0;
}

LRESULT CServerCheckerListView::OnListViewNMCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    auto* lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);

    switch (lplvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
    {
        auto color = model_->getItemColor(lplvcd->nmcd.dwItemSpec);

        if (color) {
            lplvcd->clrTextBk = color;
            return CDRF_NEWFONT;
        }
    }
    break;
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
        lplvcd->clrText = RGB(255, 0, 0);
        return CDRF_NEWFONT;
    }
    return 0;
}

void CServerCheckerListView::onRowChanged(size_t index) {
    PostMessage(LVM_REDRAWITEMS, index, static_cast<LPARAM>(index));
}
}
