/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "HistoryTreeControl.h"

#include <utility>

#include <boost/format.hpp>

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/ServiceLocator.h"
#include "Gui/GuiTools.h"
#include "Func/IuCommonFunctions.h"
#include "Func/WinUtils.h"
#include "Core/LocalFileCache.h"
#include "Core/Images/Utils.h"
#include "Core/AppParams.h"
#include "Func/MyEngineList.h"
#include "Core/i18n/Translator.h"
#include "Core/AbstractServerIconCache.h"
#include "Core/TaskDispatcher.h"
#include "Gui/Helpers/DPIHelper.h"

// CHistoryTreeControl
CHistoryTreeControl::CHistoryTreeControl(std::shared_ptr<INetworkClientFactory> factory)
    : networkClientFactory_(std::move(factory))
{
    m_SessionItemHeight = 0;
    m_SubItemHeight = 0;
    downloading_enabled_ = true;
    m_bIsRunning = false;
}

CHistoryTreeControl::~CHistoryTreeControl()
{
    for (const auto& it : m_fileIconCache) {
        if (it.second) {
            DestroyIcon(it.second);
        }
    }
}

void CHistoryTreeControl::CreateDownloader() {
    if(!m_FileDownloader) {
        m_FileDownloader = std::make_unique<CFileDownloader>(networkClientFactory_, AppParams::instance()->tempDirectory());
        m_FileDownloader->setOnConfigureNetworkClientCallback([this](auto* nm) { OnConfigureNetworkClient(nm); });
        m_FileDownloader->setOnFileFinishedCallback([this](bool ok, int statusCode, const CFileDownloader::DownloadFileListItem& it) {
            OnFileFinished(ok, statusCode, it);
        });
        m_FileDownloader->setOnQueueFinishedCallback([this] { QueueFinishedEvent(); });
    }
}


LRESULT CHistoryTreeControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Init();
    return 0;
}

LRESULT CHistoryTreeControl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled)
{
    abortLoadingThreads();
    return 0;
}

LRESULT CHistoryTreeControl::OnThumbLoaded(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Invalidate();
    return 0;
}

void CHistoryTreeControl::abortLoadingThreads()
{
    {
        std::lock_guard<std::mutex> lk(m_thumbLoadingQueueMutex);
        m_thumbLoadingQueue.clear();
    }

    if(IsRunning()) {
        SignalStop();
    }

    if (m_FileDownloader && m_FileDownloader->isRunning()) {
        m_FileDownloader->stop();
    }

    if(IsRunning()) {
        SignalStop();
    }
}

void CHistoryTreeControl::Clear()
{
    /*SetRedraw(false);

    int n = GetCount();
    for(int i= 0; i<n; i++)
    {
            HistoryTreeControlItem * item =(HistoryTreeControlItem *)GetItemDataPtr(i);
            delete item;
    }
    ResetContent();
    SetRedraw(true);*/
}

void CHistoryTreeControl::addSubEntry(TreeItem* res, const HistoryItem* it, bool autoExpand)
{
    auto* it2 = new HistoryTreeItem();
    TreeItem *item = AddSubItem(Utf8ToWCstring(IuCoreUtils::TimeStampToString(it->timeStamp)+ " "+ it->localFilePath), res, it2, autoExpand);
    item->setCallback(this);
    it2->hi = it;
    it2->thumbnail = nullptr;
    it2->ThumbnailRequested = false;
    it2->treeItem = item;
}

DWORD CHistoryTreeControl::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
{
    return 0;
}

DWORD CHistoryTreeControl::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
    return CDRF_DODEFAULT;
}

const HistoryItem* CHistoryTreeControl::getItemData(const TreeItem* res)
{
    if (!res || res->level() != 1) {
        return nullptr;
    }
    auto* item = static_cast<HistoryTreeItem*> (res->userData());
    return item ? item->hi : nullptr;
}

void CHistoryTreeControl::setOnThreadsFinishedCallback(std::function<void()> cb) {
    onThreadsFinished_ = std::move(cb);
}

void CHistoryTreeControl::setOnThreadsStartedCallback(std::function<void()> cb) {
    onThreadsStarted_ = std::move(cb);
}

