#include "ImageEditorWindow.h"

#include <boost/format.hpp>

#include "ImageEditorView.h"
#include "ImageEditor/Gui/Toolbar.h"
#include "ColorsDelegate.h"
#include "Core/Logging.h"
#include "Core/Images/Utils.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Func/ClipboardUtils.h"
#include "Func/IuCommonFunctions.h"
#include "ImageEditor/MovableElements.h"
#include "Gui/Dialogs/SearchByImageDlg.h"
#include "Gui/Components/MyFileDialog.h"
#include "ScreenCapture/MonitorEnumerator.h"
#include "Core/AbstractServerIconCache.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Helpers/DPIHelper.h"
#include "Gui/Controls/ServerSelectorControl.h"

namespace ImageEditor {

const auto UPLOAD_BUTTON_MAX_LENGTH = 35;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ImageEditorWindow::ImageEditorWindow(std::shared_ptr<Gdiplus::Bitmap> bitmap, bool hasTransparentPixels, ConfigurationProvider* configurationProvider,
    UploadEngineManager* uploadEngineManager, bool onlySelectRegion)
    : horizontalToolbar_(Toolbar::orHorizontal, !onlySelectRegion)
    , verticalToolbar_(Toolbar::orVertical),
    uploadEngineManager_(uploadEngineManager) {
    currentDoc_ =  std::make_unique<ImageEditor::Document>(std::move(bitmap), hasTransparentPixels);
    configurationProvider_ = configurationProvider;
    askBeforeClose_ = true;
    allowAltTab_ = false;
    onlySelectRegion_ = onlySelectRegion;

    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    serverDisplayName_ = CUploadEngineListBase::getServerDisplayName(settings->quickScreenshotServer.getByIndex(0).uploadEngineData());

    init();
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
ImageEditorWindow::ImageEditorWindow(CString imageFileName, ConfigurationProvider* configurationProvider ):
    horizontalToolbar_(Toolbar::orHorizontal), verticalToolbar_(Toolbar::orVertical)
    , uploadEngineManager_(nullptr) {
    currentDoc_ = std::make_unique<ImageEditor::Document>(imageFileName);

    sourceFileName_ = imageFileName;
    configurationProvider_ = configurationProvider;
    askBeforeClose_ = true;
    allowAltTab_ = false;
    init();
}

void ImageEditorWindow::init()
{
    richeditLib_ = LoadLibrary(CRichEditCtrl::GetLibraryName());
//    resultingBitmap_ = 0;
    cropToolTip_ = 0;
    showUploadButton_ = true;
    showAddToWizardButton_ = true;
    prevPenSize_ = 0;
    prevRoundingRadius_ = 0;
    prevBlurRadius_ = 0;
    imageQuality_ = 85;
    currentDrawingTool_ = DrawingToolType::dtNone;
    initialDrawingTool_ = DrawingToolType::dtBrush;
    displayMode_ = wdmAuto;
    accelerators_ = nullptr;
    menuItems_[ID_PEN]             = DrawingToolType::dtPen;
    menuItems_[ID_LINE]            = DrawingToolType::dtLine;
    menuItems_[ID_BRUSH]           = DrawingToolType::dtBrush;
    menuItems_[ID_MARKER]          = DrawingToolType::dtMarker;
    menuItems_[ID_RECTANGLE]       = DrawingToolType::dtRectangle;
    menuItems_[ID_FILLEDRECTANGLE] = DrawingToolType::dtFilledRectangle;
    menuItems_[ID_CROP]            = DrawingToolType::dtCrop;
    menuItems_[ID_MOVE]            = DrawingToolType::dtMove;
    menuItems_[ID_ARROW]           = DrawingToolType::dtArrow;
    menuItems_[ID_SELECTION]       = DrawingToolType::dtSelection;
    menuItems_[ID_BLUR]            = DrawingToolType::dtBlur;
    menuItems_[ID_BLURRINGRECTANGLE]   = DrawingToolType::dtBlurringRectangle;
    menuItems_[ID_PIXELATERECTANGLE]   = DrawingToolType::dtPixelateRectangle;
    menuItems_[ID_COLORPICKER]     = DrawingToolType::dtColorPicker;
    menuItems_[ID_ROUNDEDRECTANGLE]     = DrawingToolType::dtRoundedRectangle;
    menuItems_[ID_ELLIPSE]     = DrawingToolType::dtEllipse;
    menuItems_[ID_FILLEDROUNDEDRECTANGLE]     = DrawingToolType::dtFilledRoundedRectangle;
    menuItems_[ID_FILLEDELLIPSE]     = DrawingToolType::dtFilledEllipse;
    menuItems_[ID_TEXT]     = DrawingToolType::dtText;
    menuItems_[ID_STEPNUMBER]     = DrawingToolType::dtStepNumber;

    SubMenuItem item;
    item.parentCommand = ID_RECTANGLE;
    item.icon = loadToolbarIcon(IDB_ICONTOOLRECTANGLEPNG);
    item.command = ID_RECTANGLE;
    item.hint = TR("Rectangle");
    subMenuItems_[DrawingToolType::dtRectangle] = item;

    item.icon = loadToolbarIcon(IDB_ICONTOOLROUNDEDRECTANGLE);
    item.command = ID_ROUNDEDRECTANGLE;
    item.hint = TR("Rounded rectangle");
    subMenuItems_[DrawingToolType::dtRoundedRectangle] = item;

    item.icon = loadToolbarIcon(IDB_ICONTOOLELLIPSE);
    item.command = ID_ELLIPSE;
    item.hint = TR("Ellipse");
    subMenuItems_[DrawingToolType::dtEllipse] = item;

    item.parentCommand = ID_FILLEDRECTANGLE;
    item.command = ID_FILLEDRECTANGLE;
    item.hint = TR("Filled rectangle");
    item.icon = loadToolbarIcon(IDB_ICONTOOLFILLEDRECTANGLE);
    subMenuItems_[DrawingToolType::dtFilledRectangle] = item;

    item.icon = loadToolbarIcon(IDB_ICONTOOLFILLEDROUNDEDRECTANGLE);
    item.command = ID_FILLEDROUNDEDRECTANGLE;
    item.hint = TR("Rounded rectangle");
    subMenuItems_[DrawingToolType::dtFilledRoundedRectangle] = item;

    item.icon = loadToolbarIcon(IDB_ICONTOOLFILLEDELLIPSE);
    item.command = ID_FILLEDELLIPSE;
    item.hint =  TR("Filled ellipse");
    subMenuItems_[DrawingToolType::dtFilledEllipse] = item;

    SubMenuItem item2;
    item2.parentCommand = ID_BLURRINGRECTANGLE;

    item2.icon = loadToolbarIcon(IDB_ICONTOOLBLURINGRECTANGLEPNG);
    item2.command = ID_BLURRINGRECTANGLE;
    item2.hint = TR("Blurring rectangle");
    subMenuItems_[DrawingToolType::dtBlurringRectangle] = item2;

    item2.icon = loadToolbarIcon(IDB_ICONTOOLPIXELATE);
    item2.command = ID_PIXELATERECTANGLE;
    item2.hint = TR("Pixelation");
    subMenuItems_[DrawingToolType::dtPixelateRectangle] = item2;

    selectedSubMenuItems_[ID_RECTANGLE] = ID_RECTANGLE;
    selectedSubMenuItems_[ID_FILLEDRECTANGLE] = ID_FILLEDRECTANGLE;
    selectedSubMenuItems_[ID_BLURRINGRECTANGLE] = ID_BLURRINGRECTANGLE;

    drawingToolsHotkeys_[kMoveKey] = ID_MOVE /*DrawingToolType::dtMove*/;
    drawingToolsHotkeys_[kBrushKey] = ID_BRUSH /*DrawingToolType::dtBrush*/;
    drawingToolsHotkeys_[kTextKey] = ID_TEXT /*DrawingToolType::dtText*/;
    drawingToolsHotkeys_[kRectangleKey] = ID_RECTANGLE /*DrawingToolType::dtRectangle*/;
    drawingToolsHotkeys_[kColorPickerKey] = ID_COLORPICKER /*DrawingToolType::dtColorPicker*/;
    drawingToolsHotkeys_[kCropKey] = ID_CROP;
    drawingToolsHotkeys_[kMarkerKey] = ID_MARKER;
    drawingToolsHotkeys_[kBlurringRectangleKey] = ID_BLURRINGRECTANGLE;
    drawingToolsHotkeys_[kArrowKey] = ID_ARROW;
    drawingToolsHotkeys_[kLineKey] = ID_LINE;
    drawingToolsHotkeys_[kFilledRectangle] = ID_FILLEDRECTANGLE;
    drawingToolsHotkeys_[kStepNumber] = ID_STEPNUMBER;

    dialogResult_ = drCancel;
}

bool ImageEditorWindow::saveDocument(ClipboardFormat clipboardFormat, bool saveAs)
{
    resultingBitmap_ = canvas_->getBitmapForExport();
	if (!saveAs) {
        canvas_->setDocumentModified(false);
	}

    if ( !resultingBitmap_ ) {
        LOG(ERROR) << "canvas_->getBitmapForExport() returned NULL";
        return false;
    }
    if (clipboardFormat == ClipboardFormat::None) {
        if ( !outFileName_.IsEmpty() ) {
            try {
                ImageUtils::SaveImageToFile(resultingBitmap_.get(), outFileName_, nullptr, ImageUtils::sifDetectByExtension, imageQuality_);
            } catch (const std::exception& ex) {
                ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Image Editor"), U2W(ex.what()));
            }
            canvas_->updateView();
            return true;
        } else {
            CString outFileName;
            try {
                if (ImageUtils::MySaveImage(resultingBitmap_.get(), "screeenshot001.png", outFileName, ImageUtils::sifJPEG, 95)) {
                    outFileName_ = outFileName;
                    canvas_->updateView();
                    return true;
                }
            } catch (const std::exception& ex) {
                ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Image Editor"), U2W(ex.what()));
            }
            return false;
        }
    } else if (clipboardFormat  == ClipboardFormat::Bitmap ) {
        ClipboardUtils::CopyBitmapToClipboard(resultingBitmap_.get(), m_hWnd, true);
        return true;
    } else if (clipboardFormat == ClipboardFormat::DataUri || clipboardFormat == ClipboardFormat::DataUriHtml) {
        try {
            ImageUtils::CopyBitmapToClipboardInDataUriFormat(resultingBitmap_.get(), ImageUtils::sifPNG, 85, clipboardFormat == ClipboardFormat::DataUriHtml);
        } catch (const std::exception& ex) {
            ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Image Editor"), U2W(ex.what()));
        }
        return true;
    }
    return false;
}

CString ImageEditorWindow::saveToTempFile() {
    const std::shared_ptr<Gdiplus::Bitmap> bitmap = canvas_->getBitmapForExport();
    CString result;
    try {
        if (ImageUtils::MySaveImage(bitmap.get(), "image.png", result, ImageUtils::sifPNG, 95)) {
            return result;
        }
    } catch (const std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
    return {};
}

void ImageEditorWindow::updateToolbarDrawingTool(DrawingToolType dt)
{
    if ( currentDrawingTool_ == dt ) {
        return;
    }

    currentDrawingTool_ = dt;
    updateRoundingRadiusSlider();
    updateFontSizeControls();

    const auto submenuIter = subMenuItems_.find(dt);
    if ( submenuIter != subMenuItems_.end() ) {
        const int buttonIndex = verticalToolbar_.getItemIndexByCommand(selectedSubMenuItems_[submenuIter->second.parentCommand]);
        if ( buttonIndex != -1 ) {
            Toolbar::Item* item = verticalToolbar_.getItem(buttonIndex);
            if ( item ) {
                item->icon = submenuIter->second.icon;
                item->command = submenuIter->second.command;
                item->hint = submenuIter->second.hint;
                verticalToolbar_.updateTooltipForItem(buttonIndex);
                selectedSubMenuItems_[submenuIter->second.parentCommand] = item->command;
            }

            verticalToolbar_.clickButton(buttonIndex);
            return;
        }
    }

    //std::map<int, Canvas::DrawingToolType>::iterator it = std::find(menuItems_.begin(), menuItems_.end(), dt);
    std::map<int, DrawingToolType>::iterator it;
    for( it = menuItems_.begin(); it != menuItems_.end(); ++it ) {
        if ( it->second == dt ) {
            int buttonIndex = verticalToolbar_.getItemIndexByCommand(it->first);
            if ( buttonIndex != -1 ) {
                verticalToolbar_.clickButton(buttonIndex);
                break;
            }
        }
    }
    if ( dt == DrawingToolType::dtCrop ) {
        createTooltip();
    } else if ( cropToolTip_ ) {
        ::DestroyWindow(cropToolTip_);
        cropToolTip_ = nullptr;
    }
}

void ImageEditorWindow::OnForegroundColorChanged(Gdiplus::Color color)
{
    colorsDelegate_->setForegroundColor(color);
    verticalToolbar_.repaintItem(colorsDelegate_->itemIndex());
}

void ImageEditorWindow::OnBackgroundColorChanged(Gdiplus::Color color)
{
    colorsDelegate_->setBackgroundColor(color);
    verticalToolbar_.repaintItem(colorsDelegate_->itemIndex());
}

void ImageEditorWindow::onFontChanged(LOGFONT font)
{
    textParamsWindow_.setFont(font);
}

ImageEditorWindow::~ImageEditorWindow()
{
    FreeLibrary(richeditLib_);
}

void ImageEditorWindow::setInitialDrawingTool(DrawingToolType dt)
{
    initialDrawingTool_ = dt;
}

void ImageEditorWindow::showUploadButton(bool show)
{
    showUploadButton_ = show;
}

void ImageEditorWindow::showAddToWizardButton(bool show)
{
    showAddToWizardButton_ = show;
}

void ImageEditorWindow::setScreenshotData(const IuCommonFunctions::ScreenshotData& screenshotData)
{
    screenshotData_ = screenshotData;
}

std::shared_ptr<Gdiplus::Bitmap> ImageEditorWindow::getResultingBitmap() const
{
    return resultingBitmap_;
}

Gdiplus::Rect ImageEditorWindow::lastAppliedCrop() const {
    return canvas_ ? canvas_->lastCrop() : Gdiplus::Rect();
}

CRect ImageEditorWindow::getSelectedRect() const {
    return selectedRect_;
}

void ImageEditorWindow::setAskBeforeClose(bool ask)
{
    askBeforeClose_ = ask;
}

CString ImageEditorWindow::outFileName() const {
    return outFileName_;
}

ImageEditorWindow::DialogResult ImageEditorWindow::DoModal(HWND parent, HMONITOR screenshotsMonitor, WindowDisplayMode mode, bool forceShowParent) {
    if (currentDoc_->isNull()) {
        GuiTools::LocalizedMessageBox(nullptr, _T("Invalid image file."), APP_NAME, MB_ICONERROR);
        return drCancel;
    }

    static BOOL verificationFlagChecked = FALSE;
    if (currentDoc_ && currentDoc_->isSrcMultiFrame() && !verificationFlagChecked) {
        int buttonPressed;
        CTaskDialog dlg;
        CString verText = TR("Do not ask again");
        dlg.SetVerificationText(verText);
        CString contentText = TR("Image Editor cannot deal with animated images. If you edit this file, animation will be lost. Do you wish to continue?");
        dlg.SetContentText(contentText);
        CString windowTitle = TR("Image Editor");
        dlg.SetWindowTitle(windowTitle);
        dlg.SetCommonButtons(TDCBF_YES_BUTTON | TDCBF_NO_BUTTON);
        dlg.SetMainIcon(TD_WARNING_ICON);
        DWORD flags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ALLOW_DIALOG_CANCELLATION;
        if (ServiceLocator::instance()->translator()->isRTL()) {
            flags |= TDF_RTL_LAYOUT;
        }
        dlg.ModifyFlags(0, flags);

        int res = dlg.DoModal(parent, &buttonPressed, nullptr, &verificationFlagChecked);
        if (SUCCEEDED(res) && buttonPressed != IDYES) {
            return drCancel;
        }
    }
    //int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    //int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    //    displayMode_ = wdmWindowed;

    CRect displayBounds;
    GuiTools::GetScreenBounds(displayBounds);

    CClientDC hdc(nullptr);
    float dpiScaleX = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    float dpiScaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
    int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
    int scrollbarHeight = GetSystemMetrics(SM_CYVSCROLL);
    int desiredClientWidth = currentDoc_->getWidth() + static_cast<int>(40 * dpiScaleX) + kCanvasMargin + scrollbarWidth; // with toolbars
    int desiredClientHeight = currentDoc_->getHeight() + static_cast<int>(60 * dpiScaleY) + kCanvasMargin + scrollbarHeight;
    CRect rc(rcDefault);
    DWORD initialWindowStyle = WS_OVERLAPPED | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX  | WS_CLIPCHILDREN;
    // Parent window should be null here!
    if (Create(nullptr, rc, _T("Image Editor"), initialWindowStyle, 0) == NULL) {
        LOG(ERROR) << "Main window creation failed!\n";
        return drCancel;
    }

    CRect workArea;
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    ::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
    workArea = mi.rcWork;

    CRect screenshotMonitorBounds;
    if (screenshotsMonitor) {
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfo(screenshotsMonitor, &mi)) {
            LOG(ERROR) << "Error getting information about monitor";
        }
        screenshotMonitorBounds = mi.rcMonitor;
    }

    if (mode == wdmAuto) {
        /*CRect newClientRect(0, 0, desiredClientWidth, desiredClientHeight);
        AdjustWindowRect(newClientRect, GetStyle(), false);*/

        /*LOG(ERROR) << "NewClienRect ajusted =" << newClientRect.Width() << "x" << newClientRect.Height() << std::endl

            << " displayBounds = " << displayBounds.Width() << "x" << displayBounds.Height() << std::endl
            << " currentDoc = " << currentDoc_->getWidth() << "x" << currentDoc_->getHeight();*/

        // newClientRect.Width() >= workArea.Width()  || newClientRect.Height()  >= workArea.Height()
        if ( currentDoc_->getWidth() == displayBounds.Width() &&  currentDoc_->getHeight() == displayBounds.Height()) {
            mode = wdmFullscreen;
        } else {
            mode = wdmWindowed;
        }
    }
    displayMode_ = mode;

    if (displayMode_ == wdmFullscreen) {
        //windowStyle = GetStyle();
        DWORD windowStyle = WS_POPUP | WS_CLIPCHILDREN;
        SetWindowLong(GWL_STYLE, windowStyle);
        HWND insertAfter = nullptr;
//#ifndef _DEBUG
        if (!allowAltTab_) {
            SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) | WS_EX_TOPMOST);
        }
        insertAfter = HWND_TOPMOST;
//#endif
        if (screenshotsMonitor) {
            SetWindowPos(insertAfter, screenshotMonitorBounds.left, screenshotMonitorBounds.top, screenshotMonitorBounds.right - screenshotMonitorBounds.left, screenshotMonitorBounds.bottom - screenshotMonitorBounds.top, 0);
        } else {
            SetWindowPos(insertAfter, displayBounds.left, displayBounds.top, displayBounds.right - displayBounds.left, displayBounds.bottom - displayBounds.top, 0);
        }
    }

