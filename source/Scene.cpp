#include "pch.h"
#include "Scene.h"
#include "Utils.h"
#include "Renderer.h"
#include <Windows.h>

void dae::Scene::ProccesKeyInput(SDL_Event& e)
{
	switch (e.key.keysym.scancode)
	{
	case SDL_SCANCODE_F1:
		CycleRenderType();
		break;
	case SDL_SCANCODE_F2:
		ToggleRotation();
		break;
	case SDL_SCANCODE_F3:
		ToggleFireFX();
		break;
	case SDL_SCANCODE_F4:
		CycleFilteringMode();
		break;
	case SDL_SCANCODE_F5:
		CycleRenderMode();
		break;
	case SDL_SCANCODE_F6:
		ToggleNormalMap();
		break;
	case SDL_SCANCODE_F7:
		ToggleDepthBufferVisual();
		break;
	case SDL_SCANCODE_F8:
		ToggleBBVisual();
		break;
	case SDL_SCANCODE_F9:
		CycleCullMode();
		break;
	case SDL_SCANCODE_F10:
		ToggleUniformClear();
		break;
	case SDL_SCANCODE_F11:
		ToggleFPS();
		break;
	case SDL_SCANCODE_C:
		ToggleClipping();
		break;
	}
}

void dae::Scene::CycleRenderType()
{
	m_RenderInfo.renderType = static_cast<RenderType>((static_cast<int>(m_RenderInfo.renderType) + 1) % static_cast<int>(RenderType::SIZE));

	SetConsoleTextAttribute(m_hConsole, 6);
	switch (m_RenderInfo.renderType)
	{
	case RenderType::Software:
		std::cout << "[RASTERIZER MODE] Software\n";
		break;
	case RenderType::Hardware:
		std::cout << "[RASTERIZER MODE] Hardware\n";
		break;
	}
}