void CHistoryTreeControl::setOnItemDblClickCallback(std::function<void(TreeItem*)> cb) {
    onItemDblClick_ = std::move(cb);
}

HICON CHistoryTreeControl::getIconForExtension(const CString& fileName)
{
    CString ext = WinUtils::GetFileExt(fileName);
    if (ext.IsEmpty()) {
        return nullptr;
    }
    auto const it = m_fileIconCache.find(ext);
    if (it != m_fileIconCache.end()) {
        return it->second;
    }

    HICON res = WinUtils::GetAssociatedIcon(fileName, false);
    if (!res) {
        return nullptr;
    }
    m_fileIconCache[ext]=res;
    return res;
}

TreeItem*  CHistoryTreeControl::addEntry(CHistorySession* session, const CString& text)
{
    TreeItem *item = AddItem(text, session);
    return item;
}

void CHistoryTreeControl::_DrawItem(TreeItem* item, HDC hdc, DWORD itemState, RECT invRC, int* outHeight) {
    const int kPaddingX = 2;
    const int kPaddingY = 2;
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    CDCHandle dc(hdc);
    HFONT listboxFont = GetFont();

    HFONT oldFont = dc.SelectFont(listboxFont);

    // If outHeight parameter is set, do not actually draw, just calculate item's dimensions
    bool draw = !outHeight;

    //bool isSelected = (itemState & CDIS_SELECTED) || (itemState&CDIS_FOCUS);
    CRect clientRect;
    GetClientRect(clientRect);

    auto* ses = static_cast<CHistorySession*>(item->userData());
    std::string label = "[" + W2U(WinUtils::TimestampToString(ses->timeStamp())) + "]";
    std::string serverName = ses->serverName();
    if (ses->entriesCount()) {
        serverName = ses->entry(0).serverName;
    }
    if (serverName.empty()) {
        serverName = _("unknown server");
    }
    std::string filesText;
    try {
        filesText = str(IuStringUtils::FormatNoExcept(boost::locale::ngettext("%d file", "%d files", ses->entriesCount())) % ses->entriesCount());
    } catch (const std::exception& e) {
        LOG(ERROR) << e.what();
    }
    std::string lowText = serverName + " (" + filesText + ")";
    CString text = Utf8ToWCstring(label);

    CRect rc = invRC;
    CRect calcRect;

    dc.SetBkMode(TRANSPARENT);

    CBrush backgroundBrush;

    DWORD color = GetSysColor(COLOR_WINDOW);
    DWORD textColor = (itemState & CDIS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_WINDOWTEXT);

    if (itemState & CDIS_SELECTED) {
        color = GetSysColor(COLOR_HIGHLIGHT);
    }
    COLORREF grayColor = GuiTools::IsColorBright(color) ? GetSysColor(COLOR_GRAYTEXT) : RGB(210, 210, 210);
    backgroundBrush.CreateSolidBrush(color);
    if (draw)
        dc.FillRect(&invRC, backgroundBrush);
    COLORREF oldTextColor = dc.SetTextColor(grayColor);
    dc.DrawText(text, text.GetLength(), &calcRect, DT_CALCRECT);

    calcRect.OffsetRect(rc.left, rc.top);

    if (draw) {
        CRect dateRect = rc;
        dateRect.right -= 20;
        dateRect.top += kPaddingY;
        dateRect.bottom -= kPaddingY;
        //dateRect.OffsetRect(400,0);
        dc.DrawText(text, text.GetLength(), &dateRect, DT_RIGHT | DT_VCENTER);
    }

    int curX = 0;

    RECT gradientLineRect = invRC;
    gradientLineRect.bottom--;
    gradientLineRect.top = gradientLineRect.bottom;
    if (draw) {
        COLORREF bgColor = GetSysColor(COLOR_WINDOW);
        GuiTools::FillRectGradient(hdc, gradientLineRect, GuiTools::IsColorBright(bgColor) ? GetSysColor(COLOR_GRAYTEXT): RGB(210,210,210), bgColor, true);
    }

    calcRect = rc;
    dc.SetTextColor(textColor);
    CString lowTextW = Utf8ToWCstring(lowText);
    dc.DrawText(lowTextW, lowTextW.GetLength(), &calcRect, DT_CALCRECT);
    if (draw) {
        bool isItemExpanded = item->IsExpanded();
        //(GetItemState(item,TVIS_EXPANDED)&TVIS_EXPANDED);
        CRect plusIconRect;
        SIZE plusIconSize = {9, 9};
        HTHEME theme = OpenThemeData(_T("treeview"));
        if (theme) {
            GetThemePartSize(dc.m_hDC, TVP_GLYPH, isItemExpanded ? GLPS_OPENED : GLPS_CLOSED, 0, TS_DRAW,
                             &plusIconSize);
        }

        int iconOffsetX = 3;
        int iconOffsetY = (rc.Height() - plusIconSize.cy) / 2;
        plusIconRect.left = rc.left + iconOffsetX;
        plusIconRect.top = rc.top + iconOffsetY;
        plusIconRect.right = plusIconRect.left + plusIconSize.cx;
        plusIconRect.bottom = plusIconRect.top + plusIconSize.cy;
        curX += iconOffsetX + plusIconSize.cx;
        if (theme) {
            DrawThemeBackground(dc.m_hDC, TVP_GLYPH, isItemExpanded ? GLPS_OPENED : GLPS_CLOSED, &plusIconRect);
            CloseThemeData();
        } else {
            CRect FrameRect(plusIconRect);
            dc.FillSolidRect(FrameRect, 0x808080);
            FrameRect.DeflateRect(1, 1, 1, 1);
            dc.FillSolidRect(FrameRect, 0xFFFFFF);

            CRect MinusRect(2, 4, 7, 5);
            MinusRect.OffsetRect(plusIconRect.TopLeft());
            dc.FillSolidRect(MinusRect, 0x000000);

            if (! isItemExpanded) {
                CRect PlusRect(4, 2, 5, 7);
                PlusRect.OffsetRect(plusIconRect.TopLeft());
                dc.FillSolidRect(PlusRect, 0x000000);
            }
        }
    }

    CRect drawRect;
    int serverIconOffsetY = (rc.Height() - 1 - 16) / 2;
    drawRect.top = rc.top + serverIconOffsetY;
    drawRect.bottom = drawRect.top + calcRect.Height();
    drawRect.left = rc.left + curX + 4;
    drawRect.right = drawRect.left + calcRect.Width();
    HICON ico = ServiceLocator::instance()->serverIconCache()->getIconForServer(ses->serverName(), dpi, true);

    if (ico && draw) {
        dc.DrawIconEx(drawRect.left, drawRect.top, ico, 16, 16);
    }
    drawRect.OffsetRect(19, 0);

    CRect textRect(drawRect.left, rc.top + kPaddingY, drawRect.left + kPaddingX  + 19 + calcRect.Width(),
        rc.top + kPaddingY + calcRect.Height());
    if (draw) {
        dc.DrawText(lowTextW, lowTextW.GetLength(), &textRect, DT_LEFT | DT_VCENTER);
    }

    dc.SelectFont(oldFont);
    dc.SetTextColor(oldTextColor);

    int itemHeight = std::max<int>(textRect.Height() + kPaddingY * 2, 16);

    if (outHeight) {
        *outHeight = itemHeight;
    }
}