    if (displayMode_ == wdmWindowed) {
        CenterWindow();
    }

    std::vector<ACCEL> accels = {
        { FVIRTKEY | FCONTROL, 'Z', ID_UNDO },
        { FVIRTKEY | FCONTROL, 'D', ID_UNSELECTALL },
        { FVIRTKEY | FCONTROL, 'S', ID_SAVE },
        { FVIRTKEY | FCONTROL | FSHIFT, 'S', ID_SAVE_ALT },
        { FVIRTKEY | FCONTROL, 'C', ID_COPYBITMAPTOCLIBOARD },
        { FVIRTKEY | FCONTROL | FSHIFT, 'C', ID_COPYBITMAPTOCLIBOARD_ALT },
        { FVIRTKEY | FCONTROL, 'F', ID_SEARCHBYIMAGE },
        { FVIRTKEY | FCONTROL | FSHIFT, 'F', ID_SEARCHBYIMAGE_ALT },
        { FVIRTKEY | FCONTROL, 'P', ID_PRINTIMAGE },
        { FVIRTKEY, VK_RETURN, IDOK },
        { FVIRTKEY, VK_DELETE, ID_DELETESELECTED },
    };

    if (displayMode_ == wdmFullscreen) {
        accels.push_back({ FVIRTKEY | FCONTROL, 'R', ID_RECORDSCREEN });
    }

    for(const auto& [k,v]: drawingToolsHotkeys_) {
        accels.push_back(
            { FVIRTKEY, static_cast<WORD>(k), static_cast<WORD>(v) }
        );
    }

    if (onlySelectRegion_) {
        accels.clear();
        accels.push_back({ FVIRTKEY, VK_RETURN, ID_CONTINUE });
    }
    accels.push_back({ FVIRTKEY, VK_ESCAPE, ID_CLOSE });

    accelerators_ = CreateAcceleratorTable(accels.data(), accels.size());

    //RECT rc;
    GetClientRect(&rc);
    /*HWND m_hWndClient = */m_view.Create(m_hWnd, rc, _T("ImageEditor_Canvas"), WS_CHILD | WS_VISIBLE /*| WS_CLIPSIBLINGS | WS_CLIPCHILDREN*/, 0);

    canvas_ = std::make_unique<ImageEditor::Canvas>(m_view);
    canvas_->setSize(currentDoc_->getWidth(), currentDoc_->getHeight());
    canvas_->setDocument(currentDoc_.get());
    canvas_->setCropOnExport(displayMode_ == wdmFullscreen);

    updateWindowTitle();

    textParamsWindow_.Create(m_hWnd);

    if (configurationProvider_) {
        canvas_->setPenSize(configurationProvider_->penSize());
        canvas_->setForegroundColor(configurationProvider_->foregroundColor());
        canvas_->setBackgroundColor(configurationProvider_->backgroundColor());
        canvas_->setFont(configurationProvider_->font());
        canvas_->setRoundingRadius(configurationProvider_->roundingRadius());
        canvas_->setBlurRadius(configurationProvider_->blurRadius());
        canvas_->setFillTextBackground(configurationProvider_->fillTextBackground());
        canvas_->setInvertSelection(configurationProvider_->invertSelection());
        canvas_->setStepColors(configurationProvider_->stepForegroundColor(), configurationProvider_->stepBackgroundColor());
        canvas_->setArrowMode(static_cast<Arrow::ArrowMode>(configurationProvider_->getArrowMode()));
        canvas_->setDrawBorder(configurationProvider_->getDrawBorder());
        allowAltTab_ = configurationProvider_->allowAltTab();
        textParamsWindow_.setFont(configurationProvider_->font());
        searchEngine_ = configurationProvider_->searchEngine();
    }
    m_view.setCanvas(canvas_.get());
    createToolbars();
    RECT horToolbarRect;
    RECT vertToolbarRect;
    horizontalToolbar_.GetClientRect(&horToolbarRect);
    verticalToolbar_.GetClientRect(&vertToolbarRect);
    rc.left = displayMode_ == wdmWindowed ? vertToolbarRect.right + kCanvasMargin : 0;
    rc.top = displayMode_ == wdmWindowed ? horToolbarRect.bottom + kCanvasMargin : 0;

