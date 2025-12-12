#include "ImGuiRendering.h"

ImGuiRendering::ImGuiRendering(HWND hwnd, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(m_pd3dDevice, m_pImmediateContext);

	io.IniFilename = nullptr;
}

void ImGuiRendering::ShutDownImGui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiRendering::ImGuiDrawAllWindows(const unsigned int FPS, float totalAppTime, Scene* currentScene)
{
	m_currentScene = currentScene;

	StartIMGUIDraw();

	DrawHideAllWindows();

	if (showWindows)
	{
		DrawVersionWindow(FPS, totalAppTime);
		DrawSelectLightWindow();
		DrawLightUpdateWindow();
		DrawObjectSelectionWindow();
		DrawObjectMovementWindow();
		DrawPixelShaderSelectionWindow();

		DrawObjectGimzo();
	}

	CompleteIMGUIDraw();
}

void ImGuiRendering::DrawVersionWindow(const unsigned int FPS, float totalAppTime)
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::Begin("LucyLabs DX11 Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("ImGUI version: (%s)", IMGUI_VERSION);
	ImGui::Text("Application Runtime (%f)", totalAppTime);
	ImGui::Text("FPS %d", FPS);
	ImGui::Checkbox("VSync Enabled", &VSyncEnabled);
	ImGui::End();
}

void ImGuiRendering::DrawHideAllWindows()
{
	ImGui::SetNextWindowPos(ImVec2(1100, 650), ImGuiCond_FirstUseEver);
	ImGui::Begin("Show/Hide UI", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("Hide All Windows", &showWindows);
	ImGui::End();
}

void ImGuiRendering::DrawSelectLightWindow()
{
	ImGui::SetNextWindowPos(ImVec2(250, 10), ImGuiCond_FirstUseEver);
	ImGui::Begin("Light Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Choose an light to select!");
	ImGui::Separator();

	for (unsigned int i = 0; i < MAX_LIGHTS; ++i)
	{
		Light& light = m_currentScene->getLightProperties().Lights[i];
		bool isSelected = (m_selectedLight == &light);

		if (ImGui::Selectable(("Light " + std::to_string(i)).c_str(), isSelected))
		{
			if (isSelected)
			{
				m_selectedLight = nullptr;
			}
			else
			{
				m_selectedLight = &light;
				m_selectedObject = nullptr;
				lightIndex = i;
			}
		}
	}

	ImGui::End();
}

void ImGuiRendering::DrawLightUpdateWindow()
{
	if (m_selectedLight != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 150), ImGuiCond_FirstUseEver);
		ImGui::Begin("Light Movement Update Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Separator();

		bool lightEnabled = m_selectedLight->Enabled;
		ImGui::Text("Light %d", lightIndex);

		if (ImGui::Checkbox(("Light " + std::to_string(lightIndex) + " Enable").c_str(), &lightEnabled))
		{
			lightEnabled ? m_selectedLight->Enabled = 1 : m_selectedLight->Enabled = 0;
		}

		float lightPos[3] = { m_selectedLight->Position.x, m_selectedLight->Position.y, m_selectedLight->Position.z };
		if (ImGui::DragFloat3(("Light " + std::to_string(lightIndex) + " Position").c_str(), lightPos, 0.1f))
		{
			m_selectedLight->Position = XMFLOAT4(lightPos[0], lightPos[1], lightPos[2], 1);
		}
		float lightColor[3] = { m_selectedLight->Color.x, m_selectedLight->Color.y, m_selectedLight->Color.z };
		if (ImGui::ColorEdit3(("Light " + std::to_string(lightIndex) + " Color").c_str(), lightColor))
		{
			m_selectedLight->Color = XMFLOAT4(lightColor[0], lightColor[1], lightColor[2], 1);
		}

		/*	float spotAngle = pLight.SpotAngle;

			if (ImGui::SliderFloat(("Light " + std::to_string(i) + " Spot Angle").c_str(), &spotAngle, 0.0f, 90.0f))
			{
				pLight.SpotAngle = XMConvertToRadians(spotAngle);
			}*/

		float constantAttenuation = m_selectedLight->ConstantAttenuation;
		if (ImGui::SliderFloat(("Light " + std::to_string(lightIndex) + " Constant Attenuation").c_str(), &constantAttenuation, 0.1f, 1.0f))
		{
			m_selectedLight->ConstantAttenuation = constantAttenuation;
		}
		float linearAttenuation = m_selectedLight->LinearAttenuation;
		if (ImGui::SliderFloat(("Light " + std::to_string(lightIndex) + " Linear Attenuation").c_str(), &linearAttenuation, 0.1f, 1.0f))
		{
			m_selectedLight->LinearAttenuation = linearAttenuation;
		}
		float quadraticAttenuation = m_selectedLight->QuadraticAttenuation;
		if (ImGui::SliderFloat(("Light " + std::to_string(lightIndex) + " Quadratic Attenuation").c_str(), &quadraticAttenuation, 0.1f, 1.0f))
		{
			m_selectedLight->QuadraticAttenuation = quadraticAttenuation;
		}
		ImGui::Separator();
		m_currentScene->UpdateLightProperties(lightIndex, *m_selectedLight);
		ImGui::End();
	}
}