int CHistoryTreeControl::CalcItemHeight(TreeItem* item)
{
    bool isRootItem = (item->level()==0);
    if(isRootItem && m_SessionItemHeight)
    {
        return m_SessionItemHeight;
    }
    else if(!isRootItem && m_SubItemHeight)
    {
        return m_SubItemHeight;
    }

    int res = 0;
    HDC dc =  GetDC();
    RECT rc;
    GetItemRect(FindVisibleItemIndex(item), &rc);
    if( !isRootItem)
    {
        DrawSubItem(item,  dc, 0, rc, &res);
        //m_SubItemHeight = res;
    }
    else
    {
        _DrawItem(item,  dc, 0, rc, &res);
        //m_SessionItemHeight = res;
    }

    ReleaseDC(dc);
    return res;
}

void CHistoryTreeControl::DrawBitmap(HDC hdc, HBITMAP bmp, int x, int y)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    BITMAP bm;
    auto hbmOld = static_cast<HBITMAP>(SelectObject(hdcMem, bmp));
    GetObject(bmp, sizeof(bm), &bm);
    BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}

void CHistoryTreeControl::DrawSubItem(TreeItem* item, HDC hdc, DWORD itemState, RECT invRC, int* outHeight) {
    const int kPaddingX = 10, kPaddingY = 2;
    RECT rc = invRC;
    bool draw = !outHeight;
    CDCHandle dc(hdc);

    HFONT listboxFont = GetFont();

    HFONT oldFont = dc.SelectFont(listboxFont);

    CBrush br;
    COLORREF oldTextColor {};

    COLORREF textColor = GetSysColor((itemState & CDIS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);

    if (draw) {
        dc.SetBkMode(TRANSPARENT);
        oldTextColor = dc.SetTextColor(textColor);
    }

    CBrush backgroundBrush;
    backgroundBrush.CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    CBrush selectedBrush;
    selectedBrush.CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
    COLORREF itemBgColor = (itemState & CDIS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW);
    COLORREF grayColor = GuiTools::IsColorBright(itemBgColor) ? GetSysColor(COLOR_GRAYTEXT) : RGB(210, 210, 210);

    if (draw) {
        CRect rc2 = rc;

        if (!(itemState & CDIS_SELECTED)) {

            rc2.left -= CATEGORY_INDENT;
            dc.FillRect(&rc2, backgroundBrush);
        } else {
            CRect rc3 = rc2;
            dc.FillRect(&rc3, selectedBrush);
            rc3.left -= CATEGORY_INDENT;
            rc3.right = rc3.left + CATEGORY_INDENT;
            dc.FillRect(&rc3, backgroundBrush);
        }
        //dc.FillRect(&rc2, backgroundBrush);
    }

    br.CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
    RECT thumbRect;
    thumbRect.left = rc.left + kPaddingX;
    thumbRect.top = rc.top + kPaddingY;
    thumbRect.bottom = thumbRect.top + thumbHeight_+ 2;
    thumbRect.right = thumbRect.left + thumbWidth_ + 2;

    if (draw)
        dc.FrameRect(&thumbRect, br);
    const HistoryItem* it2 = getItemData(item);
    std::string fileName = it2 ? IuCoreUtils::ExtractFileName(it2->localFilePath) : "";

    CString iconSourceFileName = it2 ? Utf8ToWCstring(it2->localFilePath) : "";
    if (iconSourceFileName.IsEmpty() && it2)
        iconSourceFileName = Utf8ToWCstring(it2->directUrl);
    HICON ico = getIconForExtension(iconSourceFileName);
    CString text = Utf8ToWstring(fileName).c_str();
    GuiTools::IconInfo info = GuiTools::GetIconInfo(ico);
    int iconWidth = info.nWidth;
    int iconHeight = info.nHeight;
    auto* hti = static_cast<HistoryTreeItem*>(item->userData());

    if (draw) {
        /*HBITMAP bm = */
        GetItemThumbnail(hti);
        if (hti->thumbnail) {
            DrawBitmap(dc, hti->thumbnail, thumbRect.left + 1, thumbRect.top + 1);
        } else if (ico != 0)
            dc.DrawIcon(thumbRect.left + 1 + (thumbHeight_ - iconWidth) / 2,
                        thumbRect.top + 1 + (thumbWidth_ - iconHeight) / 2, ico);
    }

    CRect calcRect = invRC;
    calcRect.left = thumbRect.right + 5;
    dc.DrawText(text, text.GetLength(), &calcRect, DT_CALCRECT);
    int filenameHeight = calcRect.Height();
    if (draw) {
        dc.DrawText(text, text.GetLength(), &calcRect, DT_LEFT);
    }

    CRect urlRect = invRC;
    urlRect.left = calcRect.left;
    urlRect.top += filenameHeight + 3;

    CString url = it2 ? Utf8ToWCstring(it2->directUrl.length() ? it2->directUrl : it2->viewUrl) : CString();

    if (draw) {
        dc.SetTextColor(grayColor);
        dc.DrawText(url, url.GetLength(), &urlRect, DT_LEFT);
        dc.SetTextColor(oldTextColor);
    }
    dc.SelectFont(oldFont);

    if (outHeight) {
        *outHeight = thumbHeight_ + kPaddingY * 2 + 2;
    }
}

bool CHistoryTreeControl::IsItemAtPos(int x, int y, bool &isRoot)
{
    if(x > 50) x = 50;
    POINT pt = {x, y};
    BOOL out;
    int index = ItemFromPoint(pt, out);
    if(out) return false;
    TreeItem* item = GetItem(index);

    isRoot = item->level()==0;
    return true;
}

void CHistoryTreeControl::DrawTreeItem(HDC dc, RECT rc, UINT itemState,  TreeItem *item)
{
    if( item->level()>0)
    {
        DrawSubItem(item,  dc, itemState, rc, 0);
    }
    else
    {
        _DrawItem(item,  dc, itemState, rc, 0);
    }
}

void CHistoryTreeControl::TreeItemSize( TreeItem *item, SIZE *sz)
{
    int height = CalcItemHeight(item);
    sz->cy = height;
}

TreeItem * CHistoryTreeControl::selectedItem()
{
    int idx = GetCurSel();
    if( idx != -1 )
    {
        TreeItem* prop = GetItem(idx);
        return prop;
    }
    return nullptr;
}

bool CHistoryTreeControl::LoadThumbnail(HistoryTreeItem * item)
{
    using namespace Gdiplus;

    Image* bm = nullptr;
    std::unique_ptr<GdiPlusImage> srcImg;

    CString filename ;
    if (!item->thumbnailSource.empty()) {
        filename = Utf8ToWCstring(item->thumbnailSource);
    } else {
        filename = Utf8ToWCstring(item->hi->localFilePath);
    }
    bool error = false;
    if(IuCommonFunctions::IsImage(filename))
    {
        srcImg = ImageUtils::LoadImageFromFileExtended(filename);
        if (!srcImg) {
            return false;
        }
        bm = srcImg->getBitmap();
        if (!bm) {
            return false;
        }
    }

    int width,height,imgwidth,imgheight,newwidth,newheight;
    width= thumbWidth_/*rc.right-2*/;
    height= thumbHeight_/*rc.bottom-16*/;
    int thumbwidth= thumbWidth_;
    int  thumbheight= thumbHeight_;

    if(bm)
    {
        imgwidth=bm->GetWidth();
        imgheight=bm->GetHeight();
//        if(imgwidth>maxwidth) maxwidth=imgwidth;
        //if(imgheight>maxheight) maxheight=imgheight;

        if((float)imgwidth/imgheight>(float)width/height)
        {
            if(imgwidth<=width)
            {
                newwidth=imgwidth;
                newheight=imgheight;
            }
            else
            {
                newwidth=width;
                newheight=int((float)newwidth/imgwidth*imgheight);}
        }
        else
        {
            if(imgheight<=height)
            {
                newwidth=imgwidth;
                newheight=imgheight;
            }
            else
            {
                newheight=height;
                newwidth=int((float)newheight/imgheight*imgwidth);
            }
        }
    }

    Graphics g(m_hWnd,true);
    Bitmap imgBuffer(thumbwidth, thumbheight, &g);
    Graphics gr(&imgBuffer);
    gr.SetPixelOffsetMode(PixelOffsetModeHalf);
    ImageAttributes attr;
    attr.SetWrapMode(WrapModeTileFlipXY);
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic );
    gr.Clear(Color(255,255,255,255));

    RectF bounds(0, 0, float(width), float(height));

    if(bm && !bm->GetWidth() && item)
    {
        error = true;
    }
    else
    {
        LinearGradientBrush br(bounds, Color(255, 255, 255, 255), Color(255, 210, 210, 210), LinearGradientModeBackwardDiagonal);

        if(IuCommonFunctions::IsImage(filename))
            gr.FillRectangle(&br,0, 0, width, height);
        gr.SetInterpolationMode(InterpolationModeHighQualityBicubic );

        if(bm)
                gr.DrawImage(/*backBuffer*/bm, (int)((width-newwidth)/2), (int)((height-newheight)/2), (int)newwidth,(int)newheight);


        RectF bounds(0, float(height), float(width), float(20));

//        if (ExtendedView)
        {
            LinearGradientBrush
                br2(bounds, Color(240, 255, 255, 255), Color(200, 210, 210, 210),
                LinearGradientModeBackwardDiagonal /*LinearGradientModeVertical*/);
            gr.FillRectangle(&br2,(float)1, (float)height+1, (float)width-2, (float)height+20-1);
        }

        if(item)
        {
            SolidBrush brush(Color(255,0,0,0));
            StringFormat format;
            format.SetAlignment(StringAlignmentCenter);
            format.SetLineAlignment(StringAlignmentCenter);
            Font font(L"Tahoma", 8, FontStyleRegular );
            LPCTSTR Filename = filename;
            CString Buffer;
            int64_t fileSize = IuCoreUtils::GetFileSize(W2U(filename));
            WCHAR buf2[25];
            WinUtils::NewBytesToString(fileSize, buf2, 25);
            CString FileExt = WinUtils::GetFileExt(Filename);
            if(!lstrcmpi(FileExt, _T("jpg")))
                FileExt = _T("JPEG");
            if(IuCommonFunctions::IsImage(filename) && bm)
            {
                Buffer.Format( _T("%s %dx%d (%s)"), FileExt.GetString(), (int)bm->GetWidth(), (int)bm->GetHeight(), (LPCTSTR)buf2);
            }
            else
            {
                Buffer = buf2;
            }
            gr.DrawString(Buffer, -1, &font, bounds, &format, &brush);
        }
    }

    HBITMAP bmp = nullptr;
    imgBuffer.GetHBITMAP(Color(255,255,255), &bmp);
    item->thumbnail = error?0:bmp;
    return !error;
}

