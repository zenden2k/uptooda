#ifndef IU_SERVERLISTTOOL_SERVERLISTVIEW_H
#define IU_SERVERLISTTOOL_SERVERLISTVIEW_H

#pragma once
#include "atlheaders.h"

class ServerListModel;
class WinServerIconCache;

class CServerListView :
    public CWindowImpl<CServerListView, CListViewCtrl> {
public:
    using TParent = CWindowImpl<CServerListView, CListViewCtrl>;

    CServerListView(ServerListModel* model, WinServerIconCache* serverIconCache);
    ~CServerListView();
    DECLARE_WND_SUPERCLASS(_T("CServerListView"), CListViewCtrl::GetWndClassName())

    BEGIN_MSG_MAP(CServerListView)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_ODFINDITEM, OnOdFindItem)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnMyDpiChanged)
        //REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnListViewNMCustomDraw)
        //REFLECTED_NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
        //REFLECTED_NOTIFY_CODE_HANDLER(LVN_DELETEALLITEMS, OnDeleteItem)
        //REFLECTED_NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
    END_MSG_MAP()

    BOOL SubclassWindow(HWND hWnd);

    void Init();
    int SetView(DWORD dwView);
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnOdFindItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnListViewNMCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnMyDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    ServerListModel* model_;
    void onRowChanged(size_t index);
    void setColumnWidths();
    void createResources();
    CImageList serverIconImageList_;
    std::vector<int> serverIconImageListIndexes_;
    WinServerIconCache* serverIconCache_;
    int FindItemByString(LPCWSTR searchText, int startIndex, DWORD flags);
    int FindItemByPartialString(LPCWSTR searchText, int startIndex);
    int FindItemByParam(LPARAM searchParam, int startIndex);
    static UINT columns[1];
};


#endif