void ImGuiRendering::DrawObjectMovementWindow()
{
	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(930, 10), ImGuiCond_FirstUseEver);
		ImGui::Begin("Object Movement Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Manipulate object:");
		ImGui::Text("Selected Object: %s", m_selectedObject->GetObjectName().c_str());
		ImGui::Separator();

		XMFLOAT3 position = m_selectedObject->GetPosition();
		if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.005f))
		{
			m_selectedObject->SetPosition(position);
		}

		XMFLOAT3 rotation = m_selectedObject->GetRotation();
		if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&rotation), 0.5f, -361, 361))
		{
			m_selectedObject->SetRotate(rotation);
		}

		XMFLOAT3 scale = m_selectedObject->GetScale();
		if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.01f, -INFINITY, INFINITY))
		{
			m_selectedObject->SetScale(scale);
		}

		ImGui::Text("(Drag the box or enter a number)");
		ImGui::Separator();

		ImGui::Text("Auto Rotate:");

		ImGui::SliderFloat("Rotation Speed", &m_selectedObject->m_autoRotationSpeed, 0.0f, 360.0f);

		ImGui::Checkbox("Auto Rotate (X+)", &m_selectedObject->m_autoRotateX);

		ImGui::Checkbox("Auto Rotate (Y+)", &m_selectedObject->m_autoRotateY);

		ImGui::Checkbox("Auto Rotate (Z+)", &m_selectedObject->m_autoRotateZ);

		ImGui::Separator();

		if (ImGui::Button("Reset Transform"))
		{
			m_selectedObject->ResetTransform();
		}

		ImGui::End();
	}
}

