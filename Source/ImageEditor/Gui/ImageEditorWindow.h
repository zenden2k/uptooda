#ifndef IMAGEEDITOR_MAINFRM_H
#define IMAGEEDITOR_MAINFRM_H

#pragma once

#include <memory>
#include <map>
#include <unordered_map>

#include "atlheaders.h"
#include "resource.h"
#include "ImageEditorView.h"
#include "ImageEditor/Document.h"
#include "Toolbar.h"
#include "TextParamsWindow.h"
#include "Core/Utils/CoreTypes.h"
#include "3rdpart/GdiplusH.h"
#include "Core/SearchByImage.h"

namespace ImageEditor {

class ColorsDelegate;

class ConfigurationProvider;

class ImageEditorWindow : public CWindowImpl<ImageEditorWindow>, CMessageFilter
{
public:
    DECLARE_WND_CLASS_EX(_T("ImageEditorWindow"), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, COLOR_APPWORKSPACE)
    enum {
        ID_UNDO = 1000,
        ID_CLOSE,
        ID_ADDTOWIZARD,
        ID_UPLOAD,
        ID_SHARE,
        ID_SAVE,
        ID_SAVEAS,
        ID_COPYBITMAPTOCLIBOARD,
        ID_COPYBITMAPTOCLIBOARD_ALT,
        ID_COPYBITMAPTOCLIBOARDASDATAURI,
        ID_COPYBITMAPTOCLIBOARDASDATAURI_ALT,
        ID_COPYBITMAPTOCLIBOARDASDATAURIHTML,
        ID_COPYBITMAPTOCLIBOARDASDATAURIHTML_ALT,
        ID_UNSELECTALL,
        ID_INCREASEPENSIZE,
        ID_DECREASEPENSIZE,
        ID_PRINTIMAGE,
        ID_SEARCHBYIMAGE,
        ID_SEARCHBYIMAGE_ALT,
        ID_ROTATECLOCKWISE,
        ID_ROTATECOUNTERCLOCKWISE, ID_FLIPVERTICAL, ID_FLIPHORIZONTAL, ID_MOREACTIONS, ID_CONTINUE,
        ID_SEARCHBYIMAGE_START = 1400,
        ID_SEARCHBYIMAGE_END = 1499,
        ID_SEARCHBYIMAGE_ALT_START = 1500,
        ID_SEARCHBYIMAGE_ALT_END = 1599,
        ID_DELETESELECTED, ID_RECORDSCREEN,
        ID_PEN = 1700,
        ID_BRUSH, ID_MARKER,ID_BLUR, ID_BLURRINGRECTANGLE, ID_PIXELATERECTANGLE, ID_LINE, ID_ARROW, ID_RECTANGLE,  ID_ROUNDEDRECTANGLE, ID_ELLIPSE,
        ID_FILLEDRECTANGLE, ID_FILLEDROUNDEDRECTANGLE, ID_FILLEDELLIPSE, ID_COLORPICKER, ID_CROP , ID_SELECTION,ID_TEXT, ID_STEPNUMBER, ID_MOVE /* ID_MOVE should be last */
    };

    enum DrawingToolHotkey {kMoveKey = 'V', kBrushKey = 'B', kTextKey = 'T', kRectangleKey = 'U', kColorPickerKey = 'I', kCropKey = 'C', // photoshop keys
        kMarkerKey = 'H', kBlurringRectangleKey = 'R', kArrowKey = 'A', kLineKey = 'L', kFilledRectangle = 'G', kStepNumber = 'E'
        // if you add an item here, do not forget to add it to drawingToolsHotkeys_ map
    };
    struct MenuItem {
        int menuItemId;
        int toolId;
    };

    struct SubMenuItem {
        int command;
        int parentCommand;
        std::shared_ptr<Gdiplus::Bitmap> icon;
        CString hint;
    };

    enum { kCanvasMargin = 4 , kToolbarOffset = 6}; // margin between toolbars and canvas in windowed mode

    enum DialogResult{
        drCancel, drAddToWizard, drUpload, drShare, drSave, drCopiedToClipboard, drPrintRequested, drSearch, drRecordScreen, drContinue
    };
    enum class ClipboardFormat{ None, Bitmap, DataUri, DataUriHtml };
    enum WindowDisplayMode {
        wdmAuto, wdmFullscreen, wdmWindowed
    };

    CImageEditorView m_view;