    if (displayMode_ == wdmWindowed) {
        horizontalToolbar_.SetWindowPos(0, rc.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        verticalToolbar_.SetWindowPos(0, 0, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    } else {
        repositionToolbar(horizontalToolbar_, vertToolbarRect);
        repositionToolbar(verticalToolbar_, horToolbarRect);
    }

    m_view.SetWindowPos(0, &rc, SWP_NOSIZE);

    using namespace std::placeholders;

    if (!onlySelectRegion_) {
        canvas_->onDrawingToolChanged.connect([this](auto && PH1) { OnDrawingToolChanged(std::forward<decltype(PH1)>(PH1)); });
        canvas_->onForegroundColorChanged.connect([this](auto && PH1) { OnForegroundColorChanged(std::forward<decltype(PH1)>(PH1)); });
        canvas_->onBackgroundColorChanged.connect([this](auto && PH1) { OnBackgroundColorChanged(std::forward<decltype(PH1)>(PH1)); });
        canvas_->onFontChanged.connect([this](auto && PH1) { onFontChanged(std::forward<decltype(PH1)>(PH1)); });
        canvas_->onTextEditStarted.connect([this](auto && PH1) { OnTextEditStarted(std::forward<decltype(PH1)>(PH1)); });
        canvas_->onTextEditFinished.connect([this](auto && PH1) { OnTextEditFinished(std::forward<decltype(PH1)>(PH1)); });
        canvas_->onSelectionChanged.connect([this] { OnSelectionChanged(); });
    }
    canvas_->onCropChanged.connect([this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) {
        OnCropChanged(
            std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
            std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4)
        );
    });
    canvas_->onCropFinished.connect([this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) {
        OnCropFinished(
            std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
            std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4)
        );
    });
    canvas_->onDocumentModified.connect([this] { updateWindowTitle();  });

    if (initialDrawingTool_ != DrawingToolType::dtCrop) {
        verticalToolbar_.ShowWindow(SW_SHOW);
        horizontalToolbar_.ShowWindow(SW_SHOW);
    }
    if (!onlySelectRegion_) {
        updatePixelLabels();
    }
    canvas_->setDrawingToolType(initialDrawingTool_);
    if (!onlySelectRegion_) {
        updateToolbarDrawingTool(initialDrawingTool_);
    }
    if (displayMode_ == wdmWindowed) {
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        ::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
        workArea = mi.rcWork;
        CRect newClientRect(0, 0, desiredClientWidth, desiredClientHeight);
        AdjustWindowRect(newClientRect, GetStyle(), false);
        int newWidth = std::max(static_cast<LONG>(500 * dpiScaleX), newClientRect.right - newClientRect.left);
        int newHeight = std::max(static_cast<LONG>(500 * dpiScaleY), newClientRect.bottom - newClientRect.top);
        newWidth = std::min(newWidth, workArea.Width()-4);
        newHeight= std::min(newHeight, workArea.Height()-4);

        SetWindowPos(0, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER);
        //ResizeClient(newWidth, newHeight);
        CenterWindow(parent);
    }
    canvas_->updateView();
    m_view.SetFocus();
    //displayMode_ = wdmWindowed;
    ShowWindow(SW_SHOW);
    SetForegroundWindow(m_hWnd);
    //BringWindowToTop();
    //m_view.Invalidate(false);

    if ( parent ) {
        ::EnableWindow(parent, false);
    }

    CMessageLoop loop;
    loop.AddMessageFilter(this);
    loop.Run();
    saveSettings();
    if ( parent ) {
        ::EnableWindow(parent, true);
        if (forceShowParent) {
            ::ShowWindow(parent, SW_SHOWNORMAL);
        }

        ::SetActiveWindow(parent);
    }
    ShowWindow(SW_HIDE);

    if (parent) {
        ::SetForegroundWindow(parent);
        ::SetFocus(parent); 
    }
    loop.RemoveMessageFilter(this);

    if (displayMode_ == wdmFullscreen) {
        const Gdiplus::Rect cropRect = canvas_ ? canvas_->lastCrop() : Gdiplus::Rect();
        CRect windowRect;
        GetWindowRect(&windowRect);
        selectedRect_.left = windowRect.left + cropRect.GetLeft();
        selectedRect_.top = windowRect.top + cropRect.GetTop();
        selectedRect_.right = selectedRect_.left + cropRect.Width;
        selectedRect_.bottom = selectedRect_.top + cropRect.Height;
    }

    DestroyWindow();
    DestroyAcceleratorTable(accelerators_);
    accelerators_ = nullptr;
    if ( dialogResult_ == drCancel  ) {
//        delete resultingBitmap_;
        //resultingBitmap_ = 0;
    }



    return dialogResult_;
}

LRESULT ImageEditorWindow::OnNcCreate(UINT, WPARAM, LPARAM, BOOL&) {
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL);
    return TRUE;
}

LRESULT ImageEditorWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowText(TR("Image Editor"));
    createIcons();
    return 0;
}

LRESULT ImageEditorWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = FALSE;
    return 1;
}

LRESULT ImageEditorWindow::OnClose(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    OnClickedClose(ID_CLOSE,0,0, bHandled);
    return 0;
}

LRESULT ImageEditorWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    if ( horizontalToolbar_.m_hWnd ) { // It is possible that toolbars are not created yet
        RECT rc;
        GetClientRect(&rc);
        RECT horToolbarRect;
        RECT vertToolbarRect;
        horizontalToolbar_.GetClientRect(&horToolbarRect);
        verticalToolbar_.GetClientRect(&vertToolbarRect);

        rc.left = displayMode_ == wdmWindowed ? vertToolbarRect.right + kCanvasMargin : 0;
        rc.top = displayMode_ == wdmWindowed ? horToolbarRect.bottom + kCanvasMargin : 0;

        m_view.SetWindowPos(0, &rc, SWP_NOZORDER);

        //rc.right -=  (displayMode_ == wdmWindowed ? vertToolbarRect.right+kCanvasMargin : 0);
        //rc.bottom -=  (displayMode_ == wdmWindowed ? horToolbarRect.bottom+kCanvasMargin : 0);

        m_view.SetWindowPos(0, &rc, SWP_NOZORDER);
        //m_view.Invalidate(TRUE);
    }
    return 0;
}

LRESULT ImageEditorWindow::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    return 0;
    CPaintDC dc(m_hWnd);
    if ( displayMode_ == wdmWindowed ) {

        CRect clientRect;
        GetClientRect(&clientRect);
        CRgn rgn;
        rgn.CreateRectRgn(clientRect.left,clientRect.top, clientRect.right, clientRect.bottom);

        RECT horToolbarRect;
        RECT vertToolbarRect;
        RECT viewRect;
        horizontalToolbar_.GetClientRect(&horToolbarRect);
        horizontalToolbar_.ClientToScreen(&horToolbarRect);
        ScreenToClient(&horToolbarRect);
        verticalToolbar_.GetClientRect(&vertToolbarRect);
        verticalToolbar_.ClientToScreen(&vertToolbarRect);
        ScreenToClient(&vertToolbarRect);
        m_view.GetClientRect(&viewRect);
        m_view.ClientToScreen(&viewRect);
        ScreenToClient(&viewRect);

        CRgn horToolRgn;
        CRgn vertToolRgn;
        CRgn viewRgn;
        horToolRgn.CreateRectRgn(horToolbarRect.left, horToolbarRect.top, horToolbarRect.right, horToolbarRect.bottom);
        vertToolRgn.CreateRectRgn(vertToolbarRect.left, vertToolbarRect.top, vertToolbarRect.right, vertToolbarRect.bottom);
        viewRgn.CreateRectRgn(viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);
        rgn.CombineRgn(horToolRgn, RGN_DIFF);
        rgn.CombineRgn(vertToolRgn, RGN_DIFF);
        rgn.CombineRgn(viewRgn, RGN_DIFF);

        CBrush br;
        br.CreateSolidBrush(GetSysColor(COLOR_APPWORKSPACE)); // MDI background
        dc.FillRgn(rgn, br);

        /*CRect viewRect;
        m_view.GetClientRect(&viewRect);*/

        /*RECT topRect = {0, 0, clientRect.right,horToolbarRect.bottom + kCanvasMargin};
        dc.FillRect(&topRect, br);
        RECT leftRect = {0, topRect.bottom, vertToolbarRect.right + kCanvasMargin, clientRect.bottom};
        dc.FillRect(&leftRect, br);*/
    }

    return 0;
}

LRESULT ImageEditorWindow::OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = true;
    return TRUE;
}

LRESULT ImageEditorWindow::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    if ( !horizontalToolbar_.m_hWnd ) {
        return 0;
    }
    MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
    RECT horRc, vertRc;
    horizontalToolbar_.GetClientRect(&horRc);
    verticalToolbar_.GetClientRect(&vertRc);
    RECT clientRect;
    RECT windowRect;
    GetClientRect(&clientRect);
    GetWindowRect(&windowRect);
    int paddingX = windowRect.right -  windowRect.left - clientRect.right;
    int paddingY = windowRect.bottom -  windowRect.top - clientRect.bottom;

    mmi->ptMinTrackSize.y = paddingY + kToolbarOffset+ horRc.bottom + vertRc.bottom;
    mmi->ptMinTrackSize.x = paddingX + kToolbarOffset + horRc.right + vertRc.right;
    return 0;
}

LRESULT ImageEditorWindow::OnClickedOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if (showUploadButton_ || showAddToWizardButton_ ) {
        if (!sourceFileName_.IsEmpty()) {
            outFileName_ = sourceFileName_;
        }
        if (saveDocument()) {
            DialogResult dr = showUploadButton_ ? drUpload : drAddToWizard;
            EndDialog(dr);
        }
    }
    return 0;
}

LRESULT ImageEditorWindow::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{   
    if (onlySelectRegion_) {
        return 0;
    }
    if (wParam == VK_OEM_6) { // ']'
         canvas_->beginPenSizeChanging();
         canvas_->setPenSize(canvas_->getPenSize()+1);
         horizontalToolbar_.penSizeSlider_.SetPos(canvas_->getPenSize());
         updatePixelLabels();
         m_view.SendMessage(WM_SETCURSOR, (LPARAM)m_view.m_hWnd, 0);
     }
     else if (wParam == VK_OEM_4 ) { //'['
         canvas_->beginPenSizeChanging();
         canvas_->setPenSize(canvas_->getPenSize()-1);
         horizontalToolbar_.penSizeSlider_.SetPos(canvas_->getPenSize());
         updatePixelLabels();
         m_view.SendMessage(WM_SETCURSOR, reinterpret_cast<LPARAM>(m_view.m_hWnd), 0);
     }
    return 0;
}

LRESULT ImageEditorWindow::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    if (onlySelectRegion_) {
        return 0;
    }
    if (wParam == VK_OEM_4 || wParam == VK_OEM_6) {
        canvas_->endPenSizeChanging(canvas_->getPenSize());
    }
    return 0;
}

