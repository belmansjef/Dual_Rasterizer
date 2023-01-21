#include "pch.h"
#include "Texture.h"
#include "Vector2.h"
#include "Vector3.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(ID3D11Device* pDevice, const std::string& path)
	{
		m_pSurface = LoadFromFile(path);
		m_pSurfacePixels = (uint32_t*)m_pSurface->pixels;
		
		// Load Texture
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_pSurface->w;
		desc.Height = m_pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA init_data;
		init_data.pSysMem = m_pSurfacePixels;
		init_data.SysMemPitch = static_cast<UINT>(m_pSurface->pitch);
		init_data.SysMemSlicePitch = static_cast<UINT>(m_pSurface->h * m_pSurface->pitch);

		HRESULT hr = pDevice->CreateTexture2D(&desc, &init_data, &m_pResource);
		if (!SUCCEEDED(hr))
			assert(-1);

		// Load Shader Resource View
		D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc{};
		SRV_desc.Format = format;
		SRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRV_desc.Texture2D.MipLevels = 1;

		hr = pDevice->CreateShaderResourceView(m_pResource, &SRV_desc, &m_pSRV);
		if (!SUCCEEDED(hr))
			assert(-1);
	}

	Texture::~Texture()
	{
		m_pSRV->Release();
		m_pResource->Release();

		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	ColorRGB Texture::SampleColor(const Vector2& uv) const
	{
		const Uint32 px{ static_cast<Uint32>(std::clamp(uv.x, 0.f, 1.f) * m_pSurface->w) };
		const Uint32 py{ static_cast<Uint32>(std::clamp(uv.y, 0.f, 1.f) * m_pSurface->h) };
		const Uint32 pixelIndex{ static_cast<Uint32>(px + (py * m_pSurface->w)) };

		Uint8 r, g, b;
		SDL_GetRGB(m_pSurfacePixels[pixelIndex], m_pSurface->format, &r, &g, &b);

		const float invResize = 1 / 255.f;
		return ColorRGB{ r * invResize , g * invResize, b * invResize };
	}

	Vector4 Texture::SampleRGBA(const Vector2& uv) const
	{
		const Uint32 px{ static_cast<Uint32>(std::clamp(uv.x, 0.f, 1.f) * m_pSurface->w) };
		const Uint32 py{ static_cast<Uint32>(std::clamp(uv.y, 0.f, 1.f) * m_pSurface->h) };
		const Uint32 pixelIndex{ static_cast<Uint32>(px + (py * m_pSurface->w)) };

		Uint8 r, g, b, a;
		SDL_GetRGBA(m_pSurfacePixels[pixelIndex], m_pSurface->format, &r, &g, &b, &a);

		const float invResize = 1 / 255.f;
		return Vector4{ r * invResize , g * invResize, b * invResize, a * invResize };
	}

	Vector3 Texture::SampleNormal(const Vector2& uv) const
	{
		const Uint32 px{ static_cast<Uint32>(std::clamp(uv.x, 0.f, 1.f) * m_pSurface->w) };
		const Uint32 py{ static_cast<Uint32>(std::clamp(uv.y, 0.f, 1.f) * m_pSurface->h) };
		const Uint32 pixelIndex{ static_cast<Uint32>(px + (py * m_pSurface->w)) };

		Uint8 r, g, b;
		SDL_GetRGB(m_pSurfacePixels[pixelIndex], m_pSurface->format, &r, &g, &b);

		const float invResize = 1 / 255.f;
		return Vector3{ r * invResize , g * invResize, b * invResize };
	}

	ID3D11ShaderResourceView* Texture::GetSRV() const
	{
		return m_pSRV;
	}

	SDL_Surface* Texture::LoadFromFile(const std::string& path)
	{
		return IMG_Load(path.c_str());
	}
}