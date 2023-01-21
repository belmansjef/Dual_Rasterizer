#pragma once
#include "Camera.h"

namespace dae
{
	class Scene
	{
	public:
		Scene() = default;
		virtual ~Scene() = default;

		Scene(const Scene& other) = delete;
		Scene(Scene&& other) noexcept = delete;
		Scene& operator=(const Scene& other) = delete;
		Scene& operator=(Scene&& other) noexcept = delete;

		virtual void Initialize(ID3D11Device* pDevice) = 0;
		virtual void ProccesKeyInput(SDL_Event& e);
		virtual void Update(const Timer* pTimer, ID3D11Device* pDevice)
		{
			m_Camera.Update(pTimer);
		};

		// Getters
		Camera GetCamera() const { return m_Camera; }
		RenderInfo GetRenderInfo() const { return m_RenderInfo; }
		std::vector<std::shared_ptr<Mesh>> GetMeshes() { return m_pMeshes; }

	protected:
		bool m_EffectUpdateRequired{ false };
		Camera m_Camera{};
		RenderInfo m_RenderInfo{};

		const ColorRGB m_UniformClear{ .1f, .1f, .1f };
		const ColorRGB m_SoftwareClear{ .39f, .39f, .39f };
		const ColorRGB m_HardwareClear{ .39f, .59f, .93f };

		std::vector<std::shared_ptr<Mesh>> m_pMeshes;
		std::vector<std::shared_ptr<Effect>> m_pEffects;

		HANDLE m_hConsole;

		void CycleRenderType();
		void ToggleRotation();
		void ToggleFireFX();
		void CycleFilteringMode();
		void CycleRenderMode();
		void ToggleNormalMap();
		void ToggleDepthBufferVisual();
		void ToggleBBVisual();
		void CycleCullMode();
		void ToggleUniformClear();
		void ToggleFPS();
		void ToggleClipping();
	};

	class ReferenceScene final : public Scene
	{
	public:
		ReferenceScene() = default;
		~ReferenceScene() = default;

		ReferenceScene(const ReferenceScene& other) = delete;
		ReferenceScene(ReferenceScene&& other) noexcept = delete;
		ReferenceScene& operator=(const ReferenceScene& other) = delete;
		ReferenceScene& operator=(ReferenceScene&& other) noexcept = delete;

		void Initialize(ID3D11Device* pDevice) override;
		void Update(const Timer* pTimer, ID3D11Device* pDevice) override;


	private:
		// Vehicle
		std::shared_ptr<Mesh> m_pVehicleMesh;

		// Effect
		std::shared_ptr<EffectPosTex> m_pVehicleEffect;
		std::shared_ptr<Texture> m_pVehicleDiffuse;
		std::shared_ptr<Texture> m_pVehicleNormal;
		std::shared_ptr<Texture> m_pVehicleSpecular;
		std::shared_ptr<Texture> m_pVehicleGloss;

		// FireFX
		std::shared_ptr<Mesh> m_pFireMesh;

		// Effect
		std::shared_ptr<EffectTransparent> m_pFireEffect;
		std::shared_ptr<Texture> m_pFireDiffuse;
	};
}

