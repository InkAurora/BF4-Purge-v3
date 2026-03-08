#include "Interface.h"
#include "Cfg.h"
#include "Logger.h"
#include "xorstr.hpp"
#include "VehicleInput.h"
#include "VMTHooking.h"

namespace {
constexpr const char* kDummyWindowClassName = "BF4PurgeV3DummyWindow";

const char* GetFeatureLevelName(D3D_FEATURE_LEVEL level) {
	switch (level) {
	case D3D_FEATURE_LEVEL_11_0:
		return "11_0";
	case D3D_FEATURE_LEVEL_10_1:
		return "10_1";
	case D3D_FEATURE_LEVEL_10_0:
		return "10_0";
	default:
		return "unknown";
	}
}

void LogWindowDiagnostics(const char* category, HWND hwnd, const char* label) {
	char className[128] = {};
	char windowTitle[256] = {};

	if (hwnd != nullptr) {
		GetClassNameA(hwnd, className, static_cast<int>(sizeof(className)));
		GetWindowTextA(hwnd, windowTitle, static_cast<int>(sizeof(windowTitle)));
	}

	LOG_INFO(
		category,
		"%s hwnd=%p valid=%d visible=%d class='%s' title='%s'",
		label,
		hwnd,
		hwnd != nullptr && IsWindow(hwnd),
		hwnd != nullptr && IsWindowVisible(hwnd),
		className[0] != '\0' ? className : "<none>",
		windowTitle[0] != '\0' ? windowTitle : "<none>");
}

bool CreateDummyWindow(HWND& dummyWindow, bool& classRegisteredThisCall) {
	classRegisteredThisCall = false;
	dummyWindow = nullptr;

	WNDCLASSEXA windowClass = {};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.lpfnWndProc = DefWindowProcA;
	windowClass.hInstance = G::hInst ? G::hInst : GetModuleHandleA(nullptr);
	windowClass.lpszClassName = kDummyWindowClassName;

	SetLastError(0);
	const ATOM classAtom = RegisterClassExA(&windowClass);
	if (classAtom == 0) {
		const DWORD lastError = GetLastError();
		if (lastError != ERROR_CLASS_ALREADY_EXISTS) {
			LOG_ERROR("visuals", "RegisterClassExA failed for dummy window, lastError=%lu", lastError);
			return false;
		}
	} else {
		classRegisteredThisCall = true;
	}

	dummyWindow = CreateWindowExA(
		0,
		kDummyWindowClassName,
		"BF4 Purge v3 Dummy Window",
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		100,
		100,
		nullptr,
		nullptr,
		windowClass.hInstance,
		nullptr);

	if (dummyWindow == nullptr) {
		LOG_ERROR("visuals", "CreateWindowExA failed for dummy window, lastError=%lu", GetLastError());
		if (classRegisteredThisCall) {
			UnregisterClassA(kDummyWindowClassName, windowClass.hInstance);
			classRegisteredThisCall = false;
		}
		return false;
	}

	LogWindowDiagnostics("visuals", dummyWindow, "created dummy output window");
	return true;
}

void DestroyDummyWindow(HWND dummyWindow, bool classRegisteredThisCall) {
	HINSTANCE instance = G::hInst ? G::hInst : GetModuleHandleA(nullptr);
	if (dummyWindow != nullptr) {
		DestroyWindow(dummyWindow);
	}
	if (classRegisteredThisCall) {
		UnregisterClassA(kDummyWindowClassName, instance);
	}
}
}

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;