LRESULT CHistoryTreeControl::OnDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    int idx = GetCurSel();
    if (idx != -1) {
        TreeItem* prop = GetItem(idx);
        ATLASSERT(prop);
        if (onItemDblClick_) {
            onItemDblClick_(prop);
        }
    }
    return CCustomTreeControlImpl<CHistoryTreeControl>::OnDblClick(uMsg, wParam, lParam, bHandled);
}

LRESULT CHistoryTreeControl::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam != VK_TAB && wParam != VK_ESCAPE) {
        return DLGC_WANTALLKEYS;
    }
    return 0;
}

HBITMAP CHistoryTreeControl::GetItemThumbnail(HistoryTreeItem* item)
{
    if(item->thumbnail!=0)
        return item->thumbnail;

    if (m_bStopped) return 0;
    if(item->ThumbnailRequested) return 0;

    item->ThumbnailRequested = true;


    std::string stdLocalFileName = item->hi->localFilePath;
    CString localFileName = Utf8ToWCstring(stdLocalFileName);
    if(!IuCommonFunctions::IsImage(localFileName))
    {
        return 0;
    }

    if(IuCoreUtils::FileExists(stdLocalFileName))
    {
        {
            std::lock_guard<std::mutex> lk(m_thumbLoadingQueueMutex);
            m_thumbLoadingQueue.push_back(item);
        }

        StartLoadingThumbnails();
    }
    else
    {
        DownloadThumb(item);
    }
    return 0;
}

