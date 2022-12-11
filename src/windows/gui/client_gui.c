/*

	thanks for
		https://zetcode.com/gui/winapi/

*/

#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define bool unsigned char

#define false 0
#define true  1

#define IDM_INDEX(v)	(v)             // reserved 0...32 indeces
#define ID_INDEX(v)		(v + 32)        // reserved 0...32 indeces

#define ID_CONTROL(v)	(g_form.controls.objects[ID_INDEX(v)])
#define IDM_FORM_QUIT	(IDM_INDEX(1))
#define IDM_FORM_MAIN	(IDM_INDEX(2))

typedef enum
{
	_control_type_none,
	_control_type_edit,
	_control_type_label,
	_control_type_button,
} _control_type;

typedef enum
{
	_form_type_none,
	_form_type_main,
} _form_type;

typedef struct
{
	struct
	{
		HWND          objects[32 * 2];
		_control_type type[32 * 2];
	} controls;
	_form_type type;
} _form_data;

static _form_data g_form;

void initialize_menu(HWND hwnd)
{
	HMENU hMenubar = CreateMenu();

	HMENU hMenu = CreateMenu();
	AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(hMenu, MF_STRING, IDM_FORM_QUIT, L"&Quit");
	AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hMenu, L"&File");

	SetMenu(hwnd, hMenubar);
}

void load_main_form(HWND hwnd);

void initialize_window(HWND hwnd)
{
	initialize_menu(hwnd);
	load_main_form(hwnd);
}

void dispose_current_form(_form_data* form_data)
{
	for (uint32_t i = 0u; i < _countof(form_data->controls.objects); ++i)
	{
		if (form_data->controls.objects[i])
		{
			DestroyWindow(form_data->controls.objects[i]);
			form_data->controls.objects[i] = NULL;
		}
	}
}

void setup_edit(_form_data* form, HWND hwnd, uint32_t index, int x, int y, int width, int height)
{
	index = ID_INDEX(index);
	form->controls.objects[index] = CreateWindowA("Edit", NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, width, height, hwnd, index, NULL, NULL);
	form->controls.type[index] = _control_type_edit;
}

void setup_label(_form_data* form, HWND hwnd, uint32_t index, int x, int y, int width, int height, char* text)
{
	index = ID_INDEX(index);
	form->controls.objects[index] = CreateWindowA("Static", text,
		WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, width, height, hwnd, index, NULL, NULL);
	form->controls.type[index] = _control_type_label;
}

void setup_button(_form_data* form, HWND hwnd, uint32_t index, int x, int y, int width, int height, char* text)
{
	index = ID_INDEX(index);
	form->controls.objects[index] = CreateWindowA("button", text,
		WS_VISIBLE | WS_CHILD, x, y, width, height, hwnd, index, NULL, NULL);
	form->controls.type[index] = _control_type_button;
}

char* get_window_text(HWND hwnd)
{
	uint32_t size = GetWindowTextLengthA(hwnd) + 1u;

	char* buffer = malloc(size);
	if (!buffer) return NULL;

	buffer[size - 1u] = '\0';
	GetWindowTextA(hwnd, buffer, size);

	return buffer;
}

void load_main_form(HWND hwnd)
{
	dispose_current_form(&g_form);

	uint32_t offset_y = 10u;

	setup_label(&g_form, hwnd, 1, 10, offset_y, 80, 20, "ip port");
	setup_edit(&g_form, hwnd, 2, 100, offset_y, 140, 20);
	offset_y += 30u;

	setup_label(&g_form, hwnd, 3, 10, offset_y, 80, 20, "passphrase");
	setup_edit(&g_form, hwnd, 4, 100, offset_y, 140, 20);
	offset_y += 30u;

	setup_button(&g_form, hwnd, 5, 10, offset_y, 230, 20, "register");
	offset_y += 30u;

	setup_label(&g_form, hwnd, 6, 10, offset_y, 100, 20, "source");
	setup_edit(&g_form, hwnd, 7, 100, offset_y, 140, 20);
	offset_y += 30u;

	setup_label(&g_form, hwnd, 8, 10, offset_y, 100, 20, "destination");
	setup_edit(&g_form, hwnd, 9, 100, offset_y, 140, 20);
	offset_y += 30u;

	setup_button(&g_form, hwnd, 10, 10, offset_y, 80, 20, "receive");
	setup_button(&g_form, hwnd, 11, 100, offset_y, 140, 20, "transmit");
	offset_y += 30u;

	setup_label(&g_form, hwnd, 12, 10, offset_y, 80, 20, "message");
	setup_edit(&g_form, hwnd, 13, 100, offset_y, 140, 20);
	offset_y += 30u;

	setup_button(&g_form, hwnd, 14, 10, offset_y, 230, 20, "send");
	offset_y += 30u;

	setup_label(&g_form, hwnd, 15, 10, offset_y, 80, 20, "path");
	setup_edit(&g_form, hwnd, 16, 100, offset_y, 140, 20);
	offset_y += 30u;

	setup_button(&g_form, hwnd, 17, 10, offset_y, 230, 20, "receive");
	offset_y += 30u;

	g_form.type = _form_type_main;
}

void on_register_click(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	char* ip_port	 = get_window_text(ID_CONTROL(2));
	char* passphrase = get_window_text(ID_CONTROL(4));

	char buffer[1024] = { 0 };

	strcat_s(buffer, sizeof(buffer), "client.exe ");
	strcat_s(buffer, sizeof(buffer), ip_port);
	strcat_s(buffer, sizeof(buffer), " \"");
	strcat_s(buffer, sizeof(buffer), passphrase);
	strcat_s(buffer, sizeof(buffer), "\" g");
	
	FILE* file = _popen(buffer, "rt");
	if (file) _pclose(file);

	free(passphrase);
	free(ip_port);
}

