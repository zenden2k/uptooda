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

#ifndef IU_GUI_WIZARDCOMMON_H
#define IU_GUI_WIZARDCOMMON_H

#pragma once

#include "atlheaders.h"

constexpr int IU_IDC_CONST = 12255;
constexpr int IDC_SETTINGS = IU_IDC_CONST + 1;
constexpr int IDC_REGIONPRINT = IU_IDC_CONST + 2;
constexpr int IDC_MEDIAFILEINFO = IU_IDC_CONST + 3;
constexpr int IDC_CLIPBOARD = IU_IDC_CONST + 4;
constexpr int IDC_ADDFOLDER = IU_IDC_CONST + 5;
constexpr int IDC_ADDFILES = IU_IDC_CONST + 6;
constexpr int IDC_DOWNLOADIMAGES = IU_IDC_CONST + 7;
constexpr int IDC_REUPLOADIMAGES = IU_IDC_CONST + 8;
constexpr int IDC_SHORTENURL = IU_IDC_CONST + 9;
constexpr int IDC_LASTREGIONSCREENSHOT = IU_IDC_CONST + 10;
constexpr int IDC_RECORDSCREENDLG = IU_IDC_CONST + 11;
constexpr int IDC_RECORDSCREEN = IU_IDC_CONST + 12;

enum ScreenshotInitiator {
    siDefault = 0,
    siFromHotkey = 1,
    siFromTray = 2,
    siFromWelcomeDialog = 3
};

class CWizardDlg;
class CWizardPage
{
    public:
        CWizardDlg* WizardDlg;
        virtual ~CWizardPage() = default;
        CBitmap HeadBitmap;
        virtual bool OnShow();
        virtual bool OnHide();
        virtual bool OnNext();
        void EnableNext(bool Enable = true);
        void EnablePrev(bool Enable = true);
        void EnableExit(bool Enable = true);
        void SetNextCaption(LPCTSTR Caption);
        HWND PageWnd;
        void ShowNext(bool Show = true);
        void ShowPrev(bool Show = true);
        virtual void SetInitialFocus();
};

#endif // IU_GUI_WIZARDCOMMON_H