bool get_present_pointer() {
	HWND dummyWindow = nullptr;
	bool classRegisteredThisCall = false;
	if (!CreateDummyWindow(dummyWindow, classRegisteredThisCall)) {
		return false;
	}

  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = dummyWindow;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  LogWindowDiagnostics("visuals", sd.OutputWindow, "get_present_pointer selected output window");
  LOG_INFO(
    "visuals",
    "creating dummy swap chain: buffers=%u format=%u windowed=%d swapEffect=%u sampleCount=%u",
    sd.BufferCount,
    static_cast<unsigned>(sd.BufferDesc.Format),
    sd.Windowed,
    static_cast<unsigned>(sd.SwapEffect),
    sd.SampleDesc.Count);

  IDXGISwapChain* swap_chain = nullptr;
  ID3D11Device* device = nullptr;
  D3D_FEATURE_LEVEL createdFeatureLevel = static_cast<D3D_FEATURE_LEVEL>(0);

  const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
  const HRESULT hr = D3D11CreateDeviceAndSwapChain(
	NULL,
	D3D_DRIVER_TYPE_HARDWARE,
	NULL,
	0,
	feature_levels,
	2,
	D3D11_SDK_VERSION,
	&sd,
	&swap_chain,
	&device,
	&createdFeatureLevel,
	nullptr);

  LOG_INFO(
	"visuals",
	"D3D11CreateDeviceAndSwapChain returned hr=0x%08lX swapChain=%p device=%p featureLevel=%s",
	static_cast<unsigned long>(hr),
	swap_chain,
	device,
	GetFeatureLevelName(createdFeatureLevel));

  if (FAILED(hr)) {
    if (sd.OutputWindow == nullptr) {
      LOG_WARN("visuals", "dummy swap-chain creation failed with a null output window");
    }
    DestroyDummyWindow(dummyWindow, classRegisteredThisCall);
	return false;
  }

	if (swap_chain == nullptr || device == nullptr) {
	  LOG_ERROR("visuals", "dummy swap-chain creation succeeded but returned null COM pointers");
	  if (swap_chain) swap_chain->Release();
	  if (device) device->Release();
	  DestroyDummyWindow(dummyWindow, classRegisteredThisCall);
	  return false;
	}

	void** p_vtable = *reinterpret_cast<void***>(swap_chain);
	if (p_vtable == nullptr || p_vtable[8] == nullptr) {
	  LOG_ERROR("visuals", "dummy swap-chain vtable did not expose a present pointer");
	  swap_chain->Release();
	  device->Release();
	  DestroyDummyWindow(dummyWindow, classRegisteredThisCall);
	  return false;
	}

	p_present_target = (present)p_vtable[8];
	LOG_INFO("visuals", "resolved present pointer at %p", p_present_target);

	swap_chain->Release();
	device->Release();
	DestroyDummyWindow(dummyWindow, classRegisteredThisCall);
	//context->Release();
	return true;
  }

WNDPROC oWndProc;
// Win32 message handler your application need to call.
// - You should COPY the line below into your .cpp code to forward declare the function and then you can call it.
// - Call from your application's message handler. Keep calling your message handler unless this function returns TRUE.
// Forward declare message handler from imgui_impl_win32.cpp
//extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//  // handle input issues here.
//
//  if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return true;
//
//  return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
//}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall HooksManager::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (G::isMenuVisible) {
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
      return 0; // Prevent mouse clicks being sent to the game
	default:
	  break;
	}

    return 1; // Prevent other inputs being sent to the game
  }

  return CallWindowProc(HooksManager::Get()->oWndproc, hWnd, uMsg, wParam, lParam);
}

string testText = "";

bool init = false;
HWND window = NULL;
ID3D11Device* p_device = NULL;
ID3D11DeviceContext* p_context = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;

