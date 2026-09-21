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

#include "ImageEditorSettingsPage.h"

#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/GuiTools.h"

LRESULT CImageEditorSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    TRC(IDC_ALLOWALTTABINIMAGEEDITOR, "Allow Alt+Tab in fullscreen editor");
    TRC(IDC_ALLOWFULLSCREENEDITORCHECK, "Allow opening existing images in fullscreen mode");
    TRC(IDC_CLOSEIMAGEEDITORONCOPY, "Close window after actions when taking screenshot");

    GuiTools::SetCheck(m_hWnd, IDC_ALLOWALTTABINIMAGEEDITOR, settings->ImageEditorSettings.AllowAltTab);
    GuiTools::SetCheck(m_hWnd, IDC_ALLOWFULLSCREENEDITORCHECK, settings->ImageEditorSettings.AllowEditingInFullscreen);
    GuiTools::SetCheck(m_hWnd, IDC_CLOSEIMAGEEDITORONCOPY, settings->ImageEditorSettings.CloseWindowAfterActionInFullScreen);

    return 1; // Let the system set the focus
}

bool CImageEditorSettingsPage::apply() {
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    settings->ImageEditorSettings.AllowAltTab = GuiTools::GetCheck(m_hWnd, IDC_ALLOWALTTABINIMAGEEDITOR);
    settings->ImageEditorSettings.AllowEditingInFullscreen = GuiTools::GetCheck(m_hWnd, IDC_ALLOWFULLSCREENEDITORCHECK);
    settings->ImageEditorSettings.CloseWindowAfterActionInFullScreen = GuiTools::GetCheck(m_hWnd, IDC_CLOSEIMAGEEDITORONCOPY);

    return true;
}