void CHistoryTreeControl::DownloadThumb(HistoryTreeItem * it)
{
    if(m_bStopped) return;
    if(it->thumbnailSource.empty())
    {
        std::string thumbUrl = it->hi->thumbUrl;
        if(thumbUrl.empty()) return ;
        std::string cacheFile = ServiceLocator::instance()->localFileCache()->get(thumbUrl);
        if(!cacheFile.empty()) {
            it->thumbnailSource = cacheFile;
            m_thumbLoadingQueueMutex.lock();
            m_thumbLoadingQueue.push_back(it);
            m_thumbLoadingQueueMutex.unlock();
            StartLoadingThumbnails();
            return;
        }
        if(downloading_enabled_)
        {
            CreateDownloader();
            m_FileDownloader->addFile(thumbUrl, it);
            if(onThreadsStarted_)
                onThreadsStarted_();
            m_FileDownloader->start();
        }
    }
}
DWORD CHistoryTreeControl::Run()
{
    while(!m_thumbLoadingQueue.empty())
    {
        if(m_bStopped) break;
        m_thumbLoadingQueueMutex.lock();
        HistoryTreeItem * it = m_thumbLoadingQueue.front();
        m_thumbLoadingQueue.pop_front();
        m_thumbLoadingQueueMutex.unlock();
        if(!LoadThumbnail(it) && it->thumbnailSource.empty())
        {
            // Try downloading it
            DownloadThumb(it);
        }
        else {
            /* ServiceLocator::instance()->taskRunner()->runInGuiThread([this, treeItem = it->treeItem] {
                RECT rc {};
                if (GetItemRect(FindVisibleItemIndex(treeItem), &rc) != LB_ERR) {
                    InvalidateRect(&rc);
                }
            },
            true);*/

            PostMessage(WM_APP_MY_THUMBLOADED, 0, 0);
        }
    }
    m_bIsRunning = false;
    if(!m_FileDownloader || !m_FileDownloader->isRunning())
        threadsFinished();
    return 0;
}

