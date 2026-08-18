@echo off
set zipcmd=7z
set "IU_APP_VER="
set "IU_BUILD_NUMBER="

For /F "tokens=2,3 delims= " %%i In (..\Source\VersionInfo.h) Do (set %%i=%%~j)

if "%IU_APP_VER%" == "" (
	echo Failed to obtain variables from VersionInfo.h
	goto End
) 

echo Creating distribution archive for Uptooda version %IU_APP_VER% %IU_BUILD_NUMBER%

set temp_dir=..\Build\portable\temp
set filename=uptooda-%IU_APP_VER%-build-%IU_BUILD_NUMBER%-portable.7z

if exist "%temp_dir%\*" (
	rmdir /q /s  %temp_dir%
	if ERRORLEVEL 1 (
		echo Failed to delete directory %temp_dir%
		goto End
	)
)

if exist "output\%filename%" (
	del /q output\%filename%
	if ERRORLEVEL 1 (
		echo Failed to delete file 'output\%filename%'
		goto End
	)
)
 
for %%x in (
    %temp_dir%
    %temp_dir%\Lang
    %temp_dir%\Data
    %temp_dir%\Modules
    %temp_dir%\Modules\MediaInfoLang
    %temp_dir%\Docs
    %temp_dir%\Data\Thumbnails\
    %temp_dir%\Data\Favicons
    %temp_dir%\Data\Scripts
    %temp_dir%\Data\Scripts\Lang
    %temp_dir%\Data\Scripts\Utils
    %temp_dir%\Data\Scripts\UploadFilters
    %temp_dir%\Data\Scripts\ImageSearch
    %temp_dir%\Data\Servers
    %temp_dir%\Data\Update
    %temp_dir%\Data\Utils
) do (
    echo Creating directory %%x
    mkdir %%x
    if ERRORLEVEL 1 goto CreateDirFailed
)

rem call signcode.bat
Copy "..\Build\bin\Release\qimageuploader.exe" %temp_dir%\uptooda.exe
if ERRORLEVEL 1 goto CopyFailed

Copy "..\Build\bin\Release\sendrpt.exe" %temp_dir%\
Copy "..\Build\bin\Release\dbghelp.dll" %temp_dir%\
Copy "..\Build\bin\Release\crashrpt.dll" %temp_dir%\

xcopy "..\Lang\locale" %temp_dir%\Lang\locale /i /e /y
if ERRORLEVEL 1 goto CopyFailed
pushd .
cd %temp_dir%\Lang\locale
del /S *.po
popd

if NOT EXIST  %temp_dir%\Lang\locale\ru\LC_MESSAGES\imageuploader.mo goto CopyFailed
Copy "..\Modules\MediaInfoLang\*.csv" %temp_dir%\Modules\MediaInfoLang\
if ERRORLEVEL 1 goto CopyFailed
xcopy "..\Docs" %temp_dir%\Docs\ /s /e /y /i
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\servers.xml" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\servers.xsd" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\templates.xml" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\template.txt" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\mime.cache" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\.env" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Favicons\*.ico" %temp_dir%\Data\Favicons\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Scripts\*.nut" %temp_dir%\Data\Scripts\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Scripts\Lang\*.json" %temp_dir%\Data\Scripts\Lang\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Scripts\Utils\*.nut" %temp_dir%\Data\Scripts\Utils\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Scripts\UploadFilters\*.nut" %temp_dir%\Data\Scripts\UploadFilters\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Scripts\ImageSearch\*.nut" %temp_dir%\Data\Scripts\ImageSearch\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Update\iu_core.xml" %temp_dir%\Data\Update\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Update\iu_serversinfo.xml" %temp_dir%\Data\Update\
if ERRORLEVEL 1 goto CopyFailed
rem Copy "..\Data\Update\iu_ffmpeg.xml" %temp_dir%\Data\Update\
rem if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Thumbnails\*.*" %temp_dir%\Data\Thumbnails\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Utils\*" %temp_dir%\Data\Utils\
if ERRORLEVEL 1 goto CopyFailed
rem Copy "..\Data\Servers\*.xml" %temp_dir%\Data\Servers\

Copy "..\Build\ShellExt\Release optimized\ExplorerIntegration.dll" %temp_dir%\
rem if ERRORLEVEL 1 goto CopyFailed
Copy "..\Build\ShellExt\Release optimized\ExplorerIntegration64.dll" %temp_dir%\
rem if ERRORLEVEL 1 goto CopyFailed
Copy "..\Build\Gui\Release\av*.dll" %temp_dir%\
rem if ERRORLEVEL 1 goto CopyFailed
Copy "..\Build\Gui\Release\sw*.dll" %temp_dir%\
rem if ERRORLEVEL 1 goto CopyFailed


rem Copy "Dll\gdiplus.dll" %temp_dir%\
Copy "curl-ca-bundle.crt" %temp_dir%\
if ERRORLEVEL 1 goto CopyFailed

if exist "%temp_dir%\Data\Scripts\test.nut" (
    del "%temp_dir%\Data\Scripts\test.nut"
)

if exist "%temp_dir%\Docs\api\.gitignore" (
    del "%temp_dir%\Docs\api\.gitignore"
)

if exist "%temp_dir%\Docs\en_US\.gitignore" (
    del "%temp_dir%\Docs\en_US\.gitignore"
)

rem signtool sign  /t http://timestamp.digicert.com /f "d:\Backups\ImageUploader\3315593d7023a0aeb48042349dc4fd40.pem" "%temp_dir%\Image Uploader.exe" "%temp_dir%\ExplorerIntegration.dll" "%temp_dir%\ExplorerIntegration64.dll"

cd %temp_dir%
%zipcmd% a -mx9 ..\..\output\uptooda-%IU_APP_VER%-build-%IU_BUILD_NUMBER%-openssl-portable.7z "*"
if ERRORLEVEL 1 goto ZipFailed
cd ..\..\

del "%temp_dir%\uptooda.exe"
if ERRORLEVEL 1 (
    echo Deleting %temp_dir%\uptooda.exe failed
    goto End
)
rem Copy "..\Build\release openssl\Image Uploader.exe" %temp_dir%\
rem if ERRORLEVEL 1 goto CopyFailed


rem cd %temp_dir%
rem %zipcmd% a -mx9 ..\..\output\image-uploader-%IU_APP_VER%-build-%IU_BUILD_NUMBER%-openssl-portable.7z "*"
rem if ERRORLEVEL 1 goto ZipFailed
rem cd ..\..\

rem rmdir /q /s  %temp_dir%
goto End

:CreateDirFailed
echo Directory creation failed.
exit 1

:CopyFailed
echo Copying files failed.
exit 2

:ZipFailed
echo Zip command failed.
exit 3

:End
echo Finished.