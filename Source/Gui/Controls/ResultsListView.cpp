#include "ResultsListView.h"

#include <strsafe.h>

#include "Gui/Models/UploadListModel.h"
#include "Core/i18n/Translator.h"

CResultsListView::CResultsListView() :  model_(nullptr) {
}

bool CResultsListView::AttachToDlgItem(HWND parent, int dlgID) {

    HWND hWnd = ::GetDlgItem(parent, dlgID);
    bool res = SubclassWindow(hWnd) != FALSE;
    Init();
    return res;
}

void CResultsListView::Init() {
}

void CResultsListView::SetModel(UploadListModel* model) {
    if (model_ == model) {
        return;
    }
    model_ = model;
    using namespace std::placeholders;
    SetItemCount(model_ ? static_cast<int>(model_->getCount()) : 0);
    if (model_) {
        model_->setOnRowChangedCallback([this](size_t index) { onRowChanged(index); });
    }
}

LRESULT CResultsListView::OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* pDispInfo = reinterpret_cast<LV_DISPINFO*>(pnmh);
    LV_ITEM* pItem = &pDispInfo->item;
    DWORD n = pItem->iItem;

    if (pItem->mask & LVIF_TEXT) {
        CString str = model_->getItemText(n, pItem->iSubItem);
        StringCchCopy(pItem->pszText, pItem->cchTextMax, str);
    }

    return 0;
}

// Disabled in MSG MAP
LRESULT CResultsListView::OnListViewNMCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    auto* lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);

    switch (lplvcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
        {
            return CDRF_NOTIFYSUBITEMDRAW;
            /*auto color = model_->getItemTextColor(lplvcd->nmcd.dwItemSpec);
            if (color) {
                lplvcd->clrText = color;
                //return CDRF_NEWFONT;
                
            }*/
        }
        break;
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
            lplvcd->clrText = model_->getItemTextColor(lplvcd->nmcd.dwItemSpec, lplvcd->iSubItem);
            return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}

// Called from the worker thread
void CResultsListView::onRowChanged(size_t index) {
    PostMessage(LVM_REDRAWITEMS, index, static_cast<LPARAM>(index));
}