void CHistoryTreeControl::StartLoadingThumbnails()
{
    if(!IsRunning())
    {
        if(onThreadsStarted_)
            onThreadsStarted_();
        m_bIsRunning = true;

        // Release thread handle if ran before
        Release();

        this->Start();
    }
}

bool CHistoryTreeControl::OnFileFinished(bool ok, int statusCode, const CFileDownloader::DownloadFileListItem& it)
{
    if(ok && !it.fileName.empty())
    {
        auto* hit = static_cast<HistoryTreeItem*>(it.id);
        if(hit) {
            hit->thumbnailSource = it.fileName;
            ServiceLocator::instance()->localFileCache()->addFile(it.url, it.fileName);
            {
                std::lock_guard<std::mutex> lk(m_thumbLoadingQueueMutex);
                m_thumbLoadingQueue.push_back(hit);
            }

            StartLoadingThumbnails();
        }
    }
    return true;
}

void CHistoryTreeControl::OnTreeItemDelete(TreeItem* item)
{
    auto* hti = static_cast<HistoryTreeItem*> (item->userData());
    if (!hti) {
        return;
    }
    HBITMAP bm = hti->thumbnail;
    if (bm) {
        DeleteObject(bm);
    }
    if (!m_thumbLoadingQueue.empty()) {
        LOG(WARNING) << "m_thumbLoadingQueue is not empty";
    }
    item->setUserData(nullptr);
    delete hti;
}

