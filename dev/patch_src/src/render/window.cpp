// The game window: swap chain, borderless fullscreen / windowed mode, Alt+Enter, resizing.
#include "render.h"
#include "config.h"

// Client size of a window; false when the window is gone
static bool ClientSize(HWND hwnd, int *w, int *h)
{
	RECT rc;
	if (!hwnd || !IsWindow(hwnd) || !GetClientRect(hwnd, &rc)) {
		return false;
	}
	*w = rc.right;
	*h = rc.bottom;
	return true;
}

bool Render::CreateSwapChain(HWND hwnd)
{
	if (swap_) {
		ctx_->OMSetRenderTargets(0, nullptr, nullptr);
		ctx_->Flush();
		swap_->Release();
		swap_ = nullptr;
	}
	hwnd_ = hwnd;
	int cw = 0, ch = 0;
	if (!ClientSize(hwnd, &cw, &ch) || cw <= 0 || ch <= 0) {
		cw = 640;
		ch = 480;
	}
	swapW_ = cw;
	swapH_ = ch;
	IDXGIDevice *dxgiDev = nullptr;
	IDXGIAdapter *adapter = nullptr;
	IDXGIFactory *factory = nullptr;
	dev_->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDev);
	dxgiDev->GetAdapter(&adapter);
	adapter->GetParent(__uuidof(IDXGIFactory), (void **)&factory);
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = swapW_;
	sd.BufferDesc.Height = swapH_;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.OutputWindow = hwnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	HRESULT hr = factory->CreateSwapChain(dev_, &sd, &swap_);
	if (FAILED(hr)) {
		// Older Windows: no flip model
		sd.BufferCount = 1;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		hr = factory->CreateSwapChain(dev_, &sd, &swap_);
	}
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER); // we handle Alt+Enter ourselves
	factory->Release();
	adapter->Release();
	dxgiDev->Release();
	if (FAILED(hr)) {
		LogLine("ERROR CreateSwapChain %08lx (window %p)", hr, hwnd);
		swap_ = nullptr;
		return false;
	}
	LogLine("swap chain on window %p (%dx%d)", hwnd, swapW_, swapH_);
	return true;
}

// The game believes it runs in exclusive fullscreen: we show a borderless window covering the monitor, or a normal
// window (Alt+Enter, saved in buzz2k.cfg)
void Render::ApplyWindowMode()
{
	if (!hwnd_) {
		return;
	}
	HMONITOR mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi = {sizeof mi};
	GetMonitorInfoA(mon, &mi);
	const RECT &m = mi.rcMonitor;
	if (!g_config.windowed) {
		SetWindowLongA(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowLongA(hwnd_, GWL_EXSTYLE, 0);
		SetWindowPos(hwnd_, HWND_TOP, m.left, m.top, m.right - m.left, m.bottom - m.top,
		             SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	} else {
		DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		RECT r = {0, 0, g_config.windowWidth, g_config.windowHeight};
		AdjustWindowRect(&r, style, FALSE);
		int ww = r.right - r.left, wh = r.bottom - r.top;
		SetWindowLongA(hwnd_, GWL_STYLE, style);
		SetWindowLongA(hwnd_, GWL_EXSTYLE, 0);
		SetWindowPos(hwnd_, HWND_NOTOPMOST, m.left + (m.right - m.left - ww) / 2, m.top + (m.bottom - m.top - wh) / 2, ww,
		             wh, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
}

// Window procedure in front of the game's: Alt+Enter toggles windowed mode; the window size is remembered
static LRESULT CALLBACK BuzzWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_SYSKEYDOWN:
		if (wp == VK_RETURN && (lp & (1 << 29))) {
			g_config.windowed = !g_config.windowed;
			g_render.ApplyWindowMode();
			ConfigSave();
			LogLine("Alt+Enter: %s", g_config.windowed ? "windowed" : "fullscreen");
			return 0;
		}
		break;
	case WM_SYSCHAR:
		if (wp == VK_RETURN) {
			return 0; // no beep
		}
		break;
	case WM_EXITSIZEMOVE:
		if (g_config.windowed) {
			RECT rc;
			GetClientRect(h, &rc);
			if (rc.right >= 320 && rc.bottom >= 240) {
				g_config.windowWidth = rc.right;
				g_config.windowHeight = rc.bottom;
				ConfigSave();
			}
		}
		break;
	}
	return CallWindowProcA((WNDPROC)GetPropA(h, "BuzzGameWndProc"), h, msg, wp, lp);
}

// The game may hand DirectDraw a different window later (the SDK framework recreates it): follow the latest one
void Render::SetWindow(HWND hwnd)
{
	if (!dev_) {
		Init(hwnd);
	} else if (hwnd && hwnd != hwnd_) {
		CreateSwapChain(hwnd);
	}
	if (hwnd && (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC) != BuzzWndProc) {
		SetPropA(hwnd, "BuzzGameWndProc", (HANDLE)GetWindowLongPtrA(hwnd, GWLP_WNDPROC));
		SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)BuzzWndProc);
	}
}

void Render::ResizeSwapChainIfNeeded()
{
	int cw, ch;
	if (!ClientSize(hwnd_, &cw, &ch)) {
		NOT_IMPL("Present on a window that no longer exists");
		return;
	}
	if (cw <= 0 || ch <= 0 || (cw == swapW_ && ch == swapH_)) {
		return;
	}
	ctx_->OMSetRenderTargets(0, nullptr, nullptr);
	HRESULT hr = swap_->ResizeBuffers(0, cw, ch, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) {
		LogLine("ERROR ResizeBuffers to %dx%d: %08lx", cw, ch, hr);
		return;
	}
	swapW_ = cw;
	swapH_ = ch;
	LogLine("swap chain resized to %dx%d", cw, ch);
}
