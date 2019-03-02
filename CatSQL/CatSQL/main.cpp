/************************************************************************************
*																					*
*		CatSQL by Jacob Brown (3/1/2019) - Written in C/C++							*
*																					*
*		This program was written as an exercise in communicating with an SQL		*
*		server using WinAPI.														*
*																					*
*		main.cpp - Handles all the window creation and interactions. Submits		*
*		a query string to ServerHandler and displays the results in the				*
*		listview at the top of the screen and the control in the bottom-right.		*
*																					*
************************************************************************************/
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <atlstr.h>
#include <CommCtrl.h>

#include "ServerHandler.h"
#include "ConnectionInfo.h"
#include "QueryResults.h"
#include "resource.h"

//#include "vld.h"

// ** GLOBAL VARIABLES **
// **********************

// Main window class name.
const TCHAR szWindowClass[] = TEXT("WINDOWCLASS");

// Background color
const COLORREF BG_COLOR = RGB(180, 180, 180);

// Global instance handle
HINSTANCE hInst;

// Used to access and retrieve information from a server
ServerHandler * serverHandler;

// ** FUNCTION PROTOTYPES **
// *************************
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL	CALLBACK ConnectProc(HWND, UINT, WPARAM, LPARAM);
BOOL	CALLBACK QueryProc(HWND, UINT, WPARAM, LPARAM);
BOOL	CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);
BOOL	StartServer(HWND, ConnectionInfo *, BOOL);
void	StopServer(HWND);
BOOL	CloseServer();
BOOL	ValidInt(TCHAR *);
void	GetLineFromBox(HWND, TCHAR *, UINT);
void	AssembleQuery(HWND, TCHAR *);
void	ResetListView(HWND);
void	CreateListView(HWND, QueryResults *);
void	EnableConnectionUIElements(HWND, BOOL);
BOOL	DrawLabel(HDC, HWND, TCHAR *);
void	DrawLine(HDC, COLORREF, int, int, int, int);

