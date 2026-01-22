#include "DX11app.h"
#include "resource.h"
#include "DX11Renderer.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DX11App* gThisApp = nullptr;

DX11App::DX11App()
{
	gThisApp = this;
}

DX11App::~DX11App()
{
}

HRESULT DX11App::init()
{
	m_pRenderer = new DX11Renderer();

	HRESULT hr = m_pRenderer->Init(m_hWnd);

	return hr;
}

void DX11App::cleanUp()
{
}

HRESULT DX11App::initWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APP_ICON);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"lWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_APP_ICON);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	m_hInst = hInstance;
	RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

	m_viewWidth = SCREEN_WIDTH;
	m_viewHeight = SCREEN_HEIGHT;


	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);


	int xPos = (screenWidth - m_viewWidth) / 2;
	int yPos = (screenHeight - m_viewHeight) / 2;

	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	m_hWnd = CreateWindow(L"lWindowClass", L"LucyLabs Proprietary Advanced Realtime Graphics Framework",
		WS_OVERLAPPEDWINDOW,
		xPos, yPos, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!m_hWnd)
		return E_FAIL;

	LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
	style &= ~WS_MAXIMIZEBOX; // Remove maximize box
	SetWindowLong(m_hWnd, GWL_STYLE, style);
	SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	ShowWindow(m_hWnd, nCmdShow);

	return S_OK;
}

float DX11App::calculateDeltaTime()
{
	//Static initializes this value only once
	static ULONGLONG frameStart = GetTickCount64();

	// Calculate the time since the last frame.
	ULONGLONG frameNow = GetTickCount64();
	m_currentDeltaTime = (frameNow - frameStart) / 1000.0f;
	m_totalTime += m_currentDeltaTime;
	frameStart = frameNow;

	m_pRenderer->m_currentDeltaTime = m_currentDeltaTime;

	m_pRenderer->m_totalTime = m_totalTime;
	return m_currentDeltaTime;
}

//--------------------------------------------------------------------------------------
// update
//--------------------------------------------------------------------------------------
void DX11App::update()
{
	float t = calculateDeltaTime(); // NOT capped at 60 fps

	m_pRenderer->Update(t);
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	if (gThisApp->getRenderer()) gThisApp->getRenderer()->Input(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_KEYDOWN:
		break;

	case WM_RBUTTONDOWN:
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;

	case WM_ACTIVATE:
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
	{
		if (wParam != SIZE_MINIMIZED)
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);

			if (gThisApp->getRenderer())  gThisApp->getRenderer()->OnResize(width, height);
		}
	}
	break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}