void ImGuiRendering::DrawObjectGimzo()
{
	// Only one can be selected at a time, dw we are not doing the same math for both.

	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(680, 10), ImGuiCond_FirstUseEver);
		ImGui::Begin("Object Gizmo Type Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGuizmo::SetOrthographic(false);

		// THE ANSWER!!!!
		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());

		ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

		XMFLOAT4X4 object4x4 = *m_selectedObject->GetTransform();
		XMMATRIX objectMatrix = XMLoadFloat4x4(&object4x4);

		float objMat[16];
		XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(objMat), objectMatrix);

		float view[16], proj[16];

		XMFLOAT4X4 v = m_currentScene->GetCamera()->GetViewMatrixFloat4x4();
		XMFLOAT4X4 p = m_currentScene->GetCamera()->GetProjectionMatrixFloat4x4();

		XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(view), XMLoadFloat4x4(&v));
		XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(proj), XMLoadFloat4x4(&p));

		static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE mode = ImGuizmo::LOCAL;

		if (ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) operation = ImGuizmo::TRANSLATE; ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) operation = ImGuizmo::ROTATE; ImGui::SameLine();
		if (ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) operation = ImGuizmo::SCALE;

		if (ImGuizmo::Manipulate(view, proj, operation, mode, objMat))
		{
			XMMATRIX newObjectMatrix = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(objMat));
			m_selectedObject->SetTransform(newObjectMatrix);
		}

		ImGui::End();
	}

	if (m_selectedLight != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(930, 10), ImGuiCond_FirstUseEver);
		ImGuizmo::SetOrthographic(false);
		// THE ANSWER!!!!
		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
		ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
		XMFLOAT4X4 light4x4;
		XMStoreFloat4x4(&light4x4, XMMatrixTranslation(m_selectedLight->Position.x, m_selectedLight->Position.y, m_selectedLight->Position.z));
		XMMATRIX lightMatrix = XMLoadFloat4x4(&light4x4);
		float lightMat[16];
		XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(lightMat), lightMatrix);
		float view[16], proj[16];
		XMFLOAT4X4 v = m_currentScene->GetCamera()->GetViewMatrixFloat4x4();
		XMFLOAT4X4 p = m_currentScene->GetCamera()->GetProjectionMatrixFloat4x4();
		XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(view), XMLoadFloat4x4(&v));
		XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(proj), XMLoadFloat4x4(&p));

		if (ImGuizmo::Manipulate(view, proj, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, lightMat))
		{
			XMMATRIX newLightMatrix = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(lightMat));

			XMVECTOR lightPosVector, dummyVec;

			// Cant just use nullptr for this, so I am just using a dummy vector

			XMMatrixDecompose(&dummyVec, &dummyVec, &lightPosVector, newLightMatrix);

			m_selectedLight->Position = XMFLOAT4(XMVectorGetX(lightPosVector), XMVectorGetY(lightPosVector), XMVectorGetZ(lightPosVector), 1.0f);

			m_currentScene->UpdateLightProperties(lightIndex, *m_selectedLight);
		}
	}
}

void ImGuiRendering::DrawObjectSelectionWindow()
{
	ImGui::SetNextWindowPos(ImVec2(465, 10), ImGuiCond_FirstUseEver);

	ImGui::Begin("Object Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Choose an object to select!");
	ImGui::Separator();

	for (auto dgo : m_currentScene->m_vecDrawables)
	{
		bool isSelected = (m_selectedObject == dgo);

		if (ImGui::Selectable(dgo->GetObjectName().c_str(), isSelected))
		{
			if (isSelected)
			{
				m_selectedObject = nullptr;
			}
			else
			{
				m_selectedObject = dgo;
				m_selectedLight = nullptr;
			}
		}
	}

	ImGui::End();
}

void ImGuiRendering::DrawPixelShaderSelectionWindow()
{
	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(930, 320), ImGuiCond_FirstUseEver);
		ImGui::Begin("Pixel Shader Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Select Pixel Shader for Each Object!");
		ImGui::Separator();
		ImGui::Text("Selected Object: %s", m_selectedObject->GetObjectName().c_str());
		ImGui::Separator();

		auto& pixelShaders = m_currentScene->m_pixelShadersMap;

		for (size_t i = 0; i < pixelShaders.size(); ++i)
		{
			const auto& shaderPair = pixelShaders[i];

			bool isSelected = (m_selectedObject->GetPixelShader().Get() == shaderPair.second.Get());

			if (ImGui::Selectable(shaderPair.first.c_str(), isSelected))
			{
				m_selectedObject->SetPixelShader(shaderPair.second);
			}
		}

		ImGui::End();
	}
}

void ImGuiRendering::StartIMGUIDraw()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
}

void ImGuiRendering::CompleteIMGUIDraw()
{
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}