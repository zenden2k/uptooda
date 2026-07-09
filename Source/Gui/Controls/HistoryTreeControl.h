#pragma once
#include <queue>
#include <map>
#include <mutex>
#include <memory>

#include "atlheaders.h"
#include "3rdpart/thread.h"
#include "Core/HistoryManager.h"
#include "Core/FileDownloader.h"
#include "CustomTreeControl.h"

constexpr auto WM_APP_MY_THUMBLOADED = WM_USER + 2000;

class INetworkClientFactory;

struct HistoryTreeItem
{
    const HistoryItem* hi;
    HBITMAP thumbnail;
    bool ThumbnailRequested;
    std::string thumbnailSource;
    TreeItem* treeItem = nullptr;

    HistoryTreeItem() :hi(nullptr), thumbnail(nullptr), ThumbnailRequested(false){
        
    }
};

class CHistoryTreeControlCallback {
public:
    virtual void OnItemDblClick(TreeItem* item) = 0;
    virtual ~CHistoryTreeControlCallback() = default;
};

class CHistoryTreeControl :
    public CCustomTreeControlImpl<CHistoryTreeControl>,
    public CThemeImpl <CHistoryTreeControl>,
    protected CThreadImpl<CHistoryTreeControl>,
    public TreeItemCallback
{
    public:
        CHistoryTreeControl(std::shared_ptr<INetworkClientFactory> factory);
        ~CHistoryTreeControl();
        //DECLARE_WND_SUPERCLASS(_T("CHistoryTreeControl"), CListBox/**/CListViewCtrl*/::GetWndClassName())

        BEGIN_MSG_MAP(CHistoryTreeControl)
            MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
            MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
            MESSAGE_HANDLER(WM_APP_MY_THUMBLOADED, OnThumbLoaded)
            CHAIN_MSG_MAP(CCustomTreeControlImpl<CHistoryTreeControl>)
        END_MSG_MAP()

        const int kThumbSize = 48;

        BOOL SubclassWindow(HWND hWnd);
        void Init();

        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnThumbLoaded(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        void DrawTreeItem(HDC dc, RECT rc, UINT itemState,  TreeItem* item) override;
        DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
        DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
        bool LoadThumbnail(HistoryTreeItem* ItemID);
        LRESULT OnDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) override;
        LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        void setDownloadingEnabled(bool enabled);
        bool m_bIsRunning;
        bool downloading_enabled_;
        void addSubEntry(TreeItem* res, const HistoryItem* it, bool autoExpand);
        TreeItem*  addEntry(CHistorySession* session, const CString& text);
        void Clear();
        bool IsItemAtPos(int x, int y, bool& isRoot);
        TreeItem* selectedItem();
        void TreeItemSize( TreeItem* item, SIZE* sz) override;
        DWORD Run();
        bool isRunning() const;
        void StartLoadingThumbnails();
        void OnTreeItemDelete(TreeItem* item) override;
        void CreateDownloader();
        void abortLoadingThreads();
        void ResetContent();
        static const HistoryItem* getItemData(const TreeItem* res);
        void setOnThreadsFinishedCallback(std::function<void()> cb);
        void setOnThreadsStartedCallback(std::function<void()> cb);
        void setOnItemDblClickCallback(std::function<void(TreeItem*)> cb);

    private:
        std::map<CString, HICON> m_fileIconCache;
        HICON getIconForExtension(const CString& fileName);
        int CalcItemHeight(TreeItem* item);

        // If outHeight parameter is set, do not actually draw, just calculate item's dimensions
        void _DrawItem(TreeItem* res, HDC dc, DWORD itemState, RECT invRC, int* outHeight);
        void DrawSubItem(TreeItem* res, HDC dc, DWORD itemState, RECT invRC,  int* outHeight);
        HBITMAP GetItemThumbnail(HistoryTreeItem* item);
        std::deque<HistoryTreeItem*> m_thumbLoadingQueue;
        std::mutex m_thumbLoadingQueueMutex;
        std::unique_ptr<CFileDownloader> m_FileDownloader;
        std::shared_ptr<INetworkClientFactory> networkClientFactory_;
        std::function<void()> onThreadsFinished_, onThreadsStarted_;
        std::function<void(TreeItem*)> onItemDblClick_;
        bool OnFileFinished(bool ok, int statusCode, const CFileDownloader::DownloadFileListItem& it);
        void DownloadThumb(HistoryTreeItem* it);
        int m_SessionItemHeight;
        int m_SubItemHeight;
        void QueueFinishedEvent();
        void threadsFinished();
        void OnConfigureNetworkClient(INetworkClient* nm);
        int thumbWidth_ = 0;
        int thumbHeight_ = 0;
        static void DrawBitmap(HDC hdc, HBITMAP bmp, int x, int y);
};