void dae::Scene::ToggleRotation()
{
	m_RenderInfo.doRotate = !m_RenderInfo.doRotate;

	SetConsoleTextAttribute(m_hConsole, 6);
	std::cout << "[VEHICLE ROTATION] ";
	m_RenderInfo.doRotate ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::ToggleFireFX()
{
	if (m_RenderInfo.renderType != RenderType::Hardware) return;
	m_RenderInfo.renderFireFX = !m_RenderInfo.renderFireFX;

	SetConsoleTextAttribute(m_hConsole, 2);
	std::cout << "[FIRE FX] ";
	m_RenderInfo.renderFireFX ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::ToggleNormalMap()
{
	if (m_RenderInfo.renderType != RenderType::Software) return;
	m_RenderInfo.useNormalMap = !m_RenderInfo.useNormalMap;

	SetConsoleTextAttribute(m_hConsole, 5);
	std::cout << "[NORMAL MAP] ";
	m_RenderInfo.useNormalMap ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::ToggleDepthBufferVisual()
{
	if (m_RenderInfo.renderType != RenderType::Software) return;
	m_RenderInfo.visualizeDepthBuffer = !m_RenderInfo.visualizeDepthBuffer;

	SetConsoleTextAttribute(m_hConsole, 5);
	std::cout << "[DEPTHBUFFER VISUALIZATION] ";
	m_RenderInfo.visualizeDepthBuffer ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::ToggleBBVisual()
{
	if (m_RenderInfo.renderType != RenderType::Software) return;
	m_RenderInfo.visualizeBoundingBox = !m_RenderInfo.visualizeBoundingBox;

	SetConsoleTextAttribute(m_hConsole, 5);
	std::cout << "[BOUNDINGBOX VISUALIZATION] ";
	m_RenderInfo.visualizeBoundingBox ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::CycleCullMode()
{
	auto currCullMode{ m_pMeshes[0]->GetCullMode() };
	currCullMode = static_cast<CullMode>((static_cast<int>(currCullMode) + 1) % static_cast<int>(CullMode::SIZE));
	m_pMeshes[0]->SetCullMode(currCullMode);

	SetConsoleTextAttribute(m_hConsole, 6);

	switch (currCullMode)
	{
	case dae::CullMode::BackFace:
		std::cout << "[CULL MODE] Back\n";
		break;
	case dae::CullMode::FrontFace:
		std::cout << "[CULL MODE] Front\n";
		break;
	case dae::CullMode::None:
		std::cout << "[CULL MODE] None\n";
		break;
	}
}

void dae::Scene::ToggleUniformClear()
{
	m_RenderInfo.useUniformClear = !m_RenderInfo.useUniformClear;

	SetConsoleTextAttribute(m_hConsole, 6);
	std::cout << "[UNIFORM CLEARCOLOR] ";
	m_RenderInfo.useUniformClear ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::ToggleFPS()
{
	m_RenderInfo.showFPS = !m_RenderInfo.showFPS;
	
	SetConsoleTextAttribute(m_hConsole, 6);
	std::cout << "[SHOW FPS] ";
	m_RenderInfo.showFPS ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::ToggleClipping()
{
	if (m_RenderInfo.renderType != RenderType::Software) return;
	m_RenderInfo.useClipping = !m_RenderInfo.useClipping;

	SetConsoleTextAttribute(m_hConsole, 5);
	std::cout << "[TRIANGLE CLIPPING] ";
	m_RenderInfo.useClipping ? std::cout << "ON\n" : std::cout << "OFF\n";
}

void dae::Scene::CycleFilteringMode()
{
	if (m_RenderInfo.renderType != RenderType::Hardware) return;
	m_RenderInfo.textureFiltering = static_cast<FilteringMode>((static_cast<int>(m_RenderInfo.textureFiltering) + 1) % static_cast<int>(FilteringMode::SIZE));

	SetConsoleTextAttribute(m_hConsole, 2);
	switch (m_RenderInfo.textureFiltering)
	{
	case dae::FilteringMode::Point:
		std::cout << "[TEXTURE SAMPLING] Point\n";
		break;
	case dae::FilteringMode::Linear:
		std::cout << "[TEXTURE SAMPLING] Linear\n";
		break;
	case dae::FilteringMode::Anisotropic:
		std::cout << "[TEXTURE SAMPLING] Anisotropic\n";
		break;
	default:
		break;
	}
}

void dae::Scene::CycleRenderMode()
{
	if (m_RenderInfo.renderType != RenderType::Software) return;
	m_RenderInfo.shadingMode = static_cast<ShadingMode>((static_cast<int>(m_RenderInfo.shadingMode) + 1) % static_cast<int>(ShadingMode::SIZE));
	
	SetConsoleTextAttribute(m_hConsole, 5);
	switch (m_RenderInfo.shadingMode)
	{
	case ShadingMode::FinalColor:
		std::cout << "[SHADING MODE] FinalColor\n";
		break;
	case ShadingMode::ObservedArea:
		std::cout << "[SHADING MODE] ObservedArea\n";
		break;
	case ShadingMode::Diffuse:
		std::cout << "[SHADING MODE] Diffuse\n";
		break;
	case ShadingMode::Specular:
		std::cout << "[SHADING MODE] Specular\n";
		break;
	}
}

void dae::ReferenceScene::Initialize(ID3D11Device* pDevice)
{
	m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(m_hConsole, 9);
	std::cout
		<< "-----------------------------------------------\n"
		<< "|| Dual Rasterizer || Belmans Jef || 2DAE15N ||\n"
		<< "-----------------------------------------------\n\n";

	SetConsoleTextAttribute(m_hConsole, 6);
	std::cout	
		<< "[Key Bindings - SHARED]\n"
		<< "  [F1] Toggle Rasterizer Mode (HARDWARE/SOFTWARE)\n"
		<< "  [F2] Toggle Vehicle Rotation (ON/OFF)\n"
		<< "  [F9] Cycle CullMode (BACK/FRONT/NONE)\n"
		<< "  [F10] Toggle Uniform ClearColor (ON/OFF)\n"
		<< "  [F11] Toggle Print FPS (ON/OFF)\n"
		<< std::endl;

	SetConsoleTextAttribute(m_hConsole, 2);
	std::cout
		<< "[Key Bindings - HARDWARE]\n"
		<< "  [F3] Toggle FireFX (ON/OFF)\n"
		<< "  [F4] Cycle Sampler State (POINT/LINEAR/ANISOTROPIC)\n"
		<< std::endl;

	SetConsoleTextAttribute(m_hConsole, 5);
	std::cout
		<< "[Key Bindings - SOFTWARE]\n"
		<< "  [F5] Cycle Shading Mode (COMBINED/OBSERVED_AREA/DIFFUSE/SPECULAR)\n"
		<< "  [F6] Toggle NormalMap (ON/OFF)\n"
		<< "  [F7] Toggle DepthBuffer Visualization (ON/OFF)\n"
		<< "  [F8] Toggle BoundingBox Visualization (ON/OFF)\n";
	SetConsoleTextAttribute(m_hConsole, 13);
	std::cout
		<< "  [C] (EXTRA) Toggle Triangle Clipping (ON/OFF)\n"
		<< std::endl;


	m_Camera.Initialize(45.f, { 0.f, 0.f, 0.f });

	std::vector<Vertex_In> vertices{};
	std::vector<uint32_t> indices{};

	// Vehicle Textures
	m_pVehicleDiffuse	= std::make_shared<Texture>(pDevice, "Resources/vehicle_diffuse.png");
	m_pVehicleNormal	= std::make_shared<Texture>(pDevice, "Resources/vehicle_normal.png");
	m_pVehicleSpecular	= std::make_shared<Texture>(pDevice, "Resources/vehicle_specular.png");
	m_pVehicleGloss		= std::make_shared<Texture>(pDevice, "Resources/vehicle_gloss.png");

	// FireFX Textures
	m_pFireDiffuse		= std::make_shared<Texture>(pDevice, "Resources/fireFX_diffuse.png");

	// Vehicle Mesh
	Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices);	
	m_pVehicleMesh = std::make_unique<Mesh>(pDevice, vertices, indices);
	m_pVehicleMesh->SetDiffuseMap(m_pVehicleDiffuse);
	m_pVehicleMesh->SetNormalMap(m_pVehicleNormal);
	m_pVehicleMesh->SetSpecularMap(m_pVehicleSpecular);
	m_pVehicleMesh->SetGlossMap(m_pVehicleGloss);

	// FireFX Mesh
	Utils::ParseOBJ("Resources/fireFX.obj", vertices, indices);
	m_pFireMesh = std::make_unique<Mesh>(pDevice, vertices, indices);
	m_pFireMesh->SetDiffuseMap(m_pFireDiffuse);

	// Vehicle Effect
	m_pVehicleEffect = std::make_shared<EffectPosTex>(pDevice, L"Resources/PosTex3D.fx");
	m_pVehicleEffect->SetDiffuseMap(m_pVehicleDiffuse.get());
	m_pVehicleEffect->SetNormalMap(m_pVehicleNormal.get());
	m_pVehicleEffect->SetSpecularMap(m_pVehicleSpecular.get());
	m_pVehicleEffect->SetGlossMap(m_pVehicleGloss.get());
	m_pVehicleMesh->SetEffect(m_pVehicleEffect);

	// FireFX Effect
	m_pFireEffect = std::make_shared<EffectTransparent>(pDevice, L"Resources/PosTexTransparent3D.fx");
	m_pFireEffect->SetDiffuseMap(m_pFireDiffuse.get());
	m_pFireMesh->SetEffect(m_pFireEffect);

	// Move the meshes back to fit inside the viewport
	m_pVehicleMesh->Translate({ 0.f, 0.f, 50.f });
	m_pFireMesh->Translate({ 0.f, 0.f, 50.f });

	// Add meshes to collection
	m_pMeshes.push_back(m_pVehicleMesh);
	m_pMeshes.push_back(m_pFireMesh);

	// Add effects to collection
	m_pEffects.push_back(m_pVehicleEffect);
	m_pEffects.push_back(m_pFireEffect);
}

void dae::ReferenceScene::Update(const Timer* pTimer, ID3D11Device* pDevice)
{
	// Call base class update
	Scene::Update(pTimer, pDevice);
	
	// Disable FireMesh in software renderer
	m_pFireMesh->SetIsEnabled(m_RenderInfo.renderType != RenderType::Software && m_RenderInfo.renderFireFX);

	// Set clear color
	if (m_RenderInfo.useUniformClear) m_RenderInfo.clearColor = m_UniformClear;
	else if (m_RenderInfo.renderType == RenderType::Hardware)  m_RenderInfo.clearColor = m_HardwareClear;
	else  m_RenderInfo.clearColor = m_SoftwareClear;

	// Update vehicle CullMode
	m_pVehicleEffect->SetCullMode(m_pVehicleMesh->GetCullMode());

	// Update all effects
	std::for_each(begin(m_pEffects), end(m_pEffects), [&](std::shared_ptr<Effect>& effect)
		{
			effect->SetTextureFiltering(m_RenderInfo.textureFiltering);
		}
	);

	// Update all meshes
	std::for_each(begin(m_pMeshes), end(m_pMeshes), [&](std::shared_ptr<Mesh>& mesh)
		{
			if(m_RenderInfo.doRotate) mesh->RotateY((45.f * TO_RADIANS) * pTimer->GetElapsed());
			mesh->Update(m_Camera.viewMatrix, m_Camera.projectionMatrix);
		}
	);
}