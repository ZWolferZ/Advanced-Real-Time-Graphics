#pragma once
#include <windows.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "structures.h"
#include <string>
#include <d3d11_1.h>
#include "Scene.h"

class ImGuiRendering
{
public:
	ImGuiRendering(HWND hwnd, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext);

	void ShutDownImGui();

	void ImGuiDrawAllWindows(const unsigned int FPS, float totalAppTime, Scene& currentScene);

private:
	void	DrawVersionWindow(const unsigned int FPS, float totalAppTime);
	void	DrawHideAllWindows();
	void	DrawLightUpdateWindow();
	void	DrawObjectMovementWindow();
	void	DrawObjectSelectionWindow();
	void	StartIMGUIDraw();
	void	CompleteIMGUIDraw();

	bool showWindows = false;
	Scene* m_currentScene = nullptr;
	GameObject* m_selectedObject = nullptr;
};