void on_receive_transmit_click(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool receive)
{
	char* ip_port	 = get_window_text(ID_CONTROL(2));
	char* passphrase = get_window_text(ID_CONTROL(4));

	char buffer[1024] = { 0 };

	strcat_s(buffer, sizeof(buffer), "client.exe ");
	strcat_s(buffer, sizeof(buffer), ip_port);
	strcat_s(buffer, sizeof(buffer), " \"");
	strcat_s(buffer, sizeof(buffer), passphrase);
	strcat_s(buffer, sizeof(buffer), receive ? "\" r " : "\" t ");

	char* from = get_window_text(ID_CONTROL(7));
	char* to   = get_window_text(ID_CONTROL(9));

	strcat_s(buffer, sizeof(buffer), "\"");
	strcat_s(buffer, sizeof(buffer), from);
	strcat_s(buffer, sizeof(buffer), "\" \"");
	strcat_s(buffer, sizeof(buffer), to);
	strcat_s(buffer, sizeof(buffer), "\"");

	free(to);
	free(from);
	
	FILE* file = _popen(buffer, "rt");
	if (file) _pclose(file);

	free(passphrase);
	free(ip_port);
}

void on_send_click(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	char* ip_port	 = get_window_text(ID_CONTROL(2));
	char* passphrase = get_window_text(ID_CONTROL(4));
	char* message	 = get_window_text(ID_CONTROL(13));

	char buffer[1024] = { 0 };

	strcat_s(buffer, sizeof(buffer), "client.exe ");
	strcat_s(buffer, sizeof(buffer), ip_port);
	strcat_s(buffer, sizeof(buffer), " \"");
	strcat_s(buffer, sizeof(buffer), passphrase);
	strcat_s(buffer, sizeof(buffer), "\" m \"");
	strcat_s(buffer, sizeof(buffer), message);
	strcat_s(buffer, sizeof(buffer), "\"");

	FILE* file = _popen(buffer, "rt");
	if (file) _pclose(file);

	free(message);
	free(passphrase);
	free(ip_port);
}

void on_receive_click(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	char* ip_port	 = get_window_text(ID_CONTROL(2));
	char* passphrase = get_window_text(ID_CONTROL(4));
	char* path		 = get_window_text(ID_CONTROL(16));

	char buffer[8192] = { 0 };

	strcat_s(buffer, sizeof(buffer), "client.exe ");
	strcat_s(buffer, sizeof(buffer), ip_port);
	strcat_s(buffer, sizeof(buffer), " \"");
	strcat_s(buffer, sizeof(buffer), passphrase);
	strcat_s(buffer, sizeof(buffer), "\" l \"");
	strcat_s(buffer, sizeof(buffer), path);
	strcat_s(buffer, sizeof(buffer), "\"");

	FILE* file = _popen(buffer, "rt");

	if (file)
	{
		char temp[1024];
		buffer[0] = '\0';

		while (!feof(file))
		{
			fgets(temp, sizeof(temp), file);

			if (temp[0] == '[') continue;
			if (temp[0] == '\n') continue;

			strcat_s(buffer, sizeof(buffer), temp);
		}

		_pclose(file);
	}

	free(path);
	free(passphrase);
	free(ip_port);

	MessageBoxA(hwnd, buffer, "message from a server", MB_ICONINFORMATION | MB_OK);
}

void on_main_form_command(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (LOWORD(wparam))
	{
		case ID_INDEX(5): // register
			return on_register_click(hwnd, msg, wparam, lparam);
		case ID_INDEX(10): // receive
			return on_receive_transmit_click(hwnd, msg, wparam, lparam, true);
		case ID_INDEX(11): // transmit
			return on_receive_transmit_click(hwnd, msg, wparam, lparam, false);
		case ID_INDEX(14): // send
			return on_send_click(hwnd, msg, wparam, lparam);
		case ID_INDEX(17): // receive
			return on_receive_click(hwnd, msg, wparam, lparam);
	}
}

void on_wm_command(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (LOWORD(wparam))
	{
		case IDM_FORM_QUIT:
			return SendMessageA(hwnd, WM_CLOSE, 0, 0);
		case IDM_FORM_MAIN:
			return load_main_form(hwnd);
	}

	switch (g_form.type)
	{
		case _form_type_main:
			return on_main_form_command(hwnd, msg, wparam, lparam);
	}
}

void process_window_message_hk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_CREATE:
			return initialize_window(hwnd);
		case WM_COMMAND:
			return on_wm_command(hwnd, msg, wparam, lparam);
		case WM_DESTROY:
			return PostQuitMessage(0);
	}
}

LRESULT CALLBACK process_window_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	process_window_message_hk(hwnd, msg, wparam, lparam);
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, PWSTR pcmdline, int ncmdshow)
{
	WNDCLASSW wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.lpszClassName	= L"Window";
	wc.hInstance		= hinstance;
	wc.hbrBackground	= GetSysColorBrush(COLOR_3DFACE);
	wc.lpszMenuName		= NULL;
	wc.lpfnWndProc		= process_window_message;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassW(&wc);

	HWND hwnd = CreateWindowW(wc.lpszClassName, L"client gui", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 100, 270, 370, NULL, NULL, hinstance, NULL);

	ShowWindow(hwnd, ncmdshow);
	UpdateWindow(hwnd);

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}