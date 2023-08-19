#include "Interface.h"
#include "Cfg.h"
#include "xorstr.hpp"
#include "VMTHooking.h"

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;

bool get_present_pointer() {
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = GetForegroundWindow();
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  IDXGISwapChain* swap_chain;
  ID3D11Device* device;

  const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
  if (D3D11CreateDeviceAndSwapChain(
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
	nullptr,
	nullptr) == S_OK) {
	void** p_vtable = *reinterpret_cast<void***>(swap_chain);
	swap_chain->Release();
	device->Release();
	//context->Release();
	p_present_target = (present)p_vtable[8];
	return true;
  }
  return false;
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
	case WM_MOUSEMOVE: return false;
	default:
	  break;
}
	return true;
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

	  TCHAR windir[MAX_PATH];
	  GetWindowsDirectory(windir, MAX_PATH);

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
	}
	else
	  return p_present(p_swap_chain, sync_interval, flags);
  }

  // PERFORMANCE TESTING
  if (Cfg::DBG::performanceTest) {
	if (frameCount == 75) {
	  PreUpdate::perf = QPC(false);
	  frameCount = 0;
	  QPC(true);
	} else {
	  frameCount++;
  if (GetAsyncKeyState(VK_INSERT) & 0x1) G::isMenuVisible = !G::isMenuVisible;
  if (GetAsyncKeyState(VK_END) & 0x8000) G::shouldExit = true;

  static bool reHook = false;
  auto pLocal = PlayerManager::GetInstance()->GetLocalPlayer();

  if (!IsValidPtr(pLocal)) { //nullptr when loading to the server
	if (!reHook) {
	  reHook = true;
	  HooksManager::Get()->pPreFrameHook->Release();
	}
  } else if (reHook) { //fully loaded
	reHook = false;
	HooksManager::Get()->pPreFrameHook->Setup(BorderInputNode::GetInstance()->m_pInputNode);
	HooksManager::Get()->pPreFrameHook->Hook(Index::PRE_FRAME_UPDATE, HooksManager::PreFrameUpdate);
  }
  // PERFORMANCE TESTING

  static auto pSSmoduleClass = (uintptr_t*)OFFSET_SSMODULE;
  if (!IsValidPtr(pSSmoduleClass)) return p_present(p_swap_chain, sync_interval, flags);

  //Simple PBSS bypass
  auto ssRequest = (*(int*)(*pSSmoduleClass + 0x14) != -1);
  if (ssRequest) return p_present(p_swap_chain, sync_interval, flags);

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame();

  if (G::isMenuVisible) {
	ImGui::Begin(xorstr_("BF4 Purge v3   [unknowncheats.me]"), &G::isMenuVisible);
	ImGui::SetWindowSize(ImVec2(480, 500), ImGuiCond_Always);
	ImGui::Text("Options:");
	//ImGui::Text(Cfg::ESP::validPlayers.c_str());
	ImGui::Checkbox("Performance Testing", &Cfg::DBG::performanceTest);
	if (Cfg::DBG::performanceTest) ImGui::Text(std::to_string(PreUpdate::perf).c_str());
	ImGui::Checkbox("Enable ESP", &Cfg::ESP::enable);
	ImGui::Checkbox("Show Teammates", &Cfg::ESP::team);
	ImGui::Checkbox("3D Boxes", &Cfg::ESP::use3DplayerBox);
	ImGui::Checkbox("ESP Vehicles", &Cfg::ESP::vehicles);
	ImGui::Checkbox("3D Vehicle Boxes", &Cfg::ESP::use3DvehicleBox);
	ImGui::Checkbox("ESP Explosives", &Cfg::ESP::explosives);
	ImGui::Checkbox("ESP Lines", &Cfg::ESP::lines);
	ImGui::Checkbox("ESP Lines Allies", &Cfg::ESP::alliesLines);
	ImGui::Checkbox("ESP Vehicles Lines", &Cfg::ESP::linesVehicles);
	ImGui::Checkbox("ESP Spectators", &Cfg::ESP::spectators);
	ImGui::Checkbox("Stats", &Cfg::Misc::showStats);
	ImGui::Checkbox("Aimbot", &Cfg::AimBot::enable);
	ImGui::SliderFloat("##s", &Cfg::AimBot::smoothSoldier, 1.0f, 10.0f, "Smoothing: %.1f");
	ImGui::SliderFloat("##v", &Cfg::AimBot::smoothVehicle, 1.0f, 10.0f, "S. Vehicle: %.1f");
	static int selected = UpdatePoseResultData::BONES::Neck;
	ImGui::RadioButton("Head", &selected, UpdatePoseResultData::BONES::Head); ImGui::SameLine();
	ImGui::RadioButton("Neck", &selected, UpdatePoseResultData::BONES::Neck); ImGui::SameLine();
	ImGui::RadioButton("Spine", &selected, UpdatePoseResultData::BONES::Spine2);
	Cfg::AimBot::bone = (UpdatePoseResultData::BONES)selected;

	ImGui::End();
  }

  ImGuiIO& io = ImGui::GetIO();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
  ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
  ImGui::Begin("XXX", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImDrawList* draw_list = window->DrawList;

  Visuals::RenderVisuals();

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
  Sleep(50);
  if (!get_present_pointer()) {
	return false;
  }

  if (MH_Initialize() != MH_OK) {
	return false;
  }

  if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
	return false;
  }

  if (MH_EnableHook(p_present_target) != MH_OK) {
	return false;
  }

  return true;
}

bool Interface::ShutdownVisuals() {
  if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
	return false;
  }
  if (MH_Uninitialize() != MH_OK) {
	return false;
  }

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
  if (p_context) { p_context->Release(); p_context = NULL; }
  if (p_device) { p_device->Release(); p_device = NULL; }
  //SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc)); // unhook

  return true;
}
