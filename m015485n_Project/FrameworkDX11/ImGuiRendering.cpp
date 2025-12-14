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

void ImGuiRendering::ImGuiDrawAllWindows(const unsigned int FPS, float totalAppTime, Scene* currentScene, ID3D11DeviceContext* pContext)
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
		DrawUpdateObjectMaterialBufferWindow(pContext);
		DrawObjectMovementWindow();
		DrawPixelShaderSelectionWindow();
		DrawTextureSelectionWindow(pContext);
		DrawNormalMapSelectionWindow(pContext);
		DrawCameraStatsWindow();

		if (showCameraSplineWindow)DrawCameraSplineWindow();

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
	ImGui::Checkbox("Show All Windows", &showWindows);
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
		string lightTypeString;

		switch (m_selectedLight->LightType)
		{
		case DirectionalLight:
			lightTypeString = "Directional Light";
			break;
		case PointLight:
			lightTypeString = "Point Light";
			break;
		case SpotLight:
			lightTypeString = "Spot Light";
			break;
		default:
			lightTypeString = "Unknown Light Type";
		}
		if (ImGui::Checkbox(("Light " + std::to_string(lightIndex) + " Enable").c_str(), &lightEnabled))
		{
			lightEnabled ? m_selectedLight->Enabled = 1 : m_selectedLight->Enabled = 0;
		}

		ImGui::SliderInt(("Light Type: " + lightTypeString).c_str(), &m_selectedLight->LightType, 0, 2);

		if (m_selectedLight->LightType != PointLight)
		{
			float lightDirection[3] = { m_selectedLight->Direction.x, m_selectedLight->Direction.y, m_selectedLight->Direction.z };
			if (ImGui::DragFloat3(("Light " + std::to_string(lightIndex) + " Direction").c_str(), lightDirection, 0.05f, -1.0f, 1.0f))
			{
				m_selectedLight->Direction = XMFLOAT4(lightDirection[0], lightDirection[1], lightDirection[2], 0);
			}
		}
		if (m_selectedLight->LightType == SpotLight)
		{
			float spotAngle = m_selectedLight->SpotAngle;

			if (ImGui::SliderFloat(("Light " + std::to_string(lightIndex) + " Spot Angle").c_str(), &spotAngle, 0.0f, 90.0f))
			{
				m_selectedLight->SpotAngle = XMConvertToRadians(spotAngle);
			}
		}

		if (m_selectedLight->LightType != DirectionalLight)
		{
			float lightPosition[3] = { m_selectedLight->Position.x, m_selectedLight->Position.y, m_selectedLight->Position.z };
			if (ImGui::DragFloat3(("Light " + std::to_string(lightIndex) + " Position").c_str(), lightPosition, 0.1f))
			{
				m_selectedLight->Position = XMFLOAT4(lightPosition[0], lightPosition[1], lightPosition[2], 1);
			}

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
		}

		float lightColor[3] = { m_selectedLight->Color.x, m_selectedLight->Color.y, m_selectedLight->Color.z };
		if (ImGui::ColorEdit3(("Light " + std::to_string(lightIndex) + " Color").c_str(), lightColor))
		{
			m_selectedLight->Color = XMFLOAT4(lightColor[0], lightColor[1], lightColor[2], 1);
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

void ImGuiRendering::DrawUpdateObjectMaterialBufferWindow(ID3D11DeviceContext* pContext)
{
	if (m_selectedObject != nullptr)
	{
		if (m_selectedObject->GetPixelShader() == m_currentScene->GetPixelShader("Solid Pixel Shader")) return;

		ImGui::SetNextWindowPos(ImVec2(930, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin("Object Material Buffer Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		MaterialPropertiesConstantBuffer currentMaterialBuffer = m_selectedObject->GetMaterialConstantBufferData();

		if (m_selectedObject->GetPixelShader() != m_currentScene->GetPixelShader("Texture UnLit Pixel Shader"))
		{
			float ambient[3] = { currentMaterialBuffer.Material.Ambient.x, currentMaterialBuffer.Material.Ambient.y, currentMaterialBuffer.Material.Ambient.z };
			if (ImGui::ColorEdit3("Ambient Color", ambient))
			{
				currentMaterialBuffer.Material.Ambient = XMFLOAT4(ambient[0], ambient[1], ambient[2], 1.0f);
			}
			float diffuse[3] = { currentMaterialBuffer.Material.Diffuse.x, currentMaterialBuffer.Material.Diffuse.y, currentMaterialBuffer.Material.Diffuse.z };
			if (ImGui::ColorEdit3("Diffuse Color", diffuse))
			{
				currentMaterialBuffer.Material.Diffuse = XMFLOAT4(diffuse[0], diffuse[1], diffuse[2], 1.0f);
			}
			float emissive[3] = { currentMaterialBuffer.Material.Emissive.x, currentMaterialBuffer.Material.Emissive.y, currentMaterialBuffer.Material.Emissive.z };
			if (ImGui::ColorEdit3("Emissive Color", emissive))
			{
				currentMaterialBuffer.Material.Emissive = XMFLOAT4(emissive[0], emissive[1], emissive[2], 1.0f);
			}
			float specular[3] = { currentMaterialBuffer.Material.Specular.x, currentMaterialBuffer.Material.Specular.y, currentMaterialBuffer.Material.Specular.z };
			if (ImGui::ColorEdit3("Specular Color", specular))
			{
				currentMaterialBuffer.Material.Specular = XMFLOAT4(specular[0], specular[1], specular[2], 1.0f);
			}
			ImGui::SliderFloat("Specular Power", &currentMaterialBuffer.Material.SpecularPower, 1.0f, 256.0f);

			ImGui::Separator();
			if (m_selectedObject->GetTextureResourceView() != nullptr)
			{
				bool useTexture = currentMaterialBuffer.Material.UseTexture;
				if (ImGui::Checkbox("Use Texture", &useTexture))
				{
					currentMaterialBuffer.Material.UseTexture = useTexture;
				}
			}
			if (m_selectedObject->GetNormalMapResourceView() != nullptr)
			{
				bool useNormalMap = currentMaterialBuffer.Material.UseNormalMap;
				if (ImGui::Checkbox("Use Normal Map", &useNormalMap))
				{
					currentMaterialBuffer.Material.UseNormalMap = useNormalMap;
				}
			}
			ImGui::Separator();
		}
		else
		{
			float ambient[3] = { currentMaterialBuffer.Material.Ambient.x, currentMaterialBuffer.Material.Ambient.y, currentMaterialBuffer.Material.Ambient.z };
			if (ImGui::ColorEdit3("Ambient Color", ambient))
			{
				currentMaterialBuffer.Material.Ambient = XMFLOAT4(ambient[0], ambient[1], ambient[2], 1.0f);
			}

			ImGui::Separator();
			if (m_selectedObject->GetTextureResourceView() != nullptr)
			{
				bool useTexture = currentMaterialBuffer.Material.UseTexture;
				if (ImGui::Checkbox("Use Texture", &useTexture))
				{
					currentMaterialBuffer.Material.UseTexture = useTexture;
				}
			}

			ImGui::Separator();
		}
		m_selectedObject->UpdateMaterialConstantBuffer(currentMaterialBuffer, pContext);

		if (ImGui::Button("Reset Material Values"))
		{
			m_selectedObject->UpdateMaterialConstantBuffer(m_selectedObject->GetOriginalMaterialConstantBufferData(), pContext);
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
		ImGui::SetNextWindowPos(ImVec2(10, 150), ImGuiCond_FirstUseEver);
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

void ImGuiRendering::DrawTextureSelectionWindow(ID3D11DeviceContext* pContext)
{
	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 295), ImGuiCond_FirstUseEver);
		ImGui::Begin("Texture Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Select a Texture for Each Object!");
		ImGui::Separator();
		ImGui::Text("Selected Object: %s", m_selectedObject->GetObjectName().c_str());
		ImGui::Separator();

		auto& textures = m_currentScene->m_textureMap;

		bool isNullSelected = (m_selectedObject->GetTextureResourceView() == nullptr);

		if (ImGui::Selectable("Nothing", isNullSelected))
		{
			MaterialPropertiesConstantBuffer buffer = m_selectedObject->GetMaterialConstantBufferData();
			buffer.Material.UseTexture = false;
			m_selectedObject->UpdateMaterialConstantBuffer(buffer, pContext);
			m_selectedObject->SetTextureResourceView(nullptr);
		}

		for (size_t i = 0; i < textures.size(); ++i)
		{
			const auto& shaderPair = textures[i];

			bool isSelected = (m_selectedObject->GetTextureResourceView() == shaderPair.second.Get());

			if (ImGui::Selectable(shaderPair.first.c_str(), isSelected))
			{
				MaterialPropertiesConstantBuffer buffer = m_selectedObject->GetMaterialConstantBufferData();
				buffer.Material.UseTexture = true;
				m_selectedObject->UpdateMaterialConstantBuffer(buffer, pContext);
				m_selectedObject->SetTextureResourceView(shaderPair.second);
			}
		}

		ImGui::End();
	}
}

void ImGuiRendering::DrawNormalMapSelectionWindow(ID3D11DeviceContext* pContext)
{
	if (m_selectedObject != nullptr)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 445), ImGuiCond_FirstUseEver);
		ImGui::Begin("Normal Map Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Select a Normal Map for Each Object!");
		ImGui::Separator();
		ImGui::Text("Selected Object: %s", m_selectedObject->GetObjectName().c_str());
		ImGui::Separator();

		auto& textures = m_currentScene->m_normalMapTextureMap;

		bool isNullSelected = (m_selectedObject->GetNormalMapResourceView() == nullptr);

		if (ImGui::Selectable("Nothing", isNullSelected))
		{
			MaterialPropertiesConstantBuffer buffer = m_selectedObject->GetMaterialConstantBufferData();
			buffer.Material.UseNormalMap = false;
			m_selectedObject->UpdateMaterialConstantBuffer(buffer, pContext);
			m_selectedObject->SetNormalMapResourceView(nullptr);
		}

		for (size_t i = 0; i < textures.size(); ++i)
		{
			const auto& shaderPair = textures[i];

			bool isSelected = (m_selectedObject->GetNormalMapResourceView() == shaderPair.second.Get());

			if (ImGui::Selectable(shaderPair.first.c_str(), isSelected))
			{
				MaterialPropertiesConstantBuffer buffer = m_selectedObject->GetMaterialConstantBufferData();
				buffer.Material.UseNormalMap = true;
				m_selectedObject->UpdateMaterialConstantBuffer(buffer, pContext);
				m_selectedObject->SetNormalMapResourceView(shaderPair.second);
			}
		}

		ImGui::End();
	}
}

void ImGuiRendering::DrawCameraStatsWindow()
{
	XMFLOAT3 cameraPosition = m_currentScene->GetCamera()->GetPosition();
	ImGui::SetNextWindowPos(ImVec2(300, 505), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Separator();
	ImGui::Text("Camera Position: X - %.3f", cameraPosition.x);
	ImGui::Text("Camera Position: Y - %.3f", cameraPosition.y);
	ImGui::Text("Camera Position: Z - %.3f", cameraPosition.z);
	ImGui::Separator();

	if (ImGui::DragFloat3("Camera Position", reinterpret_cast<float*>(&cameraPosition), 0.01f, -INFINITY, INFINITY))
	{
		m_currentScene->GetCamera()->SetPosition(cameraPosition);
	}

	ImGui::SliderFloat("Camera Move Speed", &m_currentScene->GetCamera()->m_cameraMoveSpeed, 0.5f, 4.0f);
	ImGui::Checkbox("Show Camera Spline Window", &showCameraSplineWindow);
	ImGui::Text("(Drag the box or enter a number)");
	ImGui::Separator();
	if (ImGui::Button("Reset Camera"))
	{
		m_currentScene->GetCamera()->Reset();
	}

	ImGui::End();
}

void ImGuiRendering::DrawCameraSplineWindow()
{
	ImGui::SetNextWindowPos(ImVec2(800, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera Spline Animation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Checkbox("Start / Stop Camera Animation", &m_currentScene->m_playCameraSplineAnimation);
	ImGui::Separator();
	ImGui::SliderFloat("Animation Duration", &m_currentScene->m_totalSplineAnimation, 1.0f, 10.0f);
	ImGui::SliderFloat("Current Animation Time", &m_currentScene->GetCamera()->m_splineTransition, 0.0f, 0.98f);
	ImGui::Separator();
	if (ImGui::Button("Add Point"))
	{
		if (m_currentScene->m_controlPoints.size() < 10) m_currentScene->m_controlPoints.push_back(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f));
	}

	if (ImGui::Button("Remove Point"))
	{
		if (m_currentScene->m_controlPoints.size() > 4) m_currentScene->m_controlPoints.erase(m_currentScene->m_controlPoints.end() - 1);
	}

	// Okay, IMGUI is pretty cool
	for (size_t i = 0; i < m_currentScene->m_controlPoints.size(); i++)
	{
		XMFLOAT3 point;
		XMStoreFloat3(&point, m_currentScene->m_controlPoints[i]);

		string pointName = "Spline Point " + to_string(i);

		if (i == 0) pointName = "Initial Velocity";

		if (i == m_currentScene->m_controlPoints.size() - 1) pointName = "Final Velocity";

		if (ImGui::DragFloat3(pointName.c_str(), reinterpret_cast<float*>(&point), 0.1f))
		{
			m_currentScene->m_controlPoints[i] = XMLoadFloat3(&point);
		}
	}
	ImGui::Text("(Drag the box or enter a number)");

	ImGui::End();
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