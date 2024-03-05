// XXX: This file is based on Microsoft Webview2 Demo, codes and comments need to be standardized later.

#include <windows.h>
#include <strsafe.h>
#include <io.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>
#include <AccCtrl.h>
#include <AclAPI.h>

#include "framework.h"
#include "wiewer.h"
#include "webView2.h"

#define NON_CLIENT_WIDTH SM_CXBORDER*2
#define NON_CLIENT_HEIGHT SM_CYCAPTION+SM_CYMENU+SM_CYBORDER*2

using namespace Microsoft::WRL;

// Global variables

static TCHAR sg_szWindowClass[] = _T("wiewer");		// The name of the application

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webviewWindow;

int CALLBACK wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = sg_szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	// Store instance handle in our global variable
	hInst = hInstance;

	TCHAR* szTitle = NULL;					// The text that appears in the title bar.
	TCHAR filePath[268] = TEXT("file:///");	// File path used in WebView2.

	// 400 * 300 is default size.
	unsigned short int windowWidth = 400 + NON_CLIENT_WIDTH;	// Initial width of window.
	unsigned short int windowHeight = 300 + NON_CLIENT_HEIGHT;	// Initial height of window.
	// XXX: I'm not sure the size of non-client area.

	bool lpCmdLineAvailable = true;	// Check if lpCmdLine is an available file path.

	/*
	* If lpCmdLine:
	* Not empty,
	* Accessible,
	* '\\' and '.' included,
	*/
	if (wcslen(lpCmdLine) != 0 && _waccess_s(lpCmdLine, 4) != -1 && wcsrchr(lpCmdLine, L'\\') != NULL && wcsrchr(lpCmdLine, L'.') != NULL) {
		TCHAR fileType[255];																// File extension.
		wcscpy_s(fileType, 255, wcsrchr(lpCmdLine, L'.') + 1);
		for (unsigned char i = 0; i < 255; i++) if (fileType[i] > L'Z') fileType[i] -= 32;	// Capitalize fileType.
		szTitle = wcsrchr(lpCmdLine, L'\\') + 1;;											// Set title as file name.

		// XXX: Using '\\' in url seems to be OK.
		wcscat_s(filePath, lpCmdLine);														// The format of filePath is "file:///C:\\xxx.jpg".

		// Check file type.

		// Video
		if (wcscmp(fileType, L"MP4") == 0) {}
		else if (wcscmp(fileType, L"OGG") == 0) {}
		else if (wcscmp(fileType, L"WEBM") == 0) {}

		// Audio
		else if (wcscmp(fileType, L"MP3") == 0 || wcscmp(fileType, L"AAC") == 0 || wcscmp(fileType, L"WAV") == 0) {
			windowWidth = 400 + NON_CLIENT_WIDTH;
			windowHeight = 400 + NON_CLIENT_HEIGHT;
		}

		// Image
		else if (wcscmp(fileType, L"JPG") == 0 || wcscmp(fileType, L"JPEG") == 0) {
			windowWidth = 300 + NON_CLIENT_WIDTH;
			windowHeight = 300 + NON_CLIENT_HEIGHT;
		}
		else if (wcscmp(fileType, L"PNG") == 0) {}
		else if (wcscmp(fileType, L"WEBP") == 0) {}
		else if (wcscmp(fileType, L"GIF") == 0) {}
		else if (wcscmp(fileType, L"ICO") == 0) {}

		else
		{
			lpCmdLineAvailable = false;
		}
	}
	else
	{
		lpCmdLineAvailable = false;
	}

	// If lpCmdLine is not a supported file, set wiewer as default.
	if (!lpCmdLineAvailable)
	{
		szTitle = (LPWSTR)TEXT("wiewer");	// Default title.
		*filePath = NULL;					// Empty array.
	}

	// The parameters to CreateWindow explained:
	// sg_szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// windowWidth, windowHeight: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
	HWND hWnd = CreateWindow(
		sg_szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	// The parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow: the fourth parameter from WinMain
	ShowWindow(hWnd,
		nCmdShow);
	UpdateWindow(hWnd);

	// <-- WebView2 sample code starts here -->

	HRESULT res = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[filePath, hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				//MessageBoxA(hWnd, "createView", "", NULL);
				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[filePath, hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewController = controller;
							webviewController->get_CoreWebView2(&webviewWindow);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						ICoreWebView2Settings* Settings;
						webviewWindow->get_Settings(&Settings);
						Settings->put_IsScriptEnabled(TRUE);
						Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						Settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(hWnd, &bounds);
						webviewController->put_Bounds(bounds);

						// Schedule an async task to navigate
						HRESULT res;
						if (*filePath == NULL) {
							res = webviewWindow->NavigateToString(L"<html><head><style>*{margin:0;padding:0;text-align:center;user-select:none;}body{position:absolute;height:100%;width:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;overflow:hidden;}</style></head><body><h1>Welcome to wiewer!</h1><br><font size='4'>wiewer is a simple media viewer based on WebView2.</font></body></html>");	// default page
						}
						else
						{
							res = webviewWindow->Navigate(filePath);
						}
						std::string sres = std::to_string(res).c_str();

						// Step 4 - Navigation events

						// Step 5 - Scripting

						// Step 6 - Communication between host and web content

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
	// <-- WebView2 sample code ends here -->

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			webviewController->put_Bounds(bounds);
		};
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}
