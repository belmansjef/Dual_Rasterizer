#pragma once
#include <string>

namespace dae
{
	class Texture final
	{
	public:
		Texture(ID3D11Device* pDevice, const std::string& path);
		~Texture();

		ColorRGB SampleColor(const Vector2& uv) const;
		Vector3 SampleNormal(const Vector2& uv) const;

		ID3D11ShaderResourceView* GetSRV() const;
	private:
		SDL_Surface* LoadFromFile(const std::string& path);

		ID3D11Texture2D* m_pResource;
		ID3D11ShaderResourceView* m_pSRV;

		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
	};
}