LRESULT ImageEditorWindow::OnActivateApp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (allowAltTab_ && displayMode_ == wdmFullscreen) {
        if ( !wParam ) { // if the window is being deactivated
            LONG exStyle = GetWindowLong(GWL_EXSTYLE);
            SetWindowLong(GWL_EXSTYLE, exStyle & ~(WS_EX_TOPMOST));
            SetWindowPos(HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
            SetWindowPos(HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
            HWND foregroundWindow = GetForegroundWindow();
            if ( foregroundWindow != m_hWnd ) {
                ::SetWindowPos(foregroundWindow, m_hWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
            }
        } else {
            SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
            LONG exStyle = GetWindowLong(GWL_EXSTYLE);
            SetWindowLong(GWL_EXSTYLE, exStyle | WS_EX_TOPMOST);
        }
    }
    return 0;
}


LRESULT ImageEditorWindow::OnDPICHanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    int dpiX = LOWORD(wParam);
    int dpiY = HIWORD(wParam);
    if (displayMode_ == wdmWindowed) {
        horizontalToolbar_.setDPI(dpiX, dpiY);
        horizontalToolbar_.update();
        if (!onlySelectRegion_) {
            verticalToolbar_.setDPI(dpiX, dpiY);
            verticalToolbar_.update();
        }
        RECT horToolbarRect;
        RECT vertToolbarRect;
        horizontalToolbar_.GetClientRect(&horToolbarRect);
        verticalToolbar_.GetClientRect(&vertToolbarRect);
        RECT rc {};
        rc.left = displayMode_ == wdmWindowed ? vertToolbarRect.right + kCanvasMargin : 0;
        rc.top = displayMode_ == wdmWindowed ? horToolbarRect.bottom + kCanvasMargin : 0;

        if (displayMode_ == wdmWindowed) {
            horizontalToolbar_.SetWindowPos(0, rc.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            verticalToolbar_.SetWindowPos(0, 0, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    createIcons();
    return 0;
}

LRESULT ImageEditorWindow::OnDropDownMouseDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    Toolbar::Item* item = reinterpret_cast<Toolbar::Item*>(wParam);
    if ( item->command == ID_RECTANGLE || item->command == ID_ROUNDEDRECTANGLE || item->command == ID_ELLIPSE) {
        CMenu rectangleMenu;
        RECT rc = item->rect;
        verticalToolbar_.ClientToScreen(&rc);
        rectangleMenu.CreatePopupMenu();
        rectangleMenu.AppendMenu(MF_STRING, ID_RECTANGLE, TR("Rectangle"));
        rectangleMenu.AppendMenu(MF_STRING, ID_ROUNDEDRECTANGLE, TR("Rounded rectangle"));
        rectangleMenu.AppendMenu(MF_STRING, ID_ELLIPSE, TR("Ellipse"));
        TPMPARAMS excludeArea;
        ZeroMemory(&excludeArea, sizeof(excludeArea));
        excludeArea.cbSize = sizeof(excludeArea);
        excludeArea.rcExclude = rc;
        rectangleMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd,&excludeArea);
    } else if ( item->command == ID_FILLEDRECTANGLE || item->command ==ID_FILLEDROUNDEDRECTANGLE || item->command ==ID_FILLEDELLIPSE) {
        CMenu rectangleMenu;
        RECT rc = item->rect;
        verticalToolbar_.ClientToScreen(&rc);
        rectangleMenu.CreatePopupMenu();
        rectangleMenu.AppendMenu(MF_STRING, ID_FILLEDRECTANGLE, TR("Filled rectangle"));
        rectangleMenu.AppendMenu(MF_STRING, ID_FILLEDROUNDEDRECTANGLE, TR("Rounded rectangle"));
        rectangleMenu.AppendMenu(MF_STRING, ID_FILLEDELLIPSE, TR("Filled ellipse"));
        TPMPARAMS excludeArea;
        ZeroMemory(&excludeArea, sizeof(excludeArea));
        excludeArea.cbSize = sizeof(excludeArea);
        excludeArea.rcExclude = rc;
        rectangleMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    }
    else if (item->command == ID_BLURRINGRECTANGLE || item->command == ID_PIXELATERECTANGLE) {
        CMenu blurMenu;
        RECT rc = item->rect;
        verticalToolbar_.ClientToScreen(&rc);
        blurMenu.CreatePopupMenu();
        blurMenu.AppendMenu(MF_STRING, ID_BLURRINGRECTANGLE, TR("Blurring rectangle"));
        blurMenu.AppendMenu(MF_STRING, ID_PIXELATERECTANGLE, TR("Pixelation"));
        TPMPARAMS excludeArea;
        ZeroMemory(&excludeArea, sizeof(excludeArea));
        excludeArea.cbSize = sizeof(excludeArea);
        excludeArea.rcExclude = rc;
        blurMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    } else if ( item->command == ID_SAVE ) {
        CMenu rectangleMenu;
        RECT rc = item->rect;
        horizontalToolbar_.ClientToScreen(&rc);
        rectangleMenu.CreatePopupMenu();
        rectangleMenu.AppendMenu(MF_STRING, ID_SAVEAS, TR("Save as..."));
        TPMPARAMS excludeArea;
        ZeroMemory(&excludeArea, sizeof(excludeArea));
        excludeArea.cbSize = sizeof(excludeArea);
        excludeArea.rcExclude = rc;
        rectangleMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    } else if (item->command == ID_COPYBITMAPTOCLIBOARD) {
        CMenu rectangleMenu;
        RECT rc = item->rect;
        horizontalToolbar_.ClientToScreen(&rc);
        rectangleMenu.CreatePopupMenu();
        rectangleMenu.AppendMenu(MF_STRING, ID_COPYBITMAPTOCLIBOARDASDATAURI, TR("Copy in data:URI format"));
        rectangleMenu.AppendMenu(MF_STRING, ID_COPYBITMAPTOCLIBOARDASDATAURIHTML, TR("Copy in data:URI format (html)"));
        TPMPARAMS excludeArea;
        ZeroMemory(&excludeArea, sizeof(excludeArea));
        excludeArea.cbSize = sizeof(excludeArea);
        excludeArea.rcExclude = rc;
        rectangleMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    }
    else if (item->command == ID_SEARCHBYIMAGE) {
        auto* engineList = ServiceLocator::instance()->engineList();
        CMenu rectangleMenu;
        RECT rc = item->rect;
        horizontalToolbar_.ClientToScreen(&rc);
        rectangleMenu.CreatePopupMenu();

        int i = 0;
        auto* serverIconCache = ServiceLocator::instance()->serverIconCache();
        for (const auto& engine : *engineList) {
            if (engine->hasType(CUploadEngineData::TypeSearchByImageServer)) {
                CString itemText;
                itemText.Format(TR("Search by image (%s)"), U2W(engine->Name).GetString());
                MENUITEMINFO mi;
                ZeroMemory(&mi, sizeof(mi));
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
                mi.fType = MFT_STRING;
                mi.wID = ID_SEARCHBYIMAGE_START + i;
                mi.dwTypeData = const_cast<LPWSTR>(itemText.GetString());
                mi.cch = itemText.GetLength();
                mi.hbmpItem = serverIconCache->getIconBitmapForServer(engine->Name, dpi, true);

                if (mi.hbmpItem) {
                    mi.fMask |= MIIM_BITMAP;
                }
                rectangleMenu.InsertMenuItem(i, true, &mi);
                i++;
            }
        }

        TPMPARAMS excludeArea;
        ZeroMemory(&excludeArea, sizeof(excludeArea));
        excludeArea.cbSize = sizeof(excludeArea);
        excludeArea.rcExclude = rc;
        rectangleMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
    } else if (item->command == ID_MOREACTIONS) {
        showMoreActionsDropdownMenu(item);
    } else if (item->command == ID_UPLOAD) {
        auto serviceLocator = ServiceLocator::instance();
        auto* settings = serviceLocator->settings<WtlGuiSettings>();
        CServerSelectorControl serverSelectorControl(uploadEngineManager_, false, false);
        serverSelectorControl.setServersMask(CUploadEngineData::TypeImageServer | CUploadEngineData::TypeFileServer);
        serverSelectorControl.setTitle(TR("Server for quick screenshot uploading"));
        serverSelectorControl.setServerProfile(settings->quickScreenshotServer.getByIndex(0));
        RECT rc = item->rect;
        horizontalToolbar_.ClientToScreen(&rc);
        serverSelectorControl.showPopup(m_hWnd, rc, true);
        settings->quickScreenshotServer.getByIndex(0) = serverSelectorControl.serverProfile();
        serverDisplayName_ = CUploadEngineListBase::getServerDisplayName(serverSelectorControl.serverProfile().uploadEngineData());
        item->title = U2WC(getUploadButtonText());
        item->hint = WinUtils::TrimStringEnd(item->title, UPLOAD_BUTTON_MAX_LENGTH);
        horizontalToolbar_.update();
    }
    return 0;
}


LRESULT ImageEditorWindow::OnDropDownClicked(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    Toolbar::Item* item = reinterpret_cast<Toolbar::Item*>(wParam);
    
    return 0;
}

LRESULT ImageEditorWindow::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    PostMessage(WM_CLOSE);
    return 0;
}

void ImageEditorWindow::createToolbars()
{
    CRect rc;
    GetClientRect(&rc);
    bool child = displayMode_ == wdmWindowed;
    if ( !horizontalToolbar_.Create(m_hWnd, !allowAltTab_, child, child ? GetSysColor(COLOR_APPWORKSPACE): RGB(255, 50, 56)) ) {
        LOG(ERROR) << "Failed to create horizontal toolbar";
        return;
    }
    int dpiX;
    {
        CClientDC dc(m_hWnd);
        dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    }

    if (displayMode_ ==  wdmFullscreen && rc.Width() < MulDiv(800, dpiX, USER_DEFAULT_SCREEN_DPI)) {
        horizontalToolbar_.setShowButtonText(false);
    }
    if (showAddToWizardButton_ && !onlySelectRegion_) {
        CString buttonHint= CString(TR("Add to the list")) + (showUploadButton_ ? _T("") : _T(" (Enter)"));
        horizontalToolbar_.addButton(Toolbar::Item(CString(TR("Add to the list")), loadToolbarIcon(IDB_ICONADDPNG),ID_ADDTOWIZARD, buttonHint));
    }

    if (onlySelectRegion_) {
        CString buttonHint = CString(TR("Continue")) + _T(" (Enter)");
        horizontalToolbar_.addButton(Toolbar::Item(CString(TR("Continue")), loadToolbarIcon(IDB_ICONOK), ID_CONTINUE, buttonHint));
    } else {
        if (showUploadButton_) {
            CString fullUploadButtonText = U2WC(getUploadButtonText());
            CString uploadButtonText = WinUtils::TrimStringEnd(fullUploadButtonText, UPLOAD_BUTTON_MAX_LENGTH);
            horizontalToolbar_.addButton(Toolbar::Item(uploadButtonText, loadToolbarIcon(IDB_ICONUPLOADPNG), ID_UPLOAD, fullUploadButtonText + _T(" (Enter)"), Toolbar::itComboButton));
        }

        bool doClose = checkCloseWindowAfterAction();

        const std::string secondLine = doClose ? _("Pressing this button while holding Shift will not close the window.") : _("Pressing this button while holding Shift will close the window.");

        std::string saveFormatStr = doClose ? _("Save and close (%s)") : _("Save (%s)");
        if (canCloseAfterAction()) {
            saveFormatStr += "\r\n\r\n";
            saveFormatStr += secondLine;
        }

        const std::string saveButtonHint = str(IuStringUtils::FormatNoExcept(saveFormatStr) % "Ctrl+S");
        horizontalToolbar_.addButton(Toolbar::Item(TR("Save"), loadToolbarIcon(IDB_ICONSAVEPNG), ID_SAVE, U2W(saveButtonHint), Toolbar::itComboButton));

        std::string copyFormatStr = doClose ? _("Copy to clipboard and close (%s)") : _("Copy to clipboard (%s)");
        if (canCloseAfterAction()) {
            copyFormatStr += "\r\n\r\n";
            copyFormatStr += secondLine;
        }

        const std::string copyButtonHint = str(IuStringUtils::FormatNoExcept(copyFormatStr) % "Ctrl+C");
        horizontalToolbar_.addButton(Toolbar::Item(TR("Copy"), loadToolbarIcon(IDB_ICONCLIPBOARDPNG), ID_COPYBITMAPTOCLIBOARD, U2W(copyButtonHint), Toolbar::itComboButton));

        if (displayMode_ == wdmFullscreen) {
            horizontalToolbar_.addButton(Toolbar::Item(TR("Record"), loadToolbarIcon(IDB_ICONRECORD), ID_RECORDSCREEN, TR("Record") + CString(_T(" (Ctrl+R)")), Toolbar::itButton));
        }

        const std::string searchButtonText = str(IuStringUtils::FormatNoExcept(_("Search on %s")) % searchEngine_.serverName());
        std::string searchFormatStr = doClose  ? _("Search on %1% and close (%2%)") : _("Search on %1% (%2%)");
        if (canCloseAfterAction()) {
            searchFormatStr += "\r\n\r\n";
            searchFormatStr += secondLine;
        }

        const std::string searchButtonHint = str(IuStringUtils::FormatNoExcept(searchFormatStr) % searchEngine_.serverName() % "Ctrl+F");

        horizontalToolbar_.addButton(Toolbar::Item(U2W(searchButtonText), loadToolbarIcon(IDB_ICONSEARCH), ID_SEARCHBYIMAGE, U2W(searchButtonHint), Toolbar::itComboButton));
        horizontalToolbar_.addButton(Toolbar::Item({}, loadToolbarIcon(IDB_ICONROTATEFLIP), ID_MOREACTIONS, TR("Rotate or flip"), Toolbar::itComboButton));

        horizontalToolbar_.addButton(Toolbar::Item({}, loadToolbarIcon(IDB_ICONPRINT), ID_PRINTIMAGE, TR("Print...") + CString(_T(" (Ctrl+P)")), Toolbar::itButton));
    }
    horizontalToolbar_.addButton(Toolbar::Item(TR("Close"), std::shared_ptr<Gdiplus::Bitmap>(), ID_CLOSE, TR("Close") + CString(_T(" (Esc)"))));
    horizontalToolbar_.AutoSize();

    if ( displayMode_ != wdmFullscreen ) {
        horizontalToolbar_.ShowWindow(SW_SHOW);
    }

    if ( !verticalToolbar_.Create(m_hWnd, !allowAltTab_, child, child ? GetSysColor(COLOR_APPWORKSPACE) : RGB(255, 50, 56)) ) {
        LOG(ERROR) << "Failed to create vertical toolbar";
    }

    if (!onlySelectRegion_) {
        /* enum DrawingToolHotkey {kMoveKey = 'V', kBrushKey = 'B', kTextKey = 'T', kRectangleKey = 'U', kColorPickerKey = 'I', kCropKey = 'C', // photoshop keys
        kMarkerKey = 'H', kBlurringRectangleKey = 'R', kArrowKey = 'A', kLineKey = 'L'
        };
        */
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_TOOLMOVEICONPNG), ID_MOVE, TR("Move") + CString(_T(" (")) + (char)kMoveKey + CString(_T(")")), Toolbar::itButton, true, 1));
        //verticalToolbar_.addButton(Toolbar::Item(CString(),  loadToolbarIcon(IDB_ICONTOOLSELECTION),ID_SELECTION,TR("Selection"), Toolbar::itButton, true, 1));

        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_TOOLCROPPING), ID_CROP, TR("Crop") + CString(_T(" (")) + (char)kCropKey + CString(_T(")")), Toolbar::itButton, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLBRUSHPNG), ID_BRUSH, TR("Brush") + CString(_T(" (")) + (char)kBrushKey + CString(_T(")")), Toolbar::itButton, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLMARKER), ID_MARKER, TR("Highlight") + CString(_T(" (")) + (char)kMarkerKey + CString(_T(")")), Toolbar::itButton, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLLINE), ID_LINE, TR("Line") + CString(_T(" (")) + (char)kLineKey + CString(_T(")")), Toolbar::itButton, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLARROWPNG), ID_ARROW, TR("Arrow") + CString(_T(" (")) + (char)kArrowKey + CString(_T(")")), Toolbar::itButton, true, 1));

        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLRECTANGLEPNG), ID_RECTANGLE, TR("Rectangle") + CString(_T(" (")) + (char)kRectangleKey + CString(_T(")")), Toolbar::itTinyCombo, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLFILLEDRECTANGLE), ID_FILLEDRECTANGLE, TR("Filled rectangle") + CString(_T(" (")) + (char)kFilledRectangle + CString(_T(")")), Toolbar::itTinyCombo, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLTEXTPNG), ID_TEXT, TR("Text") + CString(_T(" (")) + (char)kTextKey + CString(_T(")")), Toolbar::itButton, true, 1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLSTEP), ID_STEPNUMBER, TR("Step") + CString(_T(" (")) + (char)kStepNumber + CString(_T(")")), Toolbar::itButton, true, 1));

        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONTOOLBLURINGRECTANGLEPNG), ID_BLURRINGRECTANGLE, TR("Blurring rectangle") + CString(_T(" (")) + (char)kBlurringRectangleKey + CString(_T(")")), Toolbar::itTinyCombo, true, 1));

        //verticalToolbar_.addButton(Toolbar::Item(CString(),  loadToolbarIcon(IDB_ICONTOOLBRUSHPNG), ID_BLUR,TR("Blur"), Toolbar::itButton, true,1));
        verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONCOLORPICKERPNG), ID_COLORPICKER, TR("Color chooser") + CString(_T(" (")) + (char)kColorPickerKey + CString(_T(")")), Toolbar::itButton, true, 1));

        int index = verticalToolbar_.addButton(Toolbar::Item(CString(), loadToolbarIcon(IDB_ICONUNDOPNG), ID_UNDO, TR("Undo") + CString(L" (Ctrl+Z)"), Toolbar::itButton, false));

        Toolbar::Item colorsButton(CString(), loadToolbarIcon(IDB_ICONUNDOPNG), ID_UNDO, {}, Toolbar::itButton, false);
        colorsDelegate_ = std::make_unique<ColorsDelegate>(&verticalToolbar_, index + 1, canvas_.get());
        colorsButton.itemDelegate = colorsDelegate_.get();
        colorsDelegate_->setBackgroundColor(canvas_->getBackgroundColor());
        colorsDelegate_->setForegroundColor(canvas_->getForegroundColor());
        verticalToolbar_.addButton(colorsButton);

        verticalToolbar_.AutoSize();
        if (displayMode_ != wdmFullscreen) {
            verticalToolbar_.ShowWindow(SW_SHOW);
        }
        horizontalToolbar_.penSizeSlider_.SetPos(canvas_->getPenSize());
        horizontalToolbar_.roundRadiusSlider_.SetPos(canvas_->getRoundingRadius());
        horizontalToolbar_.blurRadiusSlider_.SetPos(round(canvas_->getBlurRadius() * BLUR_RADIUS_PRECISION));
        horizontalToolbar_.setStepFontSize(canvas_->getStepFontSize());
        horizontalToolbar_.setStepInitialValue(1);
        horizontalToolbar_.setArrowType(static_cast<int>(canvas_->getArrowMode()));
        horizontalToolbar_.setFillBackgroundCheckbox(canvas_->getFillTextBackground());
        horizontalToolbar_.setInvertSelectionCheckbox(canvas_->getInvertSelection());
        horizontalToolbar_.setDrawBorderCheckbox(canvas_->getDrawBorder());
    }
}