    ImageEditorWindow(std::shared_ptr<Gdiplus::Bitmap> bitmap, bool hasTransparentPixels, ConfigurationProvider* configurationProvider/* = 0*/, bool onlySelectRegion = false);
    ImageEditorWindow(CString imageFileName, ConfigurationProvider* configurationProvider/* = 0*/);
    ~ImageEditorWindow() override;
    void setInitialDrawingTool(DrawingToolType dt);
    void showUploadButton(bool show);
    void showAddToWizardButton(bool show);
    void setSuggestedFileName(CString fileName);
    std::shared_ptr<Gdiplus::Bitmap> getResultingBitmap() const;
    Gdiplus::Rect lastAppliedCrop() const;
    CRect getSelectedRect() const;
    /**
     * Set server name which is being displayed on upload button
     */
    void setServerDisplayName(const CString & serverName);
    void setAskBeforeClose(bool ask);

    DialogResult DoModal(HWND parent, HMONITOR screenshotsMonitor, WindowDisplayMode mode = wdmAuto, bool forceShowParent = false);

    BEGIN_MSG_MAP(ImageEditorWindow)
        MESSAGE_HANDLER(WM_NCCREATE, OnNcCreate)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER( WM_KEYDOWN, OnKeyDown )
        MESSAGE_HANDLER( WM_KEYUP, OnKeyUp )
        MESSAGE_HANDLER( WM_SIZE, OnSize )
        //MESSAGE_HANDLER( WM_PAINT, OnPaint )
        MESSAGE_HANDLER( WM_HSCROLL, OnHScroll )
        //MESSAGE_HANDLER( WM_ACTIVATE, OnActivate )
        MESSAGE_HANDLER( WM_ACTIVATEAPP, OnActivateApp )
        MESSAGE_HANDLER( WM_GETMINMAXINFO, OnGetMinMaxInfo )
        MESSAGE_HANDLER( MTBM_DROPDOWNCLICKED, OnDropDownClicked )
        MESSAGE_HANDLER(MTBM_FONTSIZECHANGE, OnFontSizeChanged )
        MESSAGE_HANDLER(MTBM_STEPINITIALVALUECHANGE, OnStepInitialValueChange )
        MESSAGE_HANDLER(MTBM_FILLBACKGROUNDCHANGE, OnFillBackgroundChange )
        MESSAGE_HANDLER(MTBM_INVERTSELECTIONCHANGE, OnInvertSelectionChange)
        MESSAGE_HANDLER(MTBM_DRAWBORDERCHANGE, OnDrawBorderChange)
        MESSAGE_HANDLER(MTBM_ARROWTYPECHANGE, OnArrowTypeChange )
        MESSAGE_HANDLER(MTBM_APPLY, OnApplyOperation)
        MESSAGE_HANDLER(MTBM_CANCEL, OnCancelOperation)
        MESSAGE_HANDLER( TextParamsWindow::TPWM_FONTCHANGED, OnTextParamWindowFontChanged);
        MESSAGE_HANDLER(WM_DPICHANGED, OnDPICHanged)

        COMMAND_ID_HANDLER(IDOK, OnClickedOK)
        COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
        COMMAND_RANGE_HANDLER( ID_PEN, ID_MOVE, OnMenuItemClick)
        COMMAND_ID_HANDLER( ID_UNDO, OnUndoClick )
        COMMAND_ID_HANDLER( ID_CLOSE, OnClickedClose )
        COMMAND_ID_HANDLER( ID_ADDTOWIZARD, OnClickedAddToWizard )
        COMMAND_ID_HANDLER(ID_CONTINUE, OnClickedContinue)
        COMMAND_ID_HANDLER( ID_UPLOAD, OnClickedUpload )
        COMMAND_ID_HANDLER( ID_SHARE, OnClickedShare )
        COMMAND_ID_HANDLER( ID_SAVE, OnClickedSave )
        COMMAND_ID_HANDLER( ID_SAVEAS, OnClickedSaveAs )
        COMMAND_ID_HANDLER( ID_COPYBITMAPTOCLIBOARD, OnClickedCopyToClipboard )
        COMMAND_ID_HANDLER( ID_COPYBITMAPTOCLIBOARD_ALT, OnClickedCopyToClipboard )
        COMMAND_ID_HANDLER(ID_COPYBITMAPTOCLIBOARDASDATAURI, OnClickedCopyToClipboardAsDataUri)
        COMMAND_ID_HANDLER(ID_COPYBITMAPTOCLIBOARDASDATAURI_ALT, OnClickedCopyToClipboardAsDataUri)
        COMMAND_ID_HANDLER(ID_COPYBITMAPTOCLIBOARDASDATAURIHTML, OnClickedCopyToClipboardAsDataUriHtml)
        COMMAND_ID_HANDLER(ID_COPYBITMAPTOCLIBOARDASDATAURIHTML_ALT, OnClickedCopyToClipboardAsDataUriHtml)
        COMMAND_ID_HANDLER(ID_UNSELECTALL, OnUnselectAll)
        COMMAND_ID_HANDLER(ID_PRINTIMAGE, OnPrintImage)
        COMMAND_ID_HANDLER(ID_SEARCHBYIMAGE, OnSearchByImage)
        COMMAND_ID_HANDLER(ID_SEARCHBYIMAGE_ALT, OnSearchByImage)
        COMMAND_RANGE_HANDLER(ID_SEARCHBYIMAGE_START, ID_SEARCHBYIMAGE_END, OnSearchByImage)
        COMMAND_RANGE_HANDLER(ID_SEARCHBYIMAGE_ALT_START, ID_SEARCHBYIMAGE_ALT_END, OnSearchByImage)
        COMMAND_ID_HANDLER(ID_DELETESELECTED, OnDeleteSelected)
        COMMAND_ID_HANDLER(ID_ROTATECLOCKWISE, OnRotateClockwise)
        COMMAND_ID_HANDLER(ID_ROTATECOUNTERCLOCKWISE, OnRotateCounterClockwise)
        COMMAND_ID_HANDLER(ID_FLIPVERTICAL, OnFlipVertical)
        COMMAND_ID_HANDLER(ID_FLIPHORIZONTAL, OnFlipHorizontal)
        COMMAND_ID_HANDLER(ID_MOREACTIONS, OnMoreActionsClicked)

