#ifndef GUI_CONTROLS_RESULTSLISTVIEW_H
#define GUI_CONTROLS_RESULTSLISTVIEW_H

#include "atlheaders.h"

class UploadListModel; 
/**
 ListView with no flickering
**/
class CResultsListView : public CWindowImpl<CResultsListView, CListViewCtrl> {
    public:

    BEGIN_MSG_MAP(CResultsListView)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnListViewNMCustomDraw)
    END_MSG_MAP()

    DECLARE_WND_SUPERCLASS(_T("CResultsListView"), CListViewCtrl::GetWndClassName())
    CResultsListView();
    void Init();
    void SetModel(UploadListModel* model);
    bool AttachToDlgItem(HWND parent, int dlgID);
    //LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnListViewNMCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
protected:
    void onRowChanged(size_t index);
    UploadListModel* model_;
}; // end class

#endif
