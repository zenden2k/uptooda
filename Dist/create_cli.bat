@echo off
SET CLI_VERSION=1.4.0
set zipcmd=7z

For /F "tokens=2,3 delims= " %%i In (..\Source\VersionInfo.h) Do (set %%i=%%~j)
echo Creating distribution archive for Uptooda CLI version %IU_APP_VER% %IU_BUILD_NUMBER%

set temp_dir=..\Build\portable\temp_%BUILD%_cli_%CLI_VERSION%
set filename=uptooda-cli-%IU_APP_VER%-build-%IU_BUILD_NUMBER%.7z
echo %temp_dir%

rmdir /q /s  %temp_dir%
del /q output\%filename%
 
mkdir %temp_dir%
mkdir %temp_dir%\Data
mkdir %temp_dir%\Data\Scripts
mkdir %temp_dir%\Data\Update

rem signtool sign /t http://time.certum.pl /f d:\Backups\ImageUploader\zenden2k.pem  "..\Build\CLI\win32\release\executable\uptooda.exe"  
Copy "..\Build\CLI\Release\uptooda-cli.exe" %temp_dir%\uptooda-cli.exe
if ERRORLEVEL 1 goto CopyFailed

Copy "..\Build\CLI\Release\WebView2Loader.dll" %temp_dir%\
if ERRORLEVEL 1 goto CopyFailed

Copy "curl-ca-bundle.crt" %temp_dir%\curl-ca-bundle.crt
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\servers.xml" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\mime.cache" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\.env" %temp_dir%\Data\
if ERRORLEVEL 1 goto CopyFailed
Copy "..\Data\Scripts\*.nut" %temp_dir%\Data\Scripts\
Copy "..\Data\Update\iu_servers*.xml" %temp_dir%\Data\Update\


cd %temp_dir%
%zipcmd% a -mx9 ..\..\output\%filename% "*"
cd ..\..\

rmdir /q /s  %temp_dir%

goto End

:CopyFailed
echo Copying files failed.
exit 2

:End
echo Finished.