static long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
  if (!init) {
	if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&p_device))) {
	  p_device->GetImmediateContext(&p_context);

	  DXGI_SWAP_CHAIN_DESC sd;
	  p_swap_chain->GetDesc(&sd);
	  window = sd.OutputWindow;

	  ID3D11Texture2D* pBackBuffer;
	  p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	  p_device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
	  pBackBuffer->Release();
	  /*oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);*/

	  ImGui::CreateContext();
	  ImGuiIO& io = ImGui::GetIO();

	  char windir[MAX_PATH] = {};
	  GetWindowsDirectoryA(windir, MAX_PATH);

	  std::string fontDir = std::string(windir);
	  fontDir += xorstr_("\\Fonts\\");

	  auto LoadFontFromFile = [&io, &fontDir](const std::string& fontName, float size) -> ImFont* {
		return io.Fonts->AddFontFromFileTTF((fontDir + fontName + ".ttf").c_str(), size);
	  };

	  auto LoadFontFromMemory = [&io, &fontDir](void* fontData, float size) -> ImFont* {
		return io.Fonts->AddFontFromMemoryTTF(fontData, sizeof(fontData), size);
	  };

	  DX::Verdana8 = LoadFontFromFile(xorstr_("Verdana"), 8.f);
	  DX::Verdana12 = LoadFontFromFile(xorstr_("Verdana"), 12.f);
	  DX::Verdana18 = LoadFontFromFile(xorstr_("Verdana"), 18.f);
	  DX::Verdana24 = LoadFontFromFile(xorstr_("Verdana"), 24.f);
	  DX::Verdana48 = LoadFontFromFile(xorstr_("Verdana"), 48.f);

	  io.FontDefault = DX::Verdana18;

	  ImGui_ImplWin32_Init(window);
	  ImGui_ImplDX11_Init(p_device, p_context);
	  init = true;
	  LOG_INFO("visuals", "present detour initialized, window=%p device=%p context=%p", window, p_device, p_context);
	}
	else {
	  static bool s_presentDeviceFailureLogged = false;
	  if (!s_presentDeviceFailureLogged) {
		LOG_WARN("visuals", "IDXGISwapChain::GetDevice failed during present detour initialization");
		s_presentDeviceFailureLogged = true;
	  }
	  return p_present(p_swap_chain, sync_interval, flags);
	}
  }

  G::framecount++;

  static bool betweenMeasurings = false;
  static int lastFrame;
  if (!betweenMeasurings) {
	Misc::QPC(true);
	lastFrame = G::framecount;
	betweenMeasurings = true;
  } else if (Misc::QPC(false) > 1000) {
	G::FPS = G::framecount - lastFrame;
	betweenMeasurings = false;
  }

  if (GetAsyncKeyState(VK_INSERT) & 0x1) G::isMenuVisible = !G::isMenuVisible;
  if (GetAsyncKeyState(VK_END) & 0x8000) SetEvent(G::g_exitEvent);
  if (GetAsyncKeyState(VK_CAPITAL)) {
	Cfg::AimBot::targetLock = true;
  }
  else {
    Cfg::AimBot::targetLock = false;
  }

  static bool reHook = false;
  auto pLocal = PlayerManager::GetInstance()->GetLocalPlayer();

  if (!IsValidPtr(pLocal)) { //nullptr when loading to the server
	if (!reHook) {
	  reHook = true;
	  LOG_WARN("visuals", "local player unavailable, releasing pre-frame hook until the level finishes loading");
	  HooksManager::Get()->pPreFrameHook->Release();
	}
  } else if (reHook) { //fully loaded
	reHook = false;
	LOG_INFO("visuals", "local player restored, reinstalling pre-frame hook");
	if (HooksManager::Get()->pPreFrameHook->Setup(BorderInputNode::GetInstance()->m_pInputNode)) {
	  HooksManager::Get()->pPreFrameHook->Hook(Index::PRE_FRAME_UPDATE, HooksManager::PreFrameUpdate);
	  LOG_INFO("visuals", "pre-frame hook reinstalled after load transition");
	} else {
	  LOG_ERROR("visuals", "failed to reinstall pre-frame hook after load transition");
	}
  }

  static auto pSSmoduleClass = (uintptr_t*)OFFSET_SSMODULE;
  if (!IsValidPtr(pSSmoduleClass)) return p_present(p_swap_chain, sync_interval, flags);

  //Simple PBSS bypass
  auto ssRequest = (*(int*)(*pSSmoduleClass + 0x14) != -1);
  if (ssRequest) return p_present(p_swap_chain, sync_interval, flags);

  float tempAimbotFOV = Cfg::AimBot::radius;

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame();

  ImGuiIO& io = ImGui::GetIO();

  io.MouseDrawCursor = G::isMenuVisible;

  if (G::isMenuVisible) {

	// Manually set mouse position (fixes -FLT_MAX invalidation)
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient(window, &cursorPos);
	io.MousePos = ImVec2(static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y));

	// Manually set mouse button states (ensures clicks register even if WndProc misses them)
	io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;  // Left button
	io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;  // Right button
	io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;  // Middle button
	// Add more for X1/X2 if needed: io.MouseDown[3/4] = GetAsyncKeyState(VK_XBUTTON1/2)

	ImGui::Begin(xorstr_("BF4 Purge v3"), &G::isMenuVisible, ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::BeginTabBar(xorstr_("##tabs"), ImGuiTabBarFlags_None)) {
	  if (ImGui::BeginTabItem(xorstr_("Visuals"))) {
		ImGui::Checkbox(xorstr_("Enable ESP"), &Cfg::ESP::enable);
		ImGui::Checkbox(xorstr_("Show Teammates"), &Cfg::ESP::team);
		ImGui::Checkbox(xorstr_("Lines"), &Cfg::ESP::lines);
		ImGui::Checkbox(xorstr_("Lines to Allies"), &Cfg::ESP::alliesLines);
		ImGui::Checkbox(xorstr_("ESP Vehicles"), &Cfg::ESP::vehicles);
		ImGui::Checkbox(xorstr_("Lines to Vehicles"), &Cfg::ESP::linesVehicles);
		ImGui::Checkbox(xorstr_("3D Boxes"), &Cfg::ESP::use3DplayerBox);
		ImGui::Checkbox(xorstr_("3D Vehicles"), &Cfg::ESP::use3DvehicleBox);
		ImGui::Checkbox(xorstr_("ESP Explosives"), &Cfg::ESP::explosives);
		ImGui::Checkbox(xorstr_("JDAM Prediction"), &Cfg::ESP::predictionBombImpact);
		ImGui::Checkbox(xorstr_("Angular Prediction"), &Cfg::ESP::predictionUseAngularVelocity);
        ImGui::EndTabItem();
	  }

      if (ImGui::BeginTabItem(xorstr_("Aimbot"))) {
		ImGui::Checkbox(xorstr_("Aimbot"), &Cfg::AimBot::enable);
		ImGui::SliderFloat(xorstr_("##aimbotFOV"), &Cfg::AimBot::radius, 5.0f, 200.0f, xorstr_("FOV: %1.f"));
		ImGui::SliderFloat(xorstr_("##soldierSmoothing"), &Cfg::AimBot::smoothSoldier, 1.0f, 10.0f, xorstr_("Smoothing: %.1f"));
		ImGui::SliderFloat(xorstr_("##vehicleSmoothing"), &Cfg::AimBot::smoothVehicle, 1.0f, 10.0f, xorstr_("Smoothing Vehicle: %.1f"));
		const char* turretInputModes[] = {
		  GetVehicleTurretInputModeName(VehicleTurretInputMode::MouseDevice),
		  GetVehicleTurretInputModeName(VehicleTurretInputMode::CrosshairConcepts),
		  GetVehicleTurretInputModeName(VehicleTurretInputMode::CameraConcepts),
		  GetVehicleTurretInputModeName(VehicleTurretInputMode::RightStickConcepts),
		  GetVehicleTurretInputModeName(VehicleTurretInputMode::ActionMapCrosshair),
		};
		ImGui::Combo(xorstr_("Vehicle Turret Input"), &Cfg::AimBot::vehicleTurretInputMode, turretInputModes, IM_ARRAYSIZE(turretInputModes));
		static int selected = UpdatePoseResultData::BONES::Neck;
		ImGui::RadioButton(xorstr_("Head"), &selected, UpdatePoseResultData::BONES::Head); ImGui::SameLine();
		ImGui::RadioButton(xorstr_("Neck"), &selected, UpdatePoseResultData::BONES::Neck); ImGui::SameLine();
		ImGui::RadioButton(xorstr_("Spine"), &selected, UpdatePoseResultData::BONES::Spine2);
		Cfg::AimBot::bone = (UpdatePoseResultData::BONES)selected;
		ImGui::EndTabItem();
	  }

	  /*if (ImGui::BeginTabItem(xorstr_("Radar"))) {
        ImGui::Checkbox(xorstr_("Radar"), &Cfg::ESP::Radar::enable);
	  }*/

	  if (ImGui::BeginTabItem(xorstr_("Misc"))) {
		ImGui::Checkbox(xorstr_("No Recoil"), &Cfg::Misc::disableRecoil);
		ImGui::Checkbox(xorstr_("No Spread"), &Cfg::Misc::disableSpread);
		ImGui::Checkbox(xorstr_("Spectator Warning"), &Cfg::ESP::spectators);
		ImGui::Checkbox(xorstr_("Stats"), &Cfg::Misc::showStats);
		ImGui::Checkbox(xorstr_("Minimap Spot"), &Cfg::Misc::minimapSpot);
		ImGui::Checkbox(xorstr_("Unlock All"), &Cfg::Misc::unlockAll);
		ImGui::Checkbox(xorstr_("No Overheat"), &Cfg::Misc::noOverheat);
        ImGui::EndTabItem();
	  }

      ImGui::EndTabBar();
	}

	ImGui::End();
  }

  static int drawEndFrame = 0;
  if (G::framecount < drawEndFrame || Cfg::AimBot::radius != tempAimbotFOV) {
	if (Cfg::AimBot::radius != tempAimbotFOV) {
	  drawEndFrame = G::framecount + (G::FPS * 2);
	}
	Renderer::DrawCircleOutlined(G::screenCenter, Cfg::AimBot::radius, 0, ImColor::White());
  }

  if (Cfg::DBG::testString != "") Renderer::DrawString({ 100, 100 },
	StringFlag::CENTER_Y, ImColor::Purple(), Cfg::DBG::testString.c_str());

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
  ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
  ImGui::Begin("XXX", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImDrawList* draw_list = window->DrawList;

  F::pVisuals->RenderVisuals();

  window->DrawList->PushClipRectFullScreen();
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);

  ImGui::EndFrame();

  ImGui::Render();

  p_context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  return p_present(p_swap_chain, sync_interval, flags);
}