// WinMain - Creates our main window and enters the message loop
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(BG_COLOR);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));

	if (!RegisterClassEx(&wcex)) {
		MessageBox(NULL, TEXT("Call to RegisterClassEx failed - Exiting now."), TEXT("Initialization Error"), NULL);
		return 0;
	}

	// Store instance handle in our global variable
	hInst = hInstance;

	// Creates the main window
	HWND hWnd = CreateWindow(szWindowClass, TEXT("CatSQL - Windows SQL Client"), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, NULL, NULL, hInst, NULL);

	if (!hWnd) {
		MessageBox(NULL, TEXT("Call to CreateWindow failed - Exiting now."), TEXT("Initialization Error"), NULL);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

// WndProc - Main callback function that handles all of the immediate window stuff
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
		case WM_CREATE:
			{
				HICON hMyIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));

				HWND listView = CreateWindow(WC_LISTVIEW, TEXT("List-View"), WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
					0, 0, 0, 0, hWnd, (HMENU)IDQ_LV_RESULTS, hInst, NULL);

				HWND wndQuery = CreateWindow(TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
					0, 0, 0, 0, hWnd, (HMENU)IDQ_WIN_QUERY, hInst, NULL);

				HWND btnSubmit = CreateWindow(TEXT("BUTTON"), TEXT("Submit"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
					0, 0, 0, 0, hWnd, (HMENU)IDQ_BTN_SUBMIT, hInst, NULL);

				HWND btnClear = CreateWindow(TEXT("BUTTON"), TEXT("Clear"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
					0, 0, 0, 0, hWnd, (HMENU)IDQ_BTN_CLEAR, hInst, NULL);

				HWND wndResults = CreateWindow(TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
					0, 0, 0, 0, hWnd, (HMENU)IDQ_WIN_RESULTS, hInst, NULL);

				HWND hStatus = CreateWindow(STATUSCLASSNAME, TEXT("Status"), WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 
					0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS, hInst, NULL);

				// Sets the font of the results window
				SendMessage(GetDlgItem(hWnd, IDQ_WIN_RESULTS), WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), TRUE);

				// Sets text of status bar
				SetWindowText(GetDlgItem(hWnd, IDC_STATUS), TEXT("Disconnected"));

				// Disables all the connection menu elements at first
				EnableConnectionUIElements(hWnd, FALSE);

				// Sets keyboard focus to the query editor
				SetFocus(GetDlgItem(hWnd, IDQ_WIN_QUERY));
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {

				// ** Menu options **
				// ******************

				case IDM_FILE_CONNECT:
					DialogBox(hInst, MAKEINTRESOURCE(IDR_CONNECT), hWnd, ConnectProc);
					SetFocus(GetDlgItem(hWnd, IDQ_WIN_QUERY));
					break;

				case IDM_FILE_DISCONNECT:
					StopServer(hWnd);
					EnableConnectionUIElements(hWnd, FALSE);
					SetWindowText(GetDlgItem(hWnd, IDC_STATUS), TEXT("Disconnected"));
					break;

				case IDM_FILE_QUIT:
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;

				case IDM_HELP_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDR_ABOUT), hWnd, AboutProc);
					SetFocus(GetDlgItem(hWnd, IDQ_WIN_QUERY));
					break;

				// ** Query controls **
				// ********************

				case IDQ_BTN_SUBMIT:
					{
						if (serverHandler != NULL) {

							// Takes the query and turns it into one-line string
							TCHAR query[QUERY_MAX];
							AssembleQuery(hWnd, query);

							QueryResults * queryResults = new QueryResults();

							// If the query's valid, we display the results
							if (serverHandler->SubmitQuery(query, queryResults, hWnd)) {
								ResetListView(hWnd);
								CreateListView(hWnd, queryResults);
								SetWindowText(GetDlgItem(hWnd, IDQ_WIN_RESULTS), queryResults->message);
							} else {
								MessageBox(hWnd, TEXT("Error with query syntax."), TEXT("Query Submission Error"), MB_ICONERROR);
							}					

							delete queryResults;
						}
					}
					break;

				case IDQ_BTN_CLEAR:
					SetWindowText(GetDlgItem(hWnd, IDQ_WIN_QUERY), TEXT(""));
					break;
			}

			break;

		case WM_PAINT:
			{
				PAINTSTRUCT paintStruct;
				HDC hdc;
				hdc = BeginPaint(hWnd, &paintStruct);

				// Separator between top and bottom anchored to the results listview
				RECT hWndRect;
				GetWindowRect(GetDlgItem(hWnd, IDQ_LV_RESULTS), &hWndRect);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&hWndRect, 2);
				DrawLine(hdc, RGB(200, 200, 200), hWndRect.left - 12, hWndRect.bottom + 16, hWndRect.right + 12, hWndRect.bottom + 16);

				// Draws labels above each text edit control
				DrawLabel(hdc, GetDlgItem(hWnd, IDQ_LV_RESULTS), TEXT("Query Results"));
				DrawLabel(hdc, GetDlgItem(hWnd, IDQ_WIN_QUERY), TEXT("Query Editor"));
				DrawLabel(hdc, GetDlgItem(hWnd, IDQ_WIN_RESULTS), TEXT("Results"));

				EndPaint(hWnd, &paintStruct);
			}
			break;

		case WM_SIZE:
			{
				RECT rect;

				// Sets the position of each window/button
				if (GetWindowRect(hWnd, &rect)) {

					int width = rect.right - rect.left;
					int height = rect.bottom - rect.top;

					int listviewX = 12;
					int listviewY = 30;
					int listviewW = width - 40;
					int listviewH = height - 300;
					SetWindowPos(GetDlgItem(hWnd, IDQ_LV_RESULTS), HWND_BOTTOM, listviewX, listviewY, listviewW, listviewH, NULL);

					int queryX = 12;
					int queryY = height - 220;
					int queryW = width - 302;
					int queryH = 124;
					SetWindowPos(GetDlgItem(hWnd, IDQ_WIN_QUERY), HWND_BOTTOM, queryX, queryY, queryW, queryH, NULL);

					int submitX = width - 276;
					int submitY = height - 220;
					int submitW = 84;
					int submitH = 40;
					SetWindowPos(GetDlgItem(hWnd, IDQ_BTN_SUBMIT), HWND_BOTTOM, submitX, submitY, submitW, submitH, NULL);

					int clearX = width - 276;
					int clearY = height - 176;
					int clearW = 84;
					int clearH = 40;
					SetWindowPos(GetDlgItem(hWnd, IDQ_BTN_CLEAR), HWND_BOTTOM, clearX, clearY, clearW, clearH, NULL);

					int resultsX = width - 180;
					int resultsY = height - 220;
					int resultsW = 150;
					int resultsH = 84;
					SetWindowPos(GetDlgItem(hWnd, IDQ_WIN_RESULTS), HWND_BOTTOM, resultsX, resultsY, resultsW, resultsH, NULL);
				}

				// This is needed to keep the status bar anchored to the bottom
				SendMessage(GetDlgItem(hWnd, IDC_STATUS), WM_SIZE, 0, 0);
			}
			break;

		case WM_GETMINMAXINFO:
			{
				// Defines the minimum size of the window
				LPMINMAXINFO lpMinMax = (LPMINMAXINFO)lParam;
				lpMinMax->ptMinTrackSize.x = 450;
				lpMinMax->ptMinTrackSize.y = 450;
			}
			break;

		case WM_CLOSE:
			if (MessageBox(hWnd, TEXT("Do you really want to quit?"), TEXT("Exit CatSQL"), MB_YESNO | MB_ICONWARNING) == IDYES) {
				CloseServer();
				DestroyWindow(hWnd);
				return 0;
			}
			return 1;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// ConnectProc - Callback function that handles all of the connect dialog functionality
