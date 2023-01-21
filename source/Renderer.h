#pragma once
#include "pch.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Render(Scene* pScene);

		ID3D11Device* GetDevice() const { return m_pDevice; }
		ID3D11DeviceContext* GetDeviceContext() const{ return m_pDeviceContext; }

	private:
		// Common
		SDL_Window* m_pWindow{};
		bool m_IsInitialized{ false };

		int m_Width{};
		int m_Height{};
		
		// Software
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		void RenderSoftware(std::vector<std::shared_ptr<Mesh>>& pMeshes, const Camera& camera, const RenderInfo& renderInfo);

		void ProjectMesh(std::shared_ptr<Mesh> mesh, const Camera& camera) const;
		void RasterizeMesh(Mesh& mesh, const RenderInfo& renderInfo) const;

		Mesh ClipMesh(Mesh& mesh);
		std::vector<Vector2> ClipTriangle(const std::vector<Vector2>& triVerts);

		ColorRGB ShadePixel(const Mesh& mesh, const Vertex_Out& vertex, const RenderInfo& renderInfo) const;

		// DirectX
		HRESULT InitializeDirectX();
		void RenderHardware(std::vector<std::shared_ptr<Mesh>>& pMeshes, const RenderInfo& renderInfo) const;

		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;
		IDXGISwapChain* m_pSwapChain;
		ID3D11Texture2D* m_pDepthStencilBuffer;
		ID3D11DepthStencilView* m_pDepthStencilView;
		ID3D11Texture2D* m_pRenderTargetBuffer;
		ID3D11RenderTargetView* m_pRenderTargetView;
	};
}
