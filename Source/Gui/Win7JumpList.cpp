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

#include "Win7JumpList.h"

#include <ShObjIdl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <ComDef.h>

#include "resource.h"
#include "Core/i18n/Translator.h"
#include "Func/WinUtils.h"

/*
PCWSTR const c_rgpszFiles[] =
{
	L"Microsoft_Sample_1.txt",
	L"Microsoft_Sample_2.txt",
	L"Microsoft_Sample_3.doc",
	L"Microsoft_Sample_4.doc"
};
*/

namespace
{
// Determines if the provided IShellItem is listed in the array of items that the user has removed
bool IsItemInArray(IShellItem *psi, IObjectArray *poaRemoved)
{
	bool fRet = false;
	UINT cItems;
	if (SUCCEEDED(poaRemoved->GetCount(&cItems))) {
		IShellItem *psiCompare;
		for (UINT i = 0; !fRet && i < cItems; i++) {
			if (SUCCEEDED(poaRemoved->GetAt(i, IID_PPV_ARGS(&psiCompare)))) {
				int iOrder;
				fRet = SUCCEEDED(psiCompare->Compare(psi, SICHINT_CANONICAL, &iOrder)) && (0 == iOrder);
				psiCompare->Release();
			}
		}
	}
	return fRet;
}

}

Win7JumpList::Win7JumpList() {
	CreateJumpList();
}

Win7JumpList::~Win7JumpList() {
	//DeleteJumpList();
}

// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
HRESULT Win7JumpList::CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, int iconId, IShellLink **ppsl)
{
	IShellLink *psl;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
	if (SUCCEEDED(hr)) {
		// Determine our executable's file path so the task will execute this application
		WCHAR szAppPath[MAX_PATH];
		if (GetModuleFileName(NULL, szAppPath, ARRAYSIZE(szAppPath))) {
			hr = psl->SetPath(szAppPath);
			if (SUCCEEDED(hr)) {
				hr = psl->SetArguments(pszArguments);
				if (SUCCEEDED(hr)) {
					if (iconId) {
						hr = psl->SetIconLocation(szAppPath, -iconId);
						if (!SUCCEEDED(hr)) {
							LOG(WARNING) << "Failed to set item icon, hr=" << hr;
						}
					}
                    hr = psl->SetWorkingDirectory(WinUtils::GetAppFolder());
                    if (FAILED(hr)) {
                        _com_error err(hr);
                        LOG(WARNING) << "Failed to set working directory, hr=" << hr << std::endl << err.ErrorMessage();
                    }
					// The title property is required on Jump List items provided as an IShellLink
					// instance.  This value is used as the display name in the Jump List.
					IPropertyStore *pps;
					hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
					if (SUCCEEDED(hr)) {
						PROPVARIANT propvar;
						hr = InitPropVariantFromString(pszTitle, &propvar);
						if (SUCCEEDED(hr)) {
							hr = pps->SetValue(PKEY_Title, propvar);
							if (SUCCEEDED(hr)) {
								hr = pps->Commit();
								if (SUCCEEDED(hr)) {
									hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
								}
							}
							PropVariantClear(&propvar);
						}
						pps->Release();
					}
				}
			}
		} else {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		psl->Release();
	}
	return hr;
}


// Adds a custom category to the Jump List.  Each item that should be in the category is added to
// an ordered collection, and then the category is appended to the Jump List as a whole.
HRESULT Win7JumpList::AddCategoryToList(ICustomDestinationList *pcdl, IObjectArray *poaRemoved)
{
	IObjectCollection *poc;
	HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
	if (SUCCEEDED(hr)) {
		/*for (UINT i = 0; i < ARRAYSIZE(c_rgpszFiles); i++) {
			IShellItem *psi;
			if (SUCCEEDED(SHCreateItemInKnownFolder(FOLDERID_Documents, KF_FLAG_DEFAULT, c_rgpszFiles[i], IID_PPV_ARGS(&psi)))) {
				// Items listed in the removed list may not be re-added to the Jump List during this
				// list-building transaction.  They should not be re-added to the Jump List until
				// the user has used the item again.  The AppendCategory call below will fail if
				// an attempt to add an item in the removed list is made.
				if (!_IsItemInArray(psi, poaRemoved)) {
					poc->AddObject(psi);
				}
				psi->Release();
			}
		}*/

		IObjectArray *poa;
		hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
		if (SUCCEEDED(hr)) {
			// Add the category to the Jump List.  If there were more categories, they would appear
			// from top to bottom in the order they were appended.
			hr = pcdl->AppendCategory(L"Screenshot", poa); // fails with E_INVALIDARG, need to register file type handler
			poa->Release();

		}
		poc->Release();
	}
	return hr;
}


// The Tasks category of Jump Lists supports separator items.  These are simply IShellLink instances
// that have the PKEY_AppUserModel_IsDestListSeparator property set to TRUE.  All other values are
// ignored when this property is set.
HRESULT Win7JumpList::CreateSeparatorLink(IShellLink **ppsl)
{
	IPropertyStore *pps;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pps));
	if (SUCCEEDED(hr)) {
		PROPVARIANT propvar;
		hr = InitPropVariantFromBoolean(TRUE, &propvar);
		if (SUCCEEDED(hr)) {
			hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
			if (SUCCEEDED(hr)) {
				hr = pps->Commit();
				if (SUCCEEDED(hr)) {
					hr = pps->QueryInterface(IID_PPV_ARGS(ppsl));
				}
			}
			PropVariantClear(&propvar);
		}
		pps->Release();
	}
	return hr;
}