void ImageEditorWindow::OnCropChanged(int x, int y, int w, int h)
{
    if ( cropToolTip_ ) {
        ::DestroyWindow(cropToolTip_);
        cropToolTip_ = 0;
    }

    if ( displayMode_ != wdmFullscreen ) {
        return;
    }
    enum ToolbarPosition { pBottomRight, pBottomLeft, pTopRight, pTopLeft, pBottomRightInner };

    POINT scrollOffset;
    m_view.GetScrollOffset(scrollOffset);
    x -= scrollOffset.x;
    y -= scrollOffset.y;
    RECT horRc, vertRc;
    RECT clientRect;
    GetClientRect(&clientRect);
    horizontalToolbar_.GetClientRect(&horRc);
    verticalToolbar_.GetClientRect(&vertRc);

    ToolbarPosition pos = pBottomRight ;
    if ( y + h + horRc.bottom <= canvas_->getHeigth()  && x + w + vertRc.right <= clientRect.right  ) {
        pos = pBottomRight ;
    } else if ( y >= horRc.bottom && x >= vertRc.right )  {
         pos = pTopLeft;
    } else if ( y >= horRc.bottom && x + w + vertRc.right <= clientRect.right ) {
        pos = pTopRight;
    } else if ( x >= vertRc.right && y +h+horRc.bottom  <= clientRect.bottom) {
        pos = pBottomLeft;
    }
    POINT horToolbarPos = {0, 0};
    POINT vertToolbarPos = {0, 0};

    if ( pos == pBottomRight ) {
        horToolbarPos.x = x + w - horRc.right;
        horToolbarPos.y =  y + h + kToolbarOffset;

        vertToolbarPos.x = x + w + kToolbarOffset ;
        vertToolbarPos.y = y + h - vertRc.bottom;

        horToolbarPos.x = std::min(std::max<LONG>( horToolbarPos.x, 0),clientRect.right - vertRc.right - horRc.right);
        horToolbarPos.y = std::min( std::max( horToolbarPos.y, vertRc.bottom + kToolbarOffset ) , clientRect.bottom - horRc.bottom - kToolbarOffset);
        vertToolbarPos.x = std::min(std::max( vertToolbarPos.x, horRc.right + kToolbarOffset ), clientRect.right - vertRc.right);
        vertToolbarPos.y = std::min(std::max<LONG>( vertToolbarPos.y, 0), clientRect.bottom - horRc.bottom - kToolbarOffset - vertRc.bottom);

    } else if ( pos == pTopLeft ) {
        horToolbarPos.x = x;
        horToolbarPos.y =  y - horRc.bottom - kToolbarOffset;

        vertToolbarPos.x = x - vertRc.right - kToolbarOffset;
        vertToolbarPos.y = y ;

        horToolbarPos.x = std::min(std::max(horToolbarPos.x, vertRc.right), clientRect.right - horRc.right - kToolbarOffset);
        horToolbarPos.y = std::min(std::max<LONG>(horToolbarPos.y, 0), clientRect.bottom - vertRc.bottom - horRc.bottom - kToolbarOffset);
        vertToolbarPos.x = std::min(std::max<LONG>(vertToolbarPos.x, 0), clientRect.right - horRc.right - vertRc.right - kToolbarOffset);
        vertToolbarPos.y = std::min(std::max(vertToolbarPos.y, horRc.bottom), clientRect.bottom - vertRc.bottom - kToolbarOffset);
    } else if ( pos == pTopRight ) {
        horToolbarPos.x = x + w - horRc.right - kToolbarOffset;
        horToolbarPos.y =  y - horRc.bottom - kToolbarOffset;

        vertToolbarPos.x = x + w + kToolbarOffset;
        vertToolbarPos.y = y ;

        horToolbarPos.x = /*min*(*/std::max<LONG>(horToolbarPos.x, 0)/*, clientRect.right - horRc.right - kToolbarOffset)*/;
        horToolbarPos.y = std::min(std::max<LONG>(horToolbarPos.y, 0), clientRect.bottom - vertRc.bottom - horRc.bottom - kToolbarOffset);
        vertToolbarPos.x = /*min(*/std::max(vertToolbarPos.x, horRc.right + kToolbarOffset)/*, clientRect.right - horRc.right- vertRc.right - kToolbarOffset)*/;
        vertToolbarPos.y = std::min(std::max(vertToolbarPos.y, horRc.bottom), clientRect.bottom - vertRc.bottom - kToolbarOffset);
    } else if ( pos == pBottomLeft ) {
        horToolbarPos.x = x;
        horToolbarPos.y =  y + h + kToolbarOffset;

        vertToolbarPos.x = x - vertRc.right - kToolbarOffset;
        vertToolbarPos.y = y + h - vertRc.bottom;

        horToolbarPos.x = std::min(std::max(horToolbarPos.x, vertRc.right), clientRect.right - horRc.right - kToolbarOffset);

        vertToolbarPos.x = std::min(std::max<LONG>(vertToolbarPos.x, 0), clientRect.right - horRc.right - vertRc.right - kToolbarOffset);

        //horToolbarPos.y = max( horToolbarPos.y, vertRc.bottom + kToolbarOffset );
        horToolbarPos.y = std::max(horToolbarPos.y, vertRc.bottom + kToolbarOffset);
        vertToolbarPos.y = std::max<LONG>(vertToolbarPos.y, 0);
    }
    //SIZE toolbarRect = {  horRc.right + vertRc.right, vertRc.bottom + horRc.bottom}

    ClientToScreen(&horToolbarPos);
    ClientToScreen(&vertToolbarPos);

    horizontalToolbar_.SetWindowPos(0, horToolbarPos.x, horToolbarPos.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    //horizontalToolbar_.SetWindowPos(0, horToolbarPos.x, horToolbarPos.y, 0, 0, SWP_NOSIZE);
    verticalToolbar_.SetWindowPos(0, vertToolbarPos.x, vertToolbarPos.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SetActiveWindow();
}

void ImageEditorWindow::OnCropFinished(int x, int y, int w, int h)
{
    if (!onlySelectRegion_) {
        showApplyButtons();
    }

    OnCropChanged(x,y,w,h);
    if (displayMode_ != wdmFullscreen) {
        return;
    }
    if ( !horizontalToolbar_.IsWindowVisible() || !verticalToolbar_.IsWindowVisible() ) {
        verticalToolbar_.ShowWindow(SW_SHOW);
        horizontalToolbar_.ShowWindow(SW_SHOW);
    }
}

void ImageEditorWindow::OnDrawingToolChanged(DrawingToolType drawingTool)
{
    updateToolbarDrawingTool(drawingTool);
    SendMessage(WM_SETCURSOR,0,0);
}

void ImageEditorWindow::OnTextEditStarted(ImageEditor::TextElement * textElement)
{
    auto control = std::dynamic_pointer_cast<InputBoxControl>(textElement->getInputBox());
    if (!control) {
        return;
    }
    RECT inputControlRect;
    control->GetWindowRect(&inputControlRect);
    int kOffset = 30;
    RECT viewRect;
    RECT textParamsWindowRect;
    textParamsWindow_.GetWindowRect(&textParamsWindowRect);
    m_view.GetWindowRect(&viewRect);
    int x = inputControlRect.left;
    int y = 0;
    if ( inputControlRect.top > viewRect.top + (textParamsWindowRect.bottom - textParamsWindowRect.top) + kOffset ) {
        y = inputControlRect.top - (textParamsWindowRect.bottom - textParamsWindowRect.top) - kOffset;
    } else {
        y = inputControlRect.bottom + kOffset;
    }
    textParamsWindow_.SetWindowPos(0,x,y, 0,0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);

    textParamsWindow_.ShowWindow(SW_SHOW);
    control->SetFocus();
}

void ImageEditorWindow::OnTextEditFinished(ImageEditor::TextElement * textElement)
{
    textParamsWindow_.ShowWindow(SW_HIDE);
}

void ImageEditorWindow::OnSelectionChanged()
{
    updateRoundingRadiusSlider();
    updateFontSizeControls();
}

void ImageEditorWindow::updateRoundingRadiusSlider()
{
    bool showLineWidth = ( currentDrawingTool_ != DrawingToolType::dtStepNumber
        && currentDrawingTool_ != DrawingToolType::dtText
        && currentDrawingTool_ != DrawingToolType::dtCrop
        && currentDrawingTool_ != DrawingToolType::dtBlurringRectangle
        && currentDrawingTool_ != DrawingToolType::dtPixelateRectangle
    );
    horizontalToolbar_.showPenSize(showLineWidth);

    bool showRoundingRadiusSlider = currentDrawingTool_ == DrawingToolType::dtRoundedRectangle || currentDrawingTool_ == DrawingToolType::dtFilledRoundedRectangle || canvas_->isRoundingRectangleSelected();
    horizontalToolbar_.roundRadiusLabel_.ShowWindow( showRoundingRadiusSlider ? SW_SHOW: SW_HIDE );
    horizontalToolbar_.roundRadiusSlider_.ShowWindow( showRoundingRadiusSlider ? SW_SHOW: SW_HIDE );

    bool showBlurRadiusSlider = currentDrawingTool_ == DrawingToolType::dtBlurringRectangle
        || currentDrawingTool_ == DrawingToolType::dtPixelateRectangle;

    horizontalToolbar_.blurRadiusSlider_.ShowWindow(showBlurRadiusSlider ? SW_SHOW : SW_HIDE);
    horizontalToolbar_.blurRadiusLabel_.ShowWindow(showBlurRadiusSlider ? SW_SHOW : SW_HIDE);
    horizontalToolbar_.showInvertSelectionCheckbox(showBlurRadiusSlider);

    bool showFillBackgound = currentDrawingTool_ == DrawingToolType::dtText;
    horizontalToolbar_.showFillBackgroundCheckbox(showFillBackgound);

    bool showDrawBorder = currentDrawingTool_ == DrawingToolType::dtFilledRectangle
        || currentDrawingTool_ == DrawingToolType::dtFilledRoundedRectangle
        || currentDrawingTool_ == DrawingToolType::dtFilledEllipse;

    horizontalToolbar_.showDrawBorderCheckbox(showDrawBorder);

    horizontalToolbar_.showArrowTypeCombo(currentDrawingTool_ == DrawingToolType::dtArrow);

    showApplyButtons();
}

void ImageEditorWindow::updateFontSizeControls() {
    bool showFontSizeControls = currentDrawingTool_ == DrawingToolType::dtStepNumber;
    horizontalToolbar_.showStepFontSize(showFontSizeControls);
}

void ImageEditorWindow::updateSearchButton() {
    int buttonIndex = horizontalToolbar_.getItemIndexByCommand(ID_SEARCHBYIMAGE);
    if (buttonIndex != -1) {
        Toolbar::Item* item = horizontalToolbar_.getItem(buttonIndex);
        CString searchEngineName = U2W(searchEngine_.serverName());
        item->title.Format(TR("Search on %s"), searchEngineName.GetString());
        horizontalToolbar_.AutoSize();
        //horizontalToolbar_.Invalidate(FALSE);
    }
}

std::shared_ptr<Gdiplus::Bitmap> ImageEditorWindow::loadToolbarIcon(int resource, bool resize)
{
    using namespace Gdiplus;

    std::shared_ptr<Bitmap> bm(ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(resource), _T("PNG")));
    if (!resize) {
        return bm;
    }
    UINT dpi = DPIHelper::GetDpiForWindow(m_hWnd);

    int cx = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int cy = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    auto newBm = std::make_shared<Bitmap>(cx, cy, PixelFormat32bppARGB);
    Graphics gr(newBm.get());
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gr.SetPixelOffsetMode(PixelOffsetModeHalf);
    gr.DrawImage(bm.get(), 0, 0, cx, cy);

    return newBm;
}


std::shared_ptr<Gdiplus::Bitmap> ImageEditorWindow::loadMenuIcon(int resource, bool resize /*= false*/) {
    using namespace Gdiplus;

    std::shared_ptr<Bitmap> bm(ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(resource), _T("PNG")));
    if (!resize) {
        return bm;
    }
    UINT dpi = DPIHelper::GetDpiForDialog(m_hWnd);

    int cx = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int cy = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    auto newBm = std::make_shared<Bitmap>(cx, cy, PixelFormat32bppARGB);
    Graphics gr(newBm.get());
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gr.SetPixelOffsetMode(PixelOffsetModeHalf);
    gr.DrawImage(bm.get(), 0, 0, cx, cy);

    return newBm;
}

