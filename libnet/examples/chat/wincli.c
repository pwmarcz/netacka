/*----------------------------------------------------------------
 * chat/client.c - chat client demo for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#define  STRICT
#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnet.h>

#include "chat.h"


#define SIZE 1000


NET_CHANNEL *chan;
char buffer[SIZE];
int netdriver = 0;

static char szAppname[] = "Libnet: WinClient";
static HWND hwnd;
static HINSTANCE hInstance;

static NET_DRIVERNAME * drvsforlb;
static char nick[1024];
static char addr[1024];

static char outtext[4096];

static int mytimer;

BOOL CALLBACK loginproc (HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			SetDlgItemText(hdlg, 101, "");
			SetDlgItemText(hdlg, 102, "");
			nick[0] = 0;
			addr[0] = 0;
			break;
		case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
				memset (addr, 0, 1000);
				memset (nick, 0, 1000);
				GetDlgItemText(hdlg, 101, addr, 1000);
				GetDlgItemText(hdlg, 102, nick, 1000);
				SendMessage (hdlg, WM_CLOSE, 0, 0);
				break;
			case IDCANCEL:
				SendMessage (hdlg, WM_CLOSE, 0, -1L);
				break;
		}
		break;
		case WM_CLOSE:
			EndDialog (hdlg, lParam);
			break;
		default:
			return 0;
	}
	return -1;
}

BOOL CALLBACK list_driversproc (HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
int i;
NET_DRIVERNAME * drvs;
char choice[256];

	switch (msg) {
		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, 101, LB_RESETCONTENT, 0, 0);
			drvs = drvsforlb;
			while (drvs->name) {
				SendDlgItemMessage(hdlg, 101, LB_ADDSTRING,
					0, (LPARAM) ((LPSTR) drvs->name));
				drvs++;
			}
			break;
		case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
				i = SendDlgItemMessage(hdlg, 101, LB_GETCURSEL, 0, 0);
				if (i != LB_ERR) {
					SendDlgItemMessage(hdlg, 101,	LB_GETTEXT,
						(WPARAM) i, (LPARAM) ((LPSTR) choice));
					drvs = drvsforlb;
					i = -1;
					while (drvs->name) {
						if (!strcmp (drvs->name, choice)) i = drvs->num;
						drvs++;
					}
				} else i = -1;
				SendMessage (hdlg, WM_CLOSE, 0, i);
				break;
			case IDCANCEL:
				netdriver = -1;
				SendMessage (hdlg, WM_CLOSE, 0, -1L);
				break;
		}
		break;
		case WM_CLOSE:
			EndDialog (hdlg, lParam);
			break;
		default:
			return 0;
	}
	return -1;
}

int list_drivers (HWND hwnd, NET_DRIVERNAME *drvs, char *title)
{
int i;
DLGPROC dlgprc;

	drvsforlb = drvs;
	dlgprc = (DLGPROC) MakeProcInstance (list_driversproc, hInstance);
	i = DialogBox (hInstance, (LPCSTR) "NETDRIVD", hwnd, dlgprc);
	FreeProcInstance((FARPROC) dlgprc);
	return i;
}

int in_list (NET_DRIVERNAME *drvs, int x) {
	while (drvs->name) {
		if (x==drvs->num) return 1;
		drvs++;
	}
	return 0;
}


void get_driver(HWND hwnd)
{
char buffer[20];
int choice;
NET_DRIVERNAME *drivers;
NET_DRIVERLIST avail;

	SetWindowText (hwnd, "Detecting available drivers...");
	avail = net_detectdrivers (net_drivers_all);
	SetWindowText (hwnd, "Getting detected driver names...");
	drivers = net_getdrivernames (avail);
	SetWindowText (hwnd, szAppname);

	choice = list_drivers (hwnd, drivers, "Available drivers");

	free (drivers);

	netdriver = choice;
}


void init(HWND hwnd)
{
char temp[1024], newaddr[NET_MAX_ADDRESS_LENGTH];
DLGPROC dlgprc;
NET_DRIVERLIST drv;
int i;

	drv = net_driverlist_create();
	net_driverlist_clear (drv);
	net_driverlist_add (drv, netdriver);

	if (!net_initdrivers (drv)) {
		MessageBox (hwnd, "Error initialising driver", szAppname, MB_ICONEXCLAMATION|MB_OK);
		exit (1);
	}

	dlgprc = (DLGPROC) MakeProcInstance (loginproc, hInstance);
	i = DialogBox (hInstance, (LPCSTR) "LOGIND", hwnd, dlgprc);
	FreeProcInstance((FARPROC) dlgprc);

	if (1 == -1) return;

	if (!(chan = net_openchannel (netdriver, NULL))) {
		MessageBox (hwnd, "Unable to open channel", szAppname, MB_ICONEXCLAMATION|MB_OK);
		exit (2);
	}

	sprintf (temp, "Connecting to %s...", addr);
	SetWindowText (hwnd, temp);
	return; //Just for testing...

	net_assigntarget (chan, addr);
	sprintf (temp, "%c%s", CHAT_MAGIC, nick);
	net_send (chan, temp, strlen (temp));

	while ((!net_query (chan)));

	if (0) {
		MessageBox (hwnd, "Aborted", szAppname, MB_ICONEXCLAMATION|MB_OK);
		exit (3);
	}

	{
		int x = net_receive (chan, temp, 1024, newaddr);
		if (x == -1) {
			MessageBox (hwnd, "Receive error", szAppname, MB_ICONEXCLAMATION|MB_OK);
			exit (5);
		}
		temp[x] = 0;
	}

	if (strcmp (temp, "OK")) {
		MessageBox (hwnd, "Connection refused", szAppname, MB_ICONEXCLAMATION|MB_OK);
		exit (4);
	}

	SetWindowText (hwnd, "Connection accepted, redirecting... ");
	net_assigntarget (chan, newaddr);
	SetWindowText (hwnd, szAppname);
}

void ShowIBuff (HWND hwnd, char * text, int col)
{
int i;

	i = strlen (text) + strlen (outtext);
	if (i<4094) {
		strcat (outtext, text);
	} else {
		i -= 4090;
		memmove (outtext, outtext+strlen(text), 4096-strlen(text));
		strcat (outtext, text);
	}
	SetWindowText (GetDlgItem(hwnd, 1004), outtext);
}

int do_client(HWND hwnd)
{
char ibuffer[1024] = {0};
int retval = 0;
int x, col = 0, stop = 0;

//		if (net_query (chan)) {
//			x = net_receive (chan, ibuffer, 1024, NULL);
			x = strlen ("Testing 123  ");
			strcpy (ibuffer, "Testing 123  ");

			if (x<0)
				strcpy (ibuffer, "!!! (local) error reading packet");
			else
				ibuffer[x] = 0;

			switch (ibuffer[0]) {
				case '*': col = 9; break;
				case '+':
				case '-': col = 11; break;
				case '!': col = 12; break;
			}

			ShowIBuff (hwnd, ibuffer, col);

			if (!strcmp (ibuffer, "*** go away")) stop = 1;
			if (!strcmp (ibuffer, "*** server shutting down")) stop = 1;
//		}

		return stop;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
char textbuf[1024];

	switch (message) {
		case WM_CREATE:
			CreateWindow ("edit", "", WS_CHILD|WS_VISIBLE|
				ES_LEFT,
				4, 350, 514, 18, hwnd, (HMENU) 1005, hInstance, NULL);
			CreateWindow ("button", "Send", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
				520, 348, 64, 24, hwnd, (HMENU) 1006, hInstance, NULL);
			CreateWindow ("edit", "", WS_CHILD|WS_VISIBLE|
				ES_LEFT|ES_AUTOVSCROLL|ES_MULTILINE|ES_READONLY,
				4, 4, 584, 344, hwnd, (HMENU) 1004, hInstance, NULL);
			break;
		case WM_DESTROY:
			KillTimer (hwnd, mytimer);
			PostQuitMessage(0);
			break;
		case WM_TIMER:
			do_client (hwnd);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case 1006:
						MessageBox (hwnd, "Got it 2", "Debug", MB_OK);
						SetWindowText (GetDlgItem(hwnd,1005), "");
						break;
				case 1005:
					if ((HIWORD (wParam)) == EN_KILLFOCUS) {
						MessageBox (hwnd, "Got it", "Debug", MB_OK);
						GetWindowText (GetDlgItem(hwnd,1005), textbuf, 1024);
//						net_send (chan, textbuf, strlen (textbuf));
						SetWindowText (GetDlgItem(hwnd,1005), "");
					}
					break;
/*				case 1001:
					GetWindowText (GetDlgItem(hwnd, 1002), textbuf, 128);
					i = atof (textbuf); i *= 1000000.0;
					GetWindowText (GetDlgItem(hwnd, 1003), textbuf, 128);
					i += (double) (atoi (textbuf)*1000);
					sprintf (textbuf, "%.0f", i);
					SetWindowText (GetDlgItem(hwnd, 1010), textbuf);
					sprintf (textbuf, "%.0f", (i/8)*60);
					SetWindowText (GetDlgItem(hwnd, 1011), textbuf);
					GetWindowText (GetDlgItem(hwnd, 1004), textbuf, 128);
					j = 0; k = 0; while (textbuf[j]!=0) { if (textbuf[j]==':') break; j++; }
					if (textbuf[j]==':') { textbuf[j]=0; k = atoi (textbuf); }
					else j = -1;
					l = atoi (textbuf+j+1);
					k *= 60;
					sprintf (textbuf, "Size: %.0fMB,   In bytes:", (((i/8)*(l+k))/1024)/1024);
					SetWindowText (GetDlgItem(hwnd, 1012), textbuf);
					sprintf (textbuf, "%.0f", (i/8)*(l+k));
					SetWindowText (GetDlgItem(hwnd, 1013), textbuf);
					break;
*/			}

			break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