// Builds the collection of task items and adds them to the Task section of the Jump List.  All tasks
// should be added to the canonical "Tasks" category by calling ICustomDestinationList::AddUserTasks.
HRESULT Win7JumpList::AddTasksToList(ICustomDestinationList *pcdl)
{
	IObjectCollection *poc;
	HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
	if (SUCCEEDED(hr)) {
		IShellLink * psl = nullptr;
		hr = CreateShellLink(L"/func=addimages", TR("Upload Images"), IDI_ICONADD, &psl);
		if (SUCCEEDED(hr)) {
			hr = poc->AddObject(psl);
			psl->Release();
		}

        if (SUCCEEDED(hr)) {
            hr = CreateShellLink(L"/func=fromclipboard", TR("From Clipboard"), IDI_CLIPBOARD, &psl);
            if (SUCCEEDED(hr)) {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }

        if (SUCCEEDED(hr)) {
            hr = CreateShellLink(L"/func=reuploadimages", TR("Reupload"), IDI_ICONRELOAD, &psl);
            if (SUCCEEDED(hr)) {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }

		if (SUCCEEDED(hr)) {
            hr = CreateSeparatorLink(&psl);
            if (SUCCEEDED(hr)) {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }

		if (SUCCEEDED(hr)) {
			hr = CreateShellLink(L"/func=screenshotdlg", TR("Screenshot..."), IDI_SCREENSHOT, &psl);
			if (SUCCEEDED(hr)) {
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

		if (SUCCEEDED(hr)) {
			hr = CreateShellLink(L"/quickshot", TR("Shot of Selected Region..."), IDI_ICONREGION, &psl);
			if (SUCCEEDED(hr)) {
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

		if (SUCCEEDED(hr)) {
			hr = CreateShellLink(L"/func=fullscreenshot", TR("Capture the Entire Screen"), IDI_SCREENSHOT, &psl);
			if (SUCCEEDED(hr)) {
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

		if (SUCCEEDED(hr)) {
			hr = CreateShellLink(L"/func=windowscreenshot", TR("Capture the Active Window"), IDI_WINDOW, &psl);
			if (SUCCEEDED(hr)) {
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

		if (SUCCEEDED(hr)) {
			hr = CreateSeparatorLink(&psl);
			if (SUCCEEDED(hr)) {
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

        if (SUCCEEDED(hr)) {
            hr = CreateShellLink(L"/func=screenrecordingdlg", TR("Screen Recording..."), IDI_ICONRECORD, &psl);
            if (SUCCEEDED(hr)) {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }

        if (SUCCEEDED(hr)) {
            hr = CreateShellLink(L"/func=screenrecording", TR("Start Recording"), IDI_ICONSTARTRECORD, &psl);
            if (SUCCEEDED(hr)) {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }

		if (SUCCEEDED(hr)) {
            hr = CreateSeparatorLink(&psl);
            if (SUCCEEDED(hr)) {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }

		if (SUCCEEDED(hr)) {
			hr = CreateShellLink(L"/func=importvideo", TR("Import Video File"), IDI_GRAB, &psl);
			if (SUCCEEDED(hr)) {
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

		if (SUCCEEDED(hr)) {
			IObjectArray * poa;
			hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
			if (SUCCEEDED(hr)) {
				// Add the tasks to the Jump List. Tasks always appear in the canonical "Tasks"
				// category that is displayed at the bottom of the Jump List, after all other
				// categories.
				hr = pcdl->AddUserTasks(poa);
				poa->Release();
			}
		}
		poc->Release();
	}
	return hr;
}

// Builds a new custom Jump List for this application.
void Win7JumpList::CreateJumpList()
{
	// Create the custom Jump List object.
	ICustomDestinationList *pcdl;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
	if (SUCCEEDED(hr)) {
		// Custom Jump Lists follow a push model - applications are responsible for providing an updated
		// list anytime the contents should be changed.  Lists are generated in a list-building
		// transaction that starts by calling BeginList.  Until the list is committed, Windows will
		// display the previous version of the list, if available.
		//
		// The cMinSlots out parameter indicates the minimum number of items that the Jump List UI is
		// guaranteed to display.  Applications can provide more items when building a custom Jump List,
		// but the extra items may not be displayed.  The number is dependant upon a number of factors,
		// such as screen resolution and the "Number of recent items to display in Jump Lists" user setting.
		// See the MSDN documentation on BeginList for more information.
		//
		// The IObjectArray returned from BeginList contains a list of items the user has chosen to remove
		// from their Jump List.  Applications must respect the user's removal of an item and not re-add any
		// item in the removed list during this list-building transaction.  Applications should also clear any
		// persited usage-tracking data for any item in the removed list.  If the user begins using a
		// previously removed item in the future, it may be re-added to the list.
		UINT cMinSlots;
		IObjectArray *poaRemoved;
		hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
		if (SUCCEEDED(hr)) {
			// Add content to the Jump List.
			//hr = AddCategoryToList(pcdl, poaRemoved);
			/*if (SUCCEEDED(hr))*/ {
				hr = AddTasksToList(pcdl);
				if (SUCCEEDED(hr)) {
					// Commit the list-building transaction.
					hr = pcdl->CommitList();
				}
			}
			poaRemoved->Release();
		}
		pcdl->Release();
	}
}

// Removes that existing custom Jump List for this application.
void Win7JumpList::DeleteJumpList()
{
	ICustomDestinationList *pcdl;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
	if (SUCCEEDED(hr)) {
		hr = pcdl->DeleteList(NULL);
		pcdl->Release();
	}
}