void ImageEditorWindow::EndDialog(DialogResult dr)
{
    allowAltTab_ = false;
    dialogResult_ = dr;
    PostQuitMessage(0);
}

LRESULT ImageEditorWindow::OnMenuItemClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if (GetFocus() != m_view.m_hWnd)
    {
        ::SetFocus(m_view.m_hWnd);
    }
    DrawingToolType toolId =  static_cast<DrawingToolType>(menuItems_[wID]);
    canvas_->setDrawingToolType(toolId);
    updateToolbarDrawingTool(toolId);
    return 0;
}

LRESULT ImageEditorWindow::OnUndoClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->undo();
    return 0;
}

LRESULT ImageEditorWindow::OnTextClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->setDrawingToolType(DrawingToolType::dtText);
    updateToolbarDrawingTool(DrawingToolType::dtText);
    return 0;
}

LRESULT ImageEditorWindow::OnClickedClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    onClose();
    return 0;
}

void ImageEditorWindow::onClose() {
    DialogResult dr = drCancel;
    if (askBeforeClose_ && !onlySelectRegion_ && canvas_->isDocumentModified()) {
        int msgBoxResult = GuiTools::LocalizedMessageBox(m_hWnd, TR("Save changes?"), APP_NAME, MB_YESNOCANCEL | MB_ICONQUESTION);
        if (msgBoxResult == IDYES) {
            dr = outFileName_.IsEmpty() ? drCancel : drSave;
            if(!onSave(false)) {
                return;
            }
        }
        else if (msgBoxResult == IDCANCEL) {
            return;
        }
    }
    EndDialog(dr);
}

LRESULT ImageEditorWindow::OnClickedAddToWizard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    resultingBitmap_ = canvas_->getBitmapForExport();
    canvas_->setDocumentModified(false);

    if (!resultingBitmap_) {
        LOG(ERROR) << "canvas_->getBitmapForExport() returned NULL";
        return 0;
    }

    EndDialog(drAddToWizard);

    return 0;
}

LRESULT ImageEditorWindow::OnClickedUpload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    resultingBitmap_ = canvas_->getBitmapForExport();
    canvas_->setDocumentModified(false);

    if (!resultingBitmap_) {
        LOG(ERROR) << "canvas_->getBitmapForExport() returned NULL";
        return 0;
    }

    EndDialog(drUpload);
    return 0;
}

LRESULT ImageEditorWindow::OnClickedShare(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (saveDocument()) {
        EndDialog(drShare);
    }
    return 0;
}

LRESULT ImageEditorWindow::OnClickedSave(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool closeFlag = wID == ID_SAVE_ALT || (wNotifyCode != 1 && (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    onSave(closeFlag);
    return 0;
}

LRESULT ImageEditorWindow::OnClickedSaveAs(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool closeFlag = wID == ID_SAVEAS_ALT || (wNotifyCode != 1 && (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    onSaveAs(closeFlag);
    return 0;
}

LRESULT ImageEditorWindow::OnClickedCopyToClipboard(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    bool closeFlag = wID == ID_COPYBITMAPTOCLIBOARD_ALT || (wNotifyCode != 1 && (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    copyBitmapToClipboard(ClipboardFormat::Bitmap, closeFlag);

    return 0;
}

LRESULT ImageEditorWindow::OnClickedCopyToClipboardAsDataUri(WORD wNotifyCode, WORD wID, HWND, BOOL&) {
    bool closeFlag = wID == ID_COPYBITMAPTOCLIBOARDASDATAURI_ALT || (wNotifyCode != 1 && (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    copyBitmapToClipboard(ClipboardFormat::DataUri, wID == ID_COPYBITMAPTOCLIBOARDASDATAURI_ALT);
    return 0;
}

LRESULT ImageEditorWindow::OnClickedCopyToClipboardAsDataUriHtml(WORD wNotifyCode, WORD wID, HWND, BOOL&) {
    bool closeFlag = wID == ID_COPYBITMAPTOCLIBOARDASDATAURIHTML_ALT || (wNotifyCode != 1 && (GetKeyState(VK_SHIFT) & 0x8000) != 0);
    copyBitmapToClipboard(ClipboardFormat::DataUriHtml, closeFlag);
    return 0;
}

LRESULT ImageEditorWindow::OnPrintImage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (GetFocus() != m_view.m_hWnd) {
        ::SetFocus(m_view.m_hWnd);
    }
    const bool res = saveDocument(); // Save screenshot in default format
    if (res) {
        const std::vector<CString> files = { outFileName_ };
        WinUtils::DisplaySystemPrintDialogForImage(files, m_hWnd);
        /*if (displayMode_ == wdmFullscreen && !(GetKeyState(VK_SHIFT) & 0x8000)) {
            EndDialog(drPrintRequested);
        }*/
    }
    return res;
}

LRESULT ImageEditorWindow::OnSearchByImage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int serverIndex = -1;
    bool closeFlag = false;
    if (wID >= ID_SEARCHBYIMAGE_START && wID <= ID_SEARCHBYIMAGE_END) {
        serverIndex = wID - ID_SEARCHBYIMAGE_START;
    } else if (wID >= ID_SEARCHBYIMAGE_ALT_START && wID <= ID_SEARCHBYIMAGE_ALT_END) {
        serverIndex = wID - ID_SEARCHBYIMAGE_ALT_START;
        closeFlag = true;
    } else {
        closeFlag = (wID == ID_SEARCHBYIMAGE_ALT);
    }

    closeFlag = closeFlag || (wNotifyCode != 1 && (GetKeyState(VK_SHIFT) & 0x8000) != 0);

    if (serverIndex != -1) {
        int i = 0;
        auto* engineList = ServiceLocator::instance()->engineList();
        for (const auto& engine : *engineList) {
            if (engine->hasType(CUploadEngineData::TypeSearchByImageServer)) {
                if (i == serverIndex) {
                    searchEngine_ = ServerProfile(engine->Name);

                    break;
                }
                i++;
            }
        }
    }

    if (searchEngine_.isNull()) {
        return 0;
    }

    updateSearchButton();
    const CString fileName = saveToTempFile();
    if (fileName.IsEmpty()) {
        LOG(ERROR) << "Unable to create temporary file";
        return 0;
    }
    auto uploadManager = ServiceLocator::instance()->uploadManager();
    CSearchByImageDlg dlg(uploadManager, searchEngine_, fileName);
    if (dlg.DoModal(m_hWnd) == IDOK) {
        if (canCloseAfterAction() && (closeFlag ^ checkCloseWindowAfterAction())) {
            EndDialog(drSearch);
        }
    }

    return 0;
}

LRESULT ImageEditorWindow::OnFontSizeChanged(UINT, WPARAM, LPARAM, BOOL&) {
    int fontSize = horizontalToolbar_.getFontSize();
    if (!fontSize) {
        fontSize = Canvas::kDefaultStepFontSize;
    }
    canvas_->setStepFontSize(fontSize);
    return 0;
}

bool ImageEditorWindow::createTooltip() {
    // Create a tooltip.
    cropToolTip_ = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        m_view.m_hWnd, NULL, _Module.GetModuleInstance(),NULL);

    ::SetWindowPos(cropToolTip_, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Set up "tool" information. In this case, the "tool" is the entire parent window.
    TOOLINFO ti = {};
    CString title = TR("Select region");
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_SUBCLASS;
    ti.hwnd     = m_view.m_hWnd;
    ti.hinst    = _Module.GetModuleInstance();
    ti.lpszText = const_cast<LPWSTR>(title.GetString());
    m_view.GetClientRect(&ti.rect);

    // Associate the tooltip with the "tool" window.
    SendMessage(cropToolTip_, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    return true;
}

void ImageEditorWindow::updatePixelLabels()
{
    horizontalToolbar_.pixelLabel_.SetWindowText(WinUtils::IntToStr(canvas_->getPenSize()) + L" px");
    horizontalToolbar_.roundRadiusLabel_.SetWindowText(WinUtils::IntToStr(canvas_->getRoundingRadius()) + L" px");
    std::string s = str(boost::format("%0.2f") % canvas_->getBlurRadius());
    horizontalToolbar_.blurRadiusLabel_.SetWindowText(IuCoreUtils::Utf8ToWstring(s).c_str());
}

bool ImageEditorWindow::onSaveAs(bool closeFlag)
{
    IMyFileDialog::FileFilterArray filters = {
        { _T("PNG"), CString(_T("*.png")) },
        { _T("JPEG"), CString(_T("*.jpg;*.jpeg")) },
        { _T("WEBP"), CString(_T("*.webp")) },
        { TR("All files"), _T("*.*") }
    };

    CString fileName = makeFileName();
    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, WinUtils::GetFilePath(fileName), CString(), filters, false, false);
    dlg->setFileName(WinUtils::DoExtractFileName(fileName));
    CString ext = WinUtils::GetFileExt(fileName);
    ext.MakeLower();
    dlg->setDefaultExtension(ext);

    auto it = std::find_if(filters.begin(), filters.end(), [ext] (auto&& s) {
        return s.first.CompareNoCase(ext) == 0;
    });

    if (it != filters.end()) {
        size_t index = it - filters.begin() + 1; // This is a one-based index, not zero-based.
        dlg->setFileTypeIndex(index);
    }
    enableToolbarsIfNecessary(false);
    if (dlg->DoModal(m_hWnd) != IDOK) {
        enableToolbarsIfNecessary(true);
        return false;
    }
    enableToolbarsIfNecessary(true);
    outFileName_ = dlg->getFile();

    if (saveDocument(ClipboardFormat::None, true)) {
        auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
        /*setSuggestedFileName(IuCommonFunctions::GenerateFileName(settings->ScreenshotSettings.FilenameTemplate, ++IuCommonFunctions::screenshotIndex,
            CPoint(canvas_->currentDocument()->getWidth(), canvas_->currentDocument()->getHeight())));*/
        if (canCloseAfterAction() && (closeFlag ^ checkCloseWindowAfterAction())) {
            EndDialog(drSave);
        }
    }
    return true;
}

void ImageEditorWindow::saveSettings()
{
    if ( configurationProvider_ ) {
        configurationProvider_->setPenSize(canvas_->getPenSize());
        configurationProvider_->setForegroundColor(canvas_->getForegroundColor());
        configurationProvider_->setBackgroundColor(canvas_->getBackgroundColor());
        configurationProvider_->setFont(canvas_->getFont());
        configurationProvider_->setRoundingRadius(canvas_->getRoundingRadius());
        configurationProvider_->setBlurRadius(canvas_->getBlurRadius());
        configurationProvider_->setSearchEngine(searchEngine_);
        configurationProvider_->setFillTextBackground(canvas_->getFillTextBackground());
        configurationProvider_->setInvertSelection(canvas_->getInvertSelection());
        configurationProvider_->setStepForegroundColor(canvas_->getStepForegroundColor());
        configurationProvider_->setStepBackgroundColor(canvas_->getStepBackgroundColor());
        configurationProvider_->setArrowMode(static_cast<int>(canvas_->getArrowMode()));
        configurationProvider_->setDrawBorder(canvas_->getDrawBorder());
        configurationProvider_->saveConfiguration();
    }
}

bool ImageEditorWindow::copyBitmapToClipboard(ClipboardFormat format, bool closeFlag) {
    if (GetFocus() != m_view.m_hWnd) {
        ::SetFocus(m_view.m_hWnd);
    }
    const bool res = saveDocument(format);
    if (res && canCloseAfterAction() && (closeFlag ^ checkCloseWindowAfterAction())) {
        EndDialog(drCopyToClipboard);
    }
    return res;
}

BOOL ImageEditorWindow::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) {
        // Disable accelerators for child controls of the view window (now it's just InputBoxControl)
        // and all edit controls in toolbars.
        HWND parent = ::GetParent(pMsg->hwnd);

        if (parent) {
            if (parent == m_view.m_hWnd) {
                return FALSE;
            }

            if (parent == horizontalToolbar_.m_hWnd || parent == verticalToolbar_.m_hWnd) {
                TCHAR className[MAX_PATH]{};
                if (GetClassName(pMsg->hwnd, className, MAX_PATH) && !lstrcmp(className, _T("Edit"))) {
                    return FALSE;
                }
            }
        }
    }

    if (TranslateAccelerator(m_hWnd, accelerators_, pMsg)) {
        return TRUE;
    }

    return FALSE;
}

bool ImageEditorWindow::onSave(bool closeFlag) {

    if (GetFocus() != m_view.m_hWnd) {
        ::SetFocus(m_view.m_hWnd);
    }

    if (!sourceFileName_.IsEmpty()) {
        outFileName_ = sourceFileName_;
    } else if (openedAfterScreenshot()) {
        CString fileName = makeFileName();
        if (WinUtils::GetFilePath(fileName).IsEmpty()) {
            return onSaveAs(closeFlag);
        }
        outFileName_ = fileName;
    } else {
        return onSaveAs(closeFlag);
    }

    bool res = saveDocument();

    if (res && canCloseAfterAction() && (closeFlag ^ checkCloseWindowAfterAction())) {
        EndDialog(drSave);
    }

    return res;
}

LRESULT ImageEditorWindow::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    HWND hwndSender = reinterpret_cast<HWND>(lParam);
    if ( hwndSender == horizontalToolbar_.penSizeSlider_.m_hWnd  ) {
        int penSize = HIWORD(wParam);
        switch( LOWORD(wParam ) ) {
        case TB_ENDTRACK:
            penSize = horizontalToolbar_.penSizeSlider_.GetPos();
        case TB_THUMBPOSITION:
            canvas_->endPenSizeChanging(penSize);
            updatePixelLabels();
            prevPenSize_ = 0;
            break;
        case TB_PAGEDOWN:
        case TB_PAGEUP:
        case TB_TOP:
        case TB_LINEDOWN:
        case TB_LINEUP:
        case TB_BOTTOM:
            penSize = horizontalToolbar_.penSizeSlider_.GetPos();
        case TB_THUMBTRACK:
            if ( !prevPenSize_ ) {
                prevPenSize_ = canvas_->getPenSize();
                canvas_->beginPenSizeChanging();
            }
            canvas_->setPenSize(penSize);
            updatePixelLabels();
            break;
        }
    } else if ( hwndSender == horizontalToolbar_.roundRadiusSlider_.m_hWnd ) {
        int roundingRadius = HIWORD(wParam);
        switch( LOWORD(wParam ) ) {
        case TB_ENDTRACK:
            roundingRadius = horizontalToolbar_.roundRadiusSlider_.GetPos();
        case TB_THUMBPOSITION:
            canvas_->endRoundingRadiusChanging(roundingRadius);
            updatePixelLabels();
            prevRoundingRadius_ = 0;
            break;
        case TB_PAGEDOWN:
        case TB_PAGEUP:
        case TB_TOP:
        case TB_LINEDOWN:
        case TB_LINEUP:
        case TB_BOTTOM:
            roundingRadius = horizontalToolbar_.roundRadiusSlider_.GetPos();
        case TB_THUMBTRACK:
            if ( !prevRoundingRadius_ ) {
                prevRoundingRadius_ = canvas_->getRoundingRadius();
                canvas_->beginRoundingRadiusChanging();
            }
            canvas_->setRoundingRadius(roundingRadius);
            updatePixelLabels();
            break;
        }
    }
    else if (hwndSender == horizontalToolbar_.blurRadiusSlider_.m_hWnd) {
        float blurRadius = HIWORD(wParam)/ BLUR_RADIUS_PRECISION;
        switch (LOWORD(wParam)) {
        case TB_ENDTRACK:
            blurRadius = horizontalToolbar_.blurRadiusSlider_.GetPos()/ BLUR_RADIUS_PRECISION;
        case TB_THUMBPOSITION:
            canvas_->endBlurRadiusChanging(blurRadius);
            updatePixelLabels();
            prevBlurRadius_ = 0;
            break;
        case TB_PAGEDOWN:
        case TB_PAGEUP:
        case TB_TOP:
        case TB_LINEDOWN:
        case TB_LINEUP:
        case TB_BOTTOM:
            blurRadius = horizontalToolbar_.blurRadiusSlider_.GetPos()/ BLUR_RADIUS_PRECISION;
        case TB_THUMBTRACK:
            if (!prevRoundingRadius_) {
                prevBlurRadius_ = canvas_->getBlurRadius();
                canvas_->beginBlurRadiusChanging();
            }
            canvas_->setBlurRadius(blurRadius);
            updatePixelLabels();
            break;
        }
    }

    return 0;
}

LRESULT ImageEditorWindow::OnTextParamWindowFontChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    DWORD changeMask = lParam;
    canvas_->setFont(textParamsWindow_.getFont(), changeMask);
    return 0;
}

LRESULT ImageEditorWindow::OnUnselectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    canvas_->unselectAllElements();
    canvas_->updateView();
    return 0;
}

LRESULT ImageEditorWindow::OnStepInitialValueChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    int stepInitialValue = horizontalToolbar_.getStepInitialValue();
    if (!stepInitialValue) {
        stepInitialValue = 1;
    }
    canvas_->setStepInitialValue(stepInitialValue);
    return 0;
}


LRESULT ImageEditorWindow::OnFillBackgroundChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    canvas_->setFillTextBackground(horizontalToolbar_.isFillBackgroundChecked());
    return 0;
}

LRESULT ImageEditorWindow::OnInvertSelectionChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    canvas_->setInvertSelection(horizontalToolbar_.isInvertSelectionChecked());
    return 0;
}

LRESULT ImageEditorWindow::OnArrowTypeChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    canvas_->setArrowMode(static_cast<Arrow::ArrowMode>(horizontalToolbar_.getArrowType()));
    return 0;
}

void ImageEditorWindow::enableToolbarsIfNecessary(bool enable) {
    if (displayMode_ == wdmFullscreen) {
        horizontalToolbar_.EnableWindow(enable);
        verticalToolbar_.EnableWindow(enable);
    }
}

LRESULT ImageEditorWindow::OnEnable(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    enableToolbarsIfNecessary(static_cast<BOOL>(wParam));
    return 0;
}

void ImageEditorWindow::updateWindowTitle() {
    std::wstring windowTitle;
    if (sourceFileName_.IsEmpty()) {
        windowTitle = str(boost::wformat(TR("Image Editor  (%1%x%2%)")) % currentDoc_->getWidth() % currentDoc_->getHeight());
    }
    else {
        CString fileNameStr = WinUtils::TrimString(WinUtils::DoExtractFileName(sourceFileName_), 60);
        std::wstring fileNameWstring{ fileNameStr };

        windowTitle = str(boost::wformat(TR("Image Editor - %1% (%2%x%3%)")) % fileNameWstring % currentDoc_->getWidth() % currentDoc_->getHeight());
    }

	if (canvas_->isDocumentModified()) {
        windowTitle = L"* " + windowTitle;
	}
    if (windowTitle != windowTitle_) {
        windowTitle_ = windowTitle;
        SetWindowText(windowTitle.c_str());
    }
}

LRESULT ImageEditorWindow::OnApplyOperation(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    canvas_->applyCurrentOperation();
    showApplyButtons();
    return 0;
}

LRESULT ImageEditorWindow::OnCancelOperation(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    canvas_->cancelCurrentOperation();
    showApplyButtons();
    return 0;
}

void ImageEditorWindow::showApplyButtons() {
    horizontalToolbar_.showApplyButtons(currentDrawingTool_ == DrawingToolType::dtCrop && displayMode_ == wdmWindowed && canvas_->hasElementOfType(ElementType::etCrop));
}

void ImageEditorWindow::repositionToolbar(Toolbar& toolbar, const CRect& otherToolbarRect) {
    if (displayMode_ != wdmFullscreen) {
        return;
    }
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    ::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
    CRect workArea = mi.rcMonitor;

    MonitorEnumerator enumerator;
    CRect toolbarRc;
    toolbar.GetClientRect(toolbarRc);
    toolbar.ClientToScreen(toolbarRc);

    bool needRepos = false;

    if (enumerator.enumDisplayMonitors(nullptr, nullptr)) {
        needRepos = true;
        for (const auto& mon : enumerator) {
            CRect r = mon.rect;
            CRect intersected;
            intersected.IntersectRect(r, toolbarRc);
            if (intersected == toolbarRc) {
                needRepos = false;
                break;
            }
        }
    }

    if (needRepos) {
        CPoint newPos;
        if (toolbar.orientation() == Toolbar::orHorizontal) {
            newPos = CPoint(workArea.left + otherToolbarRect.Width() + kCanvasMargin, workArea.top);
        } else {
            newPos = CPoint(workArea.left, workArea.top + otherToolbarRect.Height() + kCanvasMargin);
        }
        toolbar.SetWindowPos(nullptr, newPos.x, newPos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void ImageEditorWindow::showMoreActionsDropdownMenu(Toolbar::Item* item) {
    CMenu rectangleMenu;
    RECT rc = item->rect;
    horizontalToolbar_.ClientToScreen(&rc);
    rectangleMenu.CreatePopupMenu();
    int i = 0;


    std::shared_ptr<Gdiplus::Bitmap> icon2 = loadMenuIcon(IDB_ICONFLIPVERTICAL, true);
    icon2->GetHBITMAP({}, &bmIconFlipVertical_.m_hBitmap);
    
    GuiTools::InsertMenu(rectangleMenu, i++, ID_ROTATECLOCKWISE, TR("Rotate clockwise (90°)"), bmIconRotateCW_);
    GuiTools::InsertMenu(rectangleMenu, i++, ID_ROTATECOUNTERCLOCKWISE, TR("Rotate counter clockwise (-90°)"), bmIconRotate_);
    GuiTools::InsertMenu(rectangleMenu, i++, ID_FLIPHORIZONTAL, TR("Flip horizontal"), bmIconFlipHorizontal_);
    GuiTools::InsertMenu(rectangleMenu, i++, ID_FLIPVERTICAL, TR("Flip vertical"), bmIconFlipVertical_);

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    rectangleMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, m_hWnd, &excludeArea);
}

void ImageEditorWindow::createIcons() {
    const int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    if (icon_) {
        icon_.DestroyIcon();
    }
    if (iconSmall_) {
        iconSmall_.DestroyIcon();
    }
    icon_ = GuiTools::LoadBigIcon(IDR_MAINFRAME, dpi);
    iconSmall_ = GuiTools::LoadSmallIcon(IDR_MAINFRAME, dpi);

    SetIcon(icon_, TRUE);
    SetIcon(iconSmall_, FALSE);

    if (bmIconRotateCW_) {
        bmIconRotateCW_.DeleteObject();
    }
    std::shared_ptr<Gdiplus::Bitmap> iconRotateCW = loadMenuIcon(IDB_ICONROTATECW, true);
    iconRotateCW->GetHBITMAP({}, &bmIconRotateCW_.m_hBitmap);

    if (bmIconRotate_) {
        bmIconRotate_.DeleteObject();
    }

    std::shared_ptr<Gdiplus::Bitmap> iconRotate = loadMenuIcon(IDB_ICONROTATE, true);
    iconRotate->GetHBITMAP({}, &bmIconRotate_.m_hBitmap);

    if (bmIconFlipHorizontal_) {
        bmIconFlipHorizontal_.DeleteObject();
    }

    std::shared_ptr<Gdiplus::Bitmap> icon = loadMenuIcon(IDB_ICONFLIPHORIZONTAL, true);
    icon->GetHBITMAP({}, &bmIconFlipHorizontal_.m_hBitmap);

    if (bmIconFlipVertical_) {
        bmIconFlipVertical_.DeleteObject();
    }
}

bool ImageEditorWindow::checkCloseWindowAfterAction() const {
    return openedAfterScreenshot() && configurationProvider_->getCloseWindowAfterActionInFullScreen() && sourceFileName_.IsEmpty();
}

bool ImageEditorWindow::canCloseAfterAction() const {
    return openedAfterScreenshot() && sourceFileName_.IsEmpty();
}

std::string ImageEditorWindow::getUploadButtonText() const {
    if (serverDisplayName_.empty()) {
        return _("Upload to Web");
    } 

    return str(IuStringUtils::FormatNoExcept(_("Upload to %s")) % serverDisplayName_);   
}

bool ImageEditorWindow::openedAfterScreenshot() const {
    return displayMode_ == wdmFullscreen;
}

CString ImageEditorWindow::makeFileName() const {
    const auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    const ScreenshotSettingsStruct& screenshotSettings = settings->ScreenshotSettings;
    CString suggestingFileName;

    if (sourceFileName_.IsEmpty()) {
        const SIZE bitmapSize = canvas_->getExportBitmapSize();
        suggestingFileName = IuCommonFunctions::MakeScreenshotFileName(screenshotData_, bitmapSize);
    } else {
        suggestingFileName = sourceFileName_;
    }

    CString result;
    int dotPos = suggestingFileName.Find(_T("."));
    if (dotPos != -1) {
        result = suggestingFileName.Left(dotPos);
    } else {
        result = suggestingFileName;
    }

    CString fileExt = suggestingFileName.Mid(dotPos + 1);
    fileExt.MakeLower();

    std::wstring_view imgTypes[] = {_T("jpg"), _T("png"), _T("gif"), _T("webp"), _T("webp")};
    int format = screenshotSettings.Format;
    if (format >= 0 && format < std::size(imgTypes)) {
        fileExt = imgTypes[format].data();
    }

    if (fileExt.IsEmpty()) {
        fileExt = _T("png");
    }

    return result + +_T(".") + fileExt;
}

LRESULT ImageEditorWindow::OnDeleteSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->deleteSelectedElements();
    return 0;
}

LRESULT ImageEditorWindow::OnRotateClockwise(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->rotate(Gdiplus::Rotate90FlipNone);
    return 0;
}

LRESULT ImageEditorWindow::OnRotateCounterClockwise(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->rotate(Gdiplus::Rotate270FlipNone);
    return 0;
}

LRESULT ImageEditorWindow::OnFlipVertical(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->rotate(Gdiplus::RotateNoneFlipY);
    return 0;
}

LRESULT ImageEditorWindow::OnFlipHorizontal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    canvas_->rotate(Gdiplus::RotateNoneFlipX);
    return 0;
}

LRESULT ImageEditorWindow::OnMoreActionsClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    int itemIndex = horizontalToolbar_.getItemIndexByCommand(ID_MOREACTIONS);
    if (itemIndex != -1) {
        showMoreActionsDropdownMenu(horizontalToolbar_.getItem(itemIndex));
    }
    return 0;
}

LRESULT ImageEditorWindow::OnRecordScreen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(drRecordScreen);
    return 0;
}

LRESULT ImageEditorWindow::OnClickedContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    // This ensures lastCrop is stored
    //resultingBitmap_ = canvas_->getBitmapForExport();

    EndDialog(drContinue);
    return 0;
}

LRESULT ImageEditorWindow::OnDrawBorderChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    canvas_->setDrawBorder(horizontalToolbar_.isDrawBorderChecked());
    return 0;
}

}