BOOL CALLBACK ConnectProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
		case WM_INITDIALOG: 
			EnableWindow(GetDlgItem(hWnd, IDD_BOX_USERNAME), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDD_BOX_PASSWORD), FALSE);
			SetFocus(GetDlgItem(hWnd, IDD_BOX_SERVER));

			// This must return false to override 
			// focus to the server edit control
			return FALSE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDD_BTN_CONNECT: 
					{
						// ConnectionInfo gathers all info needed to construct
						// a connection string in serverHandler
						ConnectionInfo * connectionInfo = new ConnectionInfo();
						GetLineFromBox(hWnd, connectionInfo->serverName, IDD_BOX_SERVER);
						GetLineFromBox(hWnd, connectionInfo->portNumber, IDD_BOX_PORTNO);
						GetLineFromBox(hWnd, connectionInfo->database, IDD_BOX_DATABASE);
						GetLineFromBox(hWnd, connectionInfo->username, IDD_BOX_USERNAME);
						GetLineFromBox(hWnd, connectionInfo->password, IDD_BOX_PASSWORD);

						// If port number entered is a valid int, we try to connect
						if (ValidInt(connectionInfo->portNumber)) {

							BOOL userAuth = FALSE;
							if (IsDlgButtonChecked(hWnd, IDD_CHECK_USER))
								userAuth = TRUE;

							if (StartServer(hWnd, connectionInfo, userAuth)) {
								EndDialog(hWnd, IDD_BTN_CONNECT);
								EnableConnectionUIElements(GetParent(hWnd), TRUE);

								// Sends message to status bar saying we're connected
								TCHAR statusStr[STR_MAX];
								swprintf(statusStr, STR_MAX, TEXT("%ls%ls%ls%ls%ls%ls"), TEXT("Connected: "), connectionInfo->serverName, 
									TEXT(", "), connectionInfo->portNumber, TEXT(" - Database: "), connectionInfo->database);
								SetWindowText(GetDlgItem(GetParent(hWnd), IDC_STATUS), statusStr);
							}
						} else {
							MessageBox(hWnd, TEXT("Invalid Port Number"), TEXT("Connection Error"), MB_ICONERROR);
						}

						delete connectionInfo;
					} 
					return TRUE;

				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					return TRUE;

				case IDD_CHECK_USER:
					if (IsDlgButtonChecked(hWnd, IDD_CHECK_USER)) {
						EnableWindow(GetDlgItem(hWnd, IDD_BOX_USERNAME), FALSE);
						EnableWindow(GetDlgItem(hWnd, IDD_BOX_PASSWORD), FALSE);
						CheckDlgButton(hWnd, IDD_CHECK_USER, BST_UNCHECKED);
					} else {
						EnableWindow(GetDlgItem(hWnd, IDD_BOX_USERNAME), TRUE);
						EnableWindow(GetDlgItem(hWnd, IDD_BOX_PASSWORD), TRUE);
						CheckDlgButton(hWnd, IDD_CHECK_USER, BST_CHECKED);
					}
					return TRUE;
				}
			return FALSE;

		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;

		default:
			return FALSE;
		}

	return TRUE;
}

// AboutProc - Callback function that handles the "About" dialog
BOOL CALLBACK AboutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDD_BTN_OK:
					EndDialog(hWnd, IDD_BTN_OK);
					return TRUE;
			}
			break;

		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;

		default:
			return FALSE;
	}

	return FALSE;
}

// ValidInt - Takes a string and verifies that every 
// character of it is an integer
BOOL ValidInt(TCHAR * str) {
	int len = _tcslen(str);
	for (int i = 0; i < len; i++) {
		if (!iswdigit(str[i]))
			return FALSE;
	}

	return TRUE;
}