        //MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBackground )
        COMMAND_ID_HANDLER(ID_RECORDSCREEN, OnRecordScreen)
        MESSAGE_HANDLER( WM_ENABLE, OnEnable )
        REFLECT_NOTIFICATIONS()
        /*CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)*/
    END_MSG_MAP()
    private:
        std::unique_ptr<ImageEditor::Document> currentDoc_;
        // Handler prototypes (uncomment arguments if needed):
        //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
        //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
        //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

        LRESULT OnNcCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnActivateApp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnDPICHanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnDropDownClicked(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnMenuItemClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnUndoClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnTextClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedAddToWizard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedUpload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedShare(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedSaveAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedCopyToClipboard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedCopyToClipboardAsDataUri(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnClickedCopyToClipboardAsDataUriHtml(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
        LRESULT OnTextParamWindowFontChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnUnselectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnPrintImage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnSearchByImage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnFontSizeChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnStepInitialValueChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnFillBackgroundChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnInvertSelectionChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnArrowTypeChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnEnable(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnApplyOperation(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnCancelOperation(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnClickedOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnDeleteSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnRotateClockwise(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnRotateCounterClockwise(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnFlipVertical(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnFlipHorizontal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnMoreActionsClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        //LRESULT ReflectedCommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnRecordScreen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnDrawBorderChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

        Toolbar horizontalToolbar_;
        Toolbar verticalToolbar_;
        std::unique_ptr<ImageEditor::Canvas> canvas_;
        std::map<int, DrawingToolType> menuItems_;
        std::map<DrawingToolType, SubMenuItem> subMenuItems_;
        std::map<int,int> selectedSubMenuItems_;
        std::unordered_map<DrawingToolHotkey, int> drawingToolsHotkeys_;
        DialogResult dialogResult_;
        WindowDisplayMode displayMode_;
        DrawingToolType initialDrawingTool_;
        DrawingToolType currentDrawingTool_;
        bool showUploadButton_;
        bool showAddToWizardButton_;
        bool askBeforeClose_;
        CString suggestedFileName_;
        CString serverDisplayName_;
        int prevPenSize_;
        int prevRoundingRadius_;
        float prevBlurRadius_;
        CIcon icon_;
        CIcon iconSmall_;
        std::unique_ptr<ColorsDelegate> colorsDelegate_;
        CString sourceFileName_;
        CString outFileName_;
        HWND cropToolTip_;
        int imageQuality_;
        bool allowAltTab_;
        ServerProfile searchEngine_;
        std::shared_ptr<Gdiplus::Bitmap> resultingBitmap_;
        std::wstring windowTitle_;
        ConfigurationProvider* configurationProvider_;
        TextParamsWindow textParamsWindow_;
        HACCEL accelerators_;
        HMODULE richeditLib_;
        CBitmap bmIconRotateCW_, bmIconRotate_, bmIconFlipVertical_, bmIconFlipHorizontal_;
        CRect selectedRect_;
        bool onlySelectRegion_ = false;
        void createToolbars();
        void OnCropChanged(int x, int y, int w, int h);
        void OnCropFinished(int x, int y, int w, int h);
        void OnDrawingToolChanged(DrawingToolType drawingTool);
        void OnTextEditStarted(ImageEditor::TextElement * textElement);
        void OnTextEditFinished(ImageEditor::TextElement * textElement);
        void OnSelectionChanged();
        void updateRoundingRadiusSlider();
        void updateSearchButton();
        void updateFontSizeControls();
        std::shared_ptr<Gdiplus::Bitmap>  loadToolbarIcon(int resource, bool resize = false);
        std::shared_ptr<Gdiplus::Bitmap>  loadMenuIcon(int resource, bool resize = false);
        void EndDialog(DialogResult dr);
        void init();
        bool saveDocument(ClipboardFormat clipboardFormat = ClipboardFormat::None, bool saveAs = false);
        CString saveToTempFile();
        void updateToolbarDrawingTool(DrawingToolType dt);
        void OnForegroundColorChanged(Gdiplus::Color color);
        void OnBackgroundColorChanged(Gdiplus::Color color);
        void onFontChanged(LOGFONT font);
        bool createTooltip();
        void updatePixelLabels();
        bool OnSaveAs();
        void saveSettings();
        bool copyBitmapToClipboard(ClipboardFormat format = ClipboardFormat::None, bool closeFlag = true);
        BOOL PreTranslateMessage(MSG* pMsg) override;
        bool OnClickedSave();
        void onClose();
        void enableToolbarsIfNecessary(bool enable);
        void updateWindowTitle();
        void showApplyButtons();

        /**
         * Reposition toolbar in full screen mode so it becomes fully visible
         */
        void repositionToolbar(Toolbar& toolbar, const CRect& otherToolbarRect);

        void showMoreActionsDropdownMenu(Toolbar::Item* item);
        void createIcons();

        bool checkCloseWindowAfterAction();
        bool canCloseAfterAction();
};

class ConfigurationProvider {
public:
    ConfigurationProvider() {
        penSize_ = 12;
        roundingRadius_ = 5;
        allowAltTab_ = false;
        memset(&font_, 0, sizeof(font_));
        fillTextBackground_ = false;
        arrowMode_ = 0;
        invertSelection_ = false;
        blurRadius_ = 1.0f;
        closeWindowAfterActionInFullScreen_ = false;
    }
    virtual ~ConfigurationProvider() = default;
    virtual void saveConfiguration() {}
    void setForegroundColor(Gdiplus::Color color) { foregroundColor_ = color; }
    Gdiplus::Color foregroundColor() const { return foregroundColor_; }
    void setBackgroundColor(Gdiplus::Color color) { backgroundColor_ = color; }
    Gdiplus::Color backgroundColor() const { return backgroundColor_; }

    void setStepForegroundColor(Gdiplus::Color color) { stepForegroundColor_ = color; }
    Gdiplus::Color stepForegroundColor() const { return stepForegroundColor_; }
    void setStepBackgroundColor(Gdiplus::Color color) { stepBackgroundColor_ = color; }
    Gdiplus::Color stepBackgroundColor() const { return stepBackgroundColor_; }

    void setPenSize(int size) { penSize_ = size; }
    int penSize() const { return penSize_;}
    void setRoundingRadius(int radius) { roundingRadius_ = radius; }
    int roundingRadius() const { return roundingRadius_;}
    void setBlurRadius(float radius) { blurRadius_ = radius; }
    float blurRadius() const { return blurRadius_; }

    bool allowAltTab() const  {  return allowAltTab_; }
    void setFont(const LOGFONT& font) { font_ = font; }
    LOGFONT font() const { return font_; }
    void setSearchEngine(const ServerProfile& se) {
        searchEngine_ = se;
    }
    const ServerProfile& searchEngine() const{
        return searchEngine_;
    }
    void setFillTextBackground(bool fill) {
        fillTextBackground_ = fill;
    }
    bool fillTextBackground() const {
        return fillTextBackground_;
    }

    void setInvertSelection(bool invert) {
        invertSelection_ = invert;
    }
    bool invertSelection() const {
        return invertSelection_;
    }
    void setArrowMode(int mode) {
        arrowMode_ = mode;
    }
    int getArrowMode() const {
        return arrowMode_;
    }

    void setCloseWindowAfterActionInFullScreen(bool enable) {
        closeWindowAfterActionInFullScreen_ = enable;
    }

    bool getCloseWindowAfterActionInFullScreen() const {
        return closeWindowAfterActionInFullScreen_;
    }

	bool getDrawBorder() const { return drawBorder_; }
    void setDrawBorder(bool val) { drawBorder_ = val; }

protected:
    Gdiplus::Color foregroundColor_, backgroundColor_,
        stepForegroundColor_, stepBackgroundColor_;
    int penSize_;
    int roundingRadius_;
    float blurRadius_;
    int arrowMode_;
    LOGFONT font_;
    bool allowAltTab_;
    bool closeWindowAfterActionInFullScreen_;  
    ServerProfile searchEngine_;
    bool fillTextBackground_;
    bool invertSelection_;
    bool drawBorder_ = true;
};

}
#endif