void CHistoryTreeControl::threadsFinished()
{
    {
        std::lock_guard<std::mutex> lk(m_thumbLoadingQueueMutex);
        m_thumbLoadingQueue.clear();
    }

    if (onThreadsFinished_) {
        onThreadsFinished_();
    }
}

void CHistoryTreeControl::QueueFinishedEvent()
{
    if(!IsRunning())
        threadsFinished();
}

bool CHistoryTreeControl::isRunning() const
{
    return (m_bIsRunning || (m_FileDownloader && m_FileDownloader->isRunning()) );
}

void CHistoryTreeControl::setDownloadingEnabled(bool enabled)
{
    downloading_enabled_ = enabled;
}

void CHistoryTreeControl::ResetContent()
{
    if(m_bIsRunning || (m_FileDownloader && m_FileDownloader->isRunning()))
    {
        LOG(ERROR) << _T("Cannot reset list while threads are still running!");
        return;
    }
    {
        std::lock_guard<std::mutex> lk(m_thumbLoadingQueueMutex);
        m_thumbLoadingQueue.clear();
    }

    CCustomTreeControlImpl<CHistoryTreeControl>::ResetContent();
}

void CHistoryTreeControl::OnConfigureNetworkClient(INetworkClient* nm) {
    nm->setTreatErrorsAsWarnings(true); // no need to bother user with download errors
}

BOOL CHistoryTreeControl::SubclassWindow(HWND hWnd) {
    BOOL bRet = CCustomTreeControlImpl<CHistoryTreeControl>::SubclassWindow(hWnd);
    if (bRet) {
        Init();
    }
    return bRet;
}

void CHistoryTreeControl::Init() {
    CClientDC dc(m_hWnd);
    int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    int dpiY = dc.GetDeviceCaps(LOGPIXELSY);

    thumbWidth_ = MulDiv(kThumbSize, dpiX, USER_DEFAULT_SCREEN_DPI);
    thumbHeight_ = MulDiv(kThumbSize, dpiY, USER_DEFAULT_SCREEN_DPI);
}