// StartServer - Creates the server handler, allocates the handles needed
// to communicate with server, and attempts to connect to the server using
// the server name and port number passed as TCHARs.
BOOL StartServer(HWND hWnd, ConnectionInfo * connectionInfo, BOOL userAuth) {

	// If the server handler hasn't been created, we create it!
	if (serverHandler == NULL) {
		serverHandler = new ServerHandler();

		if (!serverHandler->AllocateHandles()) {
			MessageBox(hWnd, TEXT("Unable to allocate connection handles"), TEXT("Connection unsuccessful"), MB_ICONERROR);
			CloseServer();
			return FALSE;
		}
		
		if (serverHandler->ConnectToInstance(connectionInfo, userAuth, hWnd)) {
			MessageBox(hWnd, TEXT("Successfully connected to server"), TEXT("Connection successful"), MB_OK);
			return TRUE;
		} else {
			MessageBox(hWnd, TEXT("Unable to connect to server."), TEXT("Connection unsuccessful"), MB_ICONERROR);
			CloseServer();
		}
		
	} else {
		MessageBox(hWnd, TEXT("Already connected to the server apparently"), TEXT("Connection unsuccessful"), MB_ICONERROR);
		return TRUE;
	}
	
	return FALSE;
}

// AssembleQuery - Takes the text from the query box and assembles it
// into a single line TCHAR ("finalQuery").
void AssembleQuery(HWND hWnd, TCHAR * finalQuery) {

	char uniSpace		  = '\u0020';
	char uniQuotationMark = '\u0022';
	char uniApostrophe	  = '\u0027';

	int totalChars = 0;
	int lineFinder = 0;

	int lineCount = (int)SendDlgItemMessage(hWnd, IDQ_WIN_QUERY, EM_GETLINECOUNT, (WPARAM)0, (LPARAM)0);

	for (int i = 0; i < lineCount; i++) {

		TCHAR currentLine[STR_MAX];

		// Grabs the number of characters in the current line
		int lineLen = (WORD)SendDlgItemMessage(hWnd, IDQ_WIN_QUERY, EM_LINELENGTH, (WPARAM)lineFinder, (LPARAM)0);

		*((LPWORD)currentLine) = (WORD)lineLen;

		SendDlgItemMessage(hWnd, IDQ_WIN_QUERY, EM_GETLINE, (WPARAM)i, (LPARAM)currentLine);

		// Appends all the chars in the current line
		// to the final query
		if (lineCount == 0) {

			for (int j = 0; j < lineLen; j++) {
				// Replaces unusuable quotation mark with apostrophe
				(currentLine[j] == uniQuotationMark) ? finalQuery[totalChars + j] = uniApostrophe :
													   finalQuery[totalChars + j] = currentLine[j];
			}
			totalChars += lineLen;

		// This adds an add'l space between lines
		} else {
			if (lineLen > 0) {

				finalQuery[totalChars] = uniSpace;
				for (int j = 1; j < lineLen; j++) {
					// Replaces unusuable quotation mark with apostrophe
					(currentLine[j - 1] == uniQuotationMark) ? finalQuery[totalChars + j] = uniApostrophe :
															   finalQuery[totalChars + j] = currentLine[j - 1];
				}
				finalQuery[totalChars + lineLen] = currentLine[lineLen - 1];
				totalChars += lineLen + 1;
			}
		}

		// The "+ 2" accounts for new lines and carriage returns
		lineFinder += lineLen + 2;
	}

	finalQuery[totalChars] = 0;
	return;
}

// GetLineFromBox - Gets the text from the edit control specified
// and returns it via the TCHAR string
void GetLineFromBox(HWND hWnd, TCHAR * string, UINT menuID) {
	WORD stringLen = (WORD)SendDlgItemMessage(hWnd, menuID, EM_LINELENGTH, (WPARAM)0, (LPARAM)0);
	*((LPWORD)string) = stringLen;
	SendDlgItemMessage(hWnd, menuID, EM_GETLINE, (WPARAM)0, (LPARAM)string);
	string[stringLen] = 0;
}

// ResetListView - Removes every column and item/subitem from
// the results listview.
void ResetListView(HWND hWnd) {

	HWND hWndListV = GetDlgItem(hWnd, IDQ_LV_RESULTS);

	HWND hWndHdr = (HWND)SendMessage(hWndListV, LVM_GETHEADER, 0, 0);
	int numColumns = (int)SendMessage(hWndHdr, HDM_GETITEMCOUNT, 0, 0L);

	ListView_DeleteAllItems(hWndListV);
	for (int i = 0; i < numColumns; i++)
		ListView_DeleteColumn(hWndListV, 0);
}