MSG msg;
WNDCLASS wndclass;
char infostr[512];
int i;
HACCEL haccel;

	for (i=0;i<4096;i++) outtext[i] = 0;

	hInstance = hInst;
	if (!hPrevInstance) {
		wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon(hInstance, "MYICON");
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject (GRAY_BRUSH);
		wndclass.lpszMenuName = szAppname;
		wndclass.lpszClassName = szAppname;
		RegisterClass(&wndclass);
	}

	haccel = LoadAccelerators (hInstance, "WCLIACCEL");

	hwnd = CreateWindow(szAppname, szAppname,
								WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
								NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow (hwnd);

	sprintf (infostr, "libnet chat client version %d.%d\n", VER_MAJOR, VER_MINOR);
	sprintf (infostr+(strlen(infostr)), "built at " __TIME__ " on " __DATE__ "\n");
	MessageBox (hwnd, infostr, szAppname, MB_ICONINFORMATION|MB_OK);

	net_init();
	net_loadconfig (NULL);

	get_driver(hwnd);
	if (netdriver == -1) {
		PostQuitMessage (0);
		return msg.wParam;
	}
	init (hwnd);
	mytimer = SetTimer (hwnd, 123, 500, NULL);

	while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
		if (TranslateAccelerator( hwnd, haccel, (LPMSG)&msg ) == 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