bool Interface::InitializeVisuals() {
	LOG_INFO("visuals", "InitializeVisuals started");
  Sleep(50);
  if (!get_present_pointer()) {
	LOG_ERROR("visuals", "failed to resolve present pointer");
	return false;
  }

	LOG_INFO("visuals", "present pointer resolved: %p", p_present_target);

  if (MH_Initialize() != MH_OK) {
	LOG_ERROR("visuals", "MH_Initialize failed");
	return false;
  }

  if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
	LOG_ERROR("visuals", "MH_CreateHook failed for IDXGISwapChain::Present");
	return false;
  }

  if (MH_EnableHook(p_present_target) != MH_OK) {
	LOG_ERROR("visuals", "MH_EnableHook failed for IDXGISwapChain::Present");
	return false;
  }

	LOG_INFO("visuals", "InitializeVisuals completed successfully");

  return true;
}

bool Interface::ShutdownVisuals() {
  if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
	LOG_ERROR("visuals", "MH_DisableHook failed during shutdown");
	return false;
  }
  if (MH_Uninitialize() != MH_OK) {
	LOG_ERROR("visuals", "MH_Uninitialize failed during shutdown");
	return false;
  }

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
  if (p_context) { p_context->Release(); p_context = NULL; }
  if (p_device) { p_device->Release(); p_device = NULL; }
  //SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc)); // unhook

	LOG_INFO("visuals", "ShutdownVisuals completed");

  return true;
}