// CreateListView - Creates and populates the listview with results
// returned from ServerHandler
void CreateListView(HWND hWnd, QueryResults * queryResults) {

	HWND hWndListV = GetDlgItem(hWnd, IDQ_LV_RESULTS);

	// Creates and adds columns
	LVCOLUMN lvc;
	ZeroMemory(&lvc, sizeof(LVCOLUMN));
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;

	int numHeaders = queryResults->numHeaders;
	for (int i = 0; i < numHeaders; i++) {
		lvc.iSubItem = i;
		lvc.pszText = queryResults->headers[i];
		lvc.cx = 120;
		ListView_InsertColumn(hWndListV, i+1, (LPARAM)&lvc);
	}

	// Creates and adds items/subitems
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_TEXT;

	int numRows = queryResults->numRows;
	for (int i = 0; i < numRows; i++) {
		lvi.iItem = i;

		lvi.pszText = queryResults->rows[i]->cells[0];
		lvi.iSubItem = 0;
		ListView_InsertItem(hWndListV, (LPARAM)&lvi);

		for (int j = 1; j < numHeaders; j++) {
			lvi.pszText = queryResults->rows[i]->cells[j];
			lvi.iSubItem = j;
			ListView_SetItem(hWndListV, (LPARAM)&lvi);
		}
	}
	
	return;
}

// StopServer - Attempts to close the server connection and displays a message appropriately
void StopServer(HWND hWnd) {
	CloseServer() ? MessageBox(hWnd, TEXT("Successfully disconnected from server"), TEXT("Disconnect successful"), MB_OK) :
					MessageBox(hWnd, TEXT("Connection is already disconnected"), TEXT("Disconnect unsuccessful"), MB_ICONERROR);
}

// CloseServer - Checks to see if the server handler is active and if so,
// closes the connection and deletes the server handler.
BOOL CloseServer() {
	if (serverHandler != NULL) {
		delete serverHandler;
		serverHandler = NULL;
		return TRUE;
	}

	return FALSE;
}

// EnableConnectionUIElements - Enables/disables any 
// connection menu items or buttons.
void EnableConnectionUIElements(HWND hWnd, BOOL connected) {
	EnableWindow(GetDlgItem(hWnd, IDQ_BTN_SUBMIT), connected);
	EnableMenuItem(GetMenu(hWnd), IDM_FILE_DISCONNECT, connected ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(GetMenu(hWnd), IDM_FILE_CONNECT, connected ? MF_DISABLED : MF_ENABLED);
}

// DrawLabel - Draws a line and a text label on top of it 
// and anchors it above an edit control
BOOL DrawLabel(HDC hdc, HWND hWnd, TCHAR * str) {

	// Checks the size of the string first -
	// the "- 2" takes into account the two spaces added later.
	if (_tcslen(str) >= STR_MAX - 2)
		return FALSE;

	RECT hWndRect;
	if (GetWindowRect(hWnd, &hWndRect)) {

		// Used to get hWnd's coordinates divorced from the main hWnd
		MapWindowPoints(HWND_DESKTOP, GetParent(hWnd), (LPPOINT)&hWndRect, 2);

		DrawLine(hdc, RGB(120, 120, 120), hWndRect.left, hWndRect.top - 13, hWndRect.right, hWndRect.top - 13);

		RECT txtRect;
		txtRect.left   = hWndRect.left + 22;
		txtRect.right  = hWndRect.right;
		txtRect.top	   = hWndRect.top - 22;
		txtRect.bottom = hWndRect.top;

		SetBkColor(hdc, BG_COLOR);
		SetBkMode(hdc, OPAQUE);

		// Adds a space before and after string to make it look nicer
		TCHAR displayStr[STR_MAX];
		swprintf(displayStr, STR_MAX, TEXT("%ls%ls%ls"), TEXT(" "), str, TEXT(" "));

		if (!DrawText(hdc, displayStr, -1, &txtRect, DT_LEFT))
			return FALSE;

		return TRUE;
	}

	return FALSE;
}

// DrawLine - Draws a line from x1, y1 to x2, y2 with the color entered
void DrawLine(HDC hdc, COLORREF color, int x1, int y1, int x2, int y2) {
	SelectObject(hdc, GetStockObject(DC_PEN));
	SetDCPenColor(hdc, color);
	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);
}