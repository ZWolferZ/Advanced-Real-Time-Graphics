#pragma once
#include <windows.h>
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/ImGuizmo.h"
#include "structures.h"
#include <string>
#include <d3d11_1.h>
#include <unordered_map>
#include <functional>
#include "miniaudio.h"

#include "Scene.h"

class ImGuiRendering
{
public:
	ImGuiRendering(HWND hwnd, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext, vector<Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>> renderSRVs, ma_engine& audioEngine);

	void ShutDownImGui();

	void ImGuiDrawAllWindows(const unsigned int FPS, float totalAppTime, Scene* currentScene, ID3D11DeviceContext* pContext, std::function<void()> toggleFullScreen);

	void	ResetAllWindowsPositions();

	void	UpdateSRVs(vector<Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>> updatedSRVs) { m_textureSRVs = updatedSRVs; }

	bool VSyncEnabled = true;
	bool m_wireframeMode = false;
	bool m_deferredRendering = true;
	bool m_isBorderlessFullscreen = false;
	PostProcessConstantBuffer m_postProcessCBData;
	GameObject* m_selectedObject = nullptr;
	Light* m_selectedLight = nullptr;
	bool showWindows = false;

private:
	void	DrawVersionWindow(const unsigned int FPS, float totalAppTime, std::function<void()> toggleFullScreen);
	void	DrawHideAllWindows();
	void	DrawSelectLightWindow();
	void	DrawLightUpdateWindow();
	void	DrawObjectMovementWindow();
	void	DrawUpdateObjectMaterialBufferWindow(ID3D11DeviceContext* pContext);
	void	DrawObjectGimzo();
	void	DrawObjectSelectionWindow();
	void	DrawPixelShaderSelectionWindow();
	void	DrawTextureSelectionWindow(ID3D11DeviceContext* pContext);
	void	DrawMeshSelectionWindow();
	void	DrawNormalMapSelectionWindow(ID3D11DeviceContext* pContext);
	void	DrawCameraStatsWindow();
	void    DrawCameraSplineWindow();
	void	DrawDeferredRenderingWindow();
	void	DrawPostProcessingWindow();
	void	StartIMGUIDraw();
	void	CompleteIMGUIDraw();
	unordered_map<std::string, ImVec2> m_originalWindowPositions;
	vector<Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>> m_textureSRVs;
	bool showCameraSplineWindow = false;
	Scene* m_currentScene = nullptr;
	int lightIndex = 0;
	bool grayscalemode = false;
	bool blurmode = false;
	bool scanlinemode = false;
	ma_engine m_audioEngine;
};
