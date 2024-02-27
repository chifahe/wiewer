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

#define VIDEO_MP4 1
#define VIDEO_OGG 2
#define VIDEO_WEBM 3

#define AUDIO_MP3 11
#define AUDIO_AAC 12
#define AUDIO_OGG 13
#define AUDIO_WAV 14

#define IMAGE_JPG 21
#define IMAGE_PNG 22
#define IMAGE_WEBP 23
#define IMAGE_GIF 24
#define IMAGE_ICO 25

using namespace Microsoft::WRL;

// Global variables

// The name of the application
static TCHAR szWindowClass[] = _T("wiewer");

// The default text that appears in the title bar.
static TCHAR szTitle_default[] = _T("wiewer");

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
	wcex.lpszClassName = szWindowClass;
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

	TCHAR* szTitle;									// The text that appears in the title bar.
	TCHAR file_path_url[268] = TEXT("file:///");	// File path used in WebView2.

	// Initial size of window
	unsigned short int window_width;
	unsigned short int window_height;

	// TODO: Get type of file by file header.
	unsigned short int file_type_num = 0;

	bool is_lpCmdLine_available = (wcslen(lpCmdLine) != 0 && _waccess_s(lpCmdLine, 4) != -1 && wcsrchr(lpCmdLine, L'\\') != NULL) ? true : false;	// Check if lpCmdLine is an available file path.
	
	if (is_lpCmdLine_available) {
		szTitle = wcsrchr(lpCmdLine, L'\\') + 1;	// Set title as file name.

		// XXX: Using '\\' in url seems to be OK.
		wcscat_s(file_path_url, lpCmdLine);			// The format of file_path_url is "file:///C:\\xxx.jpg".

		// TODO: Make window's initial size fit the file.

		// NOTE: window_height includes height of title bar.
		/*
		* Standard title bar height is 32px.
		* It may be affected by system scaling.
		* My scaling is 150%, so my height is 21.3px.
		*/
		window_width = 300;
		window_height = 300 + 21.3;
	}
	else
	{
		szTitle = szTitle_default;
		file_path_url[0] = L'\0';	// Clear array.

		// default size
		window_width = 400;
		window_height = 300;
	}

	// The parameters to CreateWindow explained:
	// szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// window_width, window_height: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		window_width, window_height,
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
			[file_path_url, hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				//MessageBoxA(hWnd, "createView", "", NULL);
				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[file_path_url, hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
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
						if (file_path_url[0] == L'\0') {
							res = webviewWindow->NavigateToString(L"<html><head><style>*{margin:0;padding:0;text-align:center;user-select:none;}body{position:absolute;height:100%;width:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;overflow:hidden;}</style></head><body><h1>Welcome to wiewer!</h1><br><font size='4'>wiewer is a simple media viewer based on WebView2.</font></body></html>");	// default page
						}
						else
						{
							res = webviewWindow->Navigate(file_path_url);
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
