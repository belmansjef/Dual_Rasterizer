#include "pch.h"
#include "Renderer.h"
#include "Utils.h"
#include "Scene.h"
#include <ppl.h>

#define USE_CONCURENCY

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		//Initialize DirectX pipeline
		if (SUCCEEDED(InitializeDirectX()))
		{
			m_IsInitialized = true;
			// std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}
	}

	Renderer::~Renderer()
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetBuffer->Release();
		m_pDepthStencilView->Release();
		m_pDepthStencilBuffer->Release();
		m_pSwapChain->Release();

		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}
		m_pDevice->Release();

		delete[] m_pDepthBufferPixels;
	}

	void Renderer::Render(Scene* pScene)
	{
		auto pMeshes{ pScene->GetMeshes() };
		switch (pScene->GetRenderInfo().renderType)
		{
			case RenderType::Hardware:
				RenderHardware(pMeshes, pScene->GetRenderInfo());
				break;

			case RenderType::Software:
				RenderSoftware(pMeshes, pScene->GetCamera(), pScene->GetRenderInfo());
				break;

			default:
				std::cout << "ERROR getting 'RenderType' in render loop!\n";
				break;
		}
	}

#pragma region Software
	void Renderer::RenderSoftware(std::vector<std::shared_ptr<Mesh>>& pMeshes, const Camera& camera, const RenderInfo& renderInfo)
	{
		//@START
		//Lock BackBuffer
		SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, (Uint8) (renderInfo.clearColor.r * 255), (Uint8) (renderInfo.clearColor.g * 255), (Uint8) (renderInfo.clearColor.b * 255)));
		SDL_LockSurface(m_pBackBuffer);

		// Initialize depth buffer with max value
		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, 1.f);

		for (auto mesh : pMeshes)
		{
			if (!mesh->IsEnabled()) continue;

			ProjectMesh(mesh, camera);
			if (renderInfo.useClipping)
			{
				Mesh clippedMesh{ ClipMesh(*mesh) };
				RasterizeMesh(clippedMesh, renderInfo);
			}
			else
				RasterizeMesh(*mesh, renderInfo);
		}

		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::ProjectMesh(std::shared_ptr<Mesh> mesh, const Camera& camera) const
	{
		auto worldMatrix{ mesh->GetWorldMatrix() };
		auto verticesIn{ mesh->GetVerticesIn() };
		auto& verticesOut{ mesh->GetVerticesOut() };

		Matrix wvp{ worldMatrix * camera.viewMatrix * camera.projectionMatrix };
		Matrix rotMatrix{ worldMatrix.GetAxisX(), worldMatrix.GetAxisY(), worldMatrix.GetAxisZ(), {0.f,0.f,0.f} };

		verticesOut.clear();
		verticesOut.reserve(verticesIn.size());
		for (int i = 0; i < verticesIn.size(); i++)
		{
			Vertex_Out vert_out{ {}, {}, verticesIn[i].uv, verticesIn[i].normal, verticesIn[i].tangent, {} };
			vert_out.normal = rotMatrix.TransformVector(verticesIn[i].normal);
			vert_out.tangent = rotMatrix.TransformVector(verticesIn[i].tangent);

			vert_out.viewDirection = worldMatrix.TransformPoint(vert_out.position) - camera.invViewMatrix[3];

			// WORLD to NDC
			vert_out.position = wvp.TransformPoint({ verticesIn[i].position, 1.f });

			// Perspective divide
			const float invDepth = 1.f / vert_out.position.w;
			vert_out.position.x *= invDepth;
			vert_out.position.y *= invDepth;
			vert_out.position.z *= invDepth;

			verticesOut.push_back(vert_out);
		}
	}

	void Renderer::RasterizeMesh(Mesh& mesh, const RenderInfo& renderInfo) const
	{
		auto indices{ mesh.GetIndices() };
		auto verticesOut{ mesh.GetVerticesOut() };
		auto primitiveTopology{ mesh.GetPrimitiveTopology() };
		auto cullMode{ mesh.GetCullMode() };

		// Return if mesh is empty (outside viewport)
		if (indices.empty()) return;

		const uint32_t numTriangles{ static_cast<uint32_t>(indices.size() / 3.f) };

		if (renderInfo.useMultiThreading && mesh.GetEffect()->GetEffectType() == EffectType::Diffuse)
		{
			concurrency::parallel_for(0U, numTriangles,
				[&](int triangle)
				{
					const int i{ triangle * 3 };
					const int i0 = indices[i];
					int i1 = indices[i + 1];
					int i2 = indices[i + 2];

					if (i0 == i1 || i1 == i2) return;

					// swap index1 and index2 for odd triangles (ccw to cw)
					if ((i & 1) == 1 && primitiveTopology == PrimitiveTopology::TriangleStrip)
					{
						std::swap(i1, i2);
					}

					// CULLING
					if (
						verticesOut[i0].position.x < -1 || verticesOut[i0].position.x > 1 ||
						verticesOut[i0].position.y < -1 || verticesOut[i0].position.y > 1 ||
						verticesOut[i0].position.z < 0 || verticesOut[i0].position.z > 1 ||
						verticesOut[i1].position.x < -1 || verticesOut[i1].position.x > 1 ||
						verticesOut[i1].position.y < -1 || verticesOut[i1].position.y > 1 ||
						verticesOut[i1].position.z < 0 || verticesOut[i1].position.z > 1 ||
						verticesOut[i2].position.x < -1 || verticesOut[i2].position.x > 1 ||
						verticesOut[i2].position.y < -1 || verticesOut[i2].position.y > 1 ||
						verticesOut[i2].position.z < 0 || verticesOut[i2].position.z > 1
						&& renderInfo.useFastCulling) return;

					// PROJECTION to SS / RASTER
					const Vector2 v0{ (verticesOut[i0].position.x + 1) * 0.5f * m_Width, (1 - verticesOut[i0].position.y) * 0.5f * m_Height };
					const Vector2 v1{ (verticesOut[i1].position.x + 1) * 0.5f * m_Width, (1 - verticesOut[i1].position.y) * 0.5f * m_Height };
					const Vector2 v2{ (verticesOut[i2].position.x + 1) * 0.5f * m_Width, (1 - verticesOut[i2].position.y) * 0.5f * m_Height };

					const Vector2 v0v1 = v1 - v0;
					const Vector2 v0v2 = v2 - v0;
					const Vector2 v1v2 = v2 - v1;
					const Vector2 v2v0 = v0 - v2;

					const float invTriArea = 1.f / Vector2::Cross(v0v1, v0v2);

					Bounds aabb;
					aabb.min.x = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::min(v0.x, std::min(v1.x, v2.x)))), 0, m_Width - 1));
					aabb.min.y = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::min(v0.y, std::min(v1.y, v2.y)))), 0, m_Height - 1));
					aabb.max.x = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::max(v0.x, std::max(v1.x, v2.x)))), 0, m_Width - 1));
					aabb.max.y = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::max(v0.y, std::max(v1.y, v2.y)))), 0, m_Height - 1));

					// SHADING LOGIC
					for (uint32_t px{ static_cast<uint32_t>(aabb.min.x) }; px < aabb.max.x; ++px)
					{
						for (uint32_t py{ static_cast<uint32_t>(aabb.min.y) }; py < aabb.max.y; ++py)
						{
							if (renderInfo.visualizeBoundingBox)
							{
								ColorRGB finalColor = { 1, 1, 1 };

								//Update Color in Buffer
								finalColor.MaxToOne();

								m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
									static_cast<uint8_t>(finalColor.r * 255),
									static_cast<uint8_t>(finalColor.g * 255),
									static_cast<uint8_t>(finalColor.b * 255));

								continue;
							}

							const Vector2 pixelPos{ static_cast<float>(px), static_cast<float>(py) };

							const Vector2 v1p = pixelPos - v1;
							float w0 = Vector2::Cross(v1v2, v1p);

							const Vector2 v2p = pixelPos - v2;
							float w1 = Vector2::Cross(v2v0, v2p);

							const Vector2 v0p = pixelPos - v0;
							float w2 = Vector2::Cross(v0v1, v0p);

							const bool isFrontFace{ w0 >= 0.f && w1 >= 0.f && w2 >= 0.f };
							const bool isBackFace{ w0 < 0.f && w1 < 0.f && w2 < 0.f };
							if (isFrontFace && cullMode == CullMode::FrontFace) continue;
							if (isBackFace && cullMode == CullMode::BackFace) continue;
							if (!isFrontFace && !isBackFace) continue;

							w0 *= invTriArea;
							w1 *= invTriArea;
							w2 *= invTriArea;

							// Depth test
							const float zBufferValue
							{
								1.f / (
									((1.f / verticesOut[i0].position.z) * w0) +
									((1.f / verticesOut[i1].position.z) * w1) +
									((1.f / verticesOut[i2].position.z) * w2)
									)
							};
							if (zBufferValue >= m_pDepthBufferPixels[px + (py * m_Width)]) continue;
							if(mesh.GetEffect()->GetEffectType() == EffectType::Diffuse) m_pDepthBufferPixels[px + (py * m_Width)] = zBufferValue;

							ColorRGB finalColor{};
							if (renderInfo.visualizeDepthBuffer)
							{
								const float remappedDepth{ Remap(zBufferValue, 0.995f, 1.f) };
								finalColor = { remappedDepth, remappedDepth, remappedDepth };

								//Update Color in Buffer
								finalColor.MaxToOne();

								m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
									static_cast<uint8_t>(finalColor.r * 255),
									static_cast<uint8_t>(finalColor.g * 255),
									static_cast<uint8_t>(finalColor.b * 255));
								continue;
							}

							Vertex_Out pixelVertex = Vertex_Out::Interpolate({ verticesOut[i0], verticesOut[i1], verticesOut[i2] }, w0, w1, w2);

							uint8_t r, g, b;
							SDL_GetRGB(m_pBackBufferPixels[(px + (py * m_Width))], m_pBackBuffer->format, &r, &g, &b);
							finalColor = ShadePixel(mesh, pixelVertex, renderInfo, ColorRGB{ static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f });

							//Update Color in Buffer
							finalColor.MaxToOne();

							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(finalColor.r * 255),
								static_cast<uint8_t>(finalColor.g * 255),
								static_cast<uint8_t>(finalColor.b * 255));
						}
					}
				});
		}
		else
		{
			for (int i = 0; i < indices.size() - 2; primitiveTopology == PrimitiveTopology::TriangleList ? i += 3 : i++)
			{
				const int i0 = indices[i];
				int i1 = indices[i + 1];
				int i2 = indices[i + 2];

				if (i0 == i1 || i1 == i2) continue;

				// swap index1 and index2 for odd triangles (ccw to cw)
				if ((i & 1) == 1 && primitiveTopology == PrimitiveTopology::TriangleStrip)
				{
					std::swap(i1, i2);
				}

				// CULLING
				if (
					verticesOut[i0].position.x < -1 || verticesOut[i0].position.x > 1 ||
					verticesOut[i0].position.y < -1 || verticesOut[i0].position.y > 1 ||
					verticesOut[i0].position.z < 0 || verticesOut[i0].position.z > 1 ||
					verticesOut[i1].position.x < -1 || verticesOut[i1].position.x > 1 ||
					verticesOut[i1].position.y < -1 || verticesOut[i1].position.y > 1 ||
					verticesOut[i1].position.z < 0 || verticesOut[i1].position.z > 1 ||
					verticesOut[i2].position.x < -1 || verticesOut[i2].position.x > 1 ||
					verticesOut[i2].position.y < -1 || verticesOut[i2].position.y > 1 ||
					verticesOut[i2].position.z < 0 || verticesOut[i2].position.z > 1
					&& renderInfo.useFastCulling) continue;

				// PROJECTION to SS / RASTER
				const Vector2 v0{ (verticesOut[i0].position.x + 1) * 0.5f * m_Width, (1 - verticesOut[i0].position.y) * 0.5f * m_Height };
				const Vector2 v1{ (verticesOut[i1].position.x + 1) * 0.5f * m_Width, (1 - verticesOut[i1].position.y) * 0.5f * m_Height };
				const Vector2 v2{ (verticesOut[i2].position.x + 1) * 0.5f * m_Width, (1 - verticesOut[i2].position.y) * 0.5f * m_Height };

				const Vector2 v0v1 = v1 - v0;
				const Vector2 v0v2 = v2 - v0;
				const Vector2 v1v2 = v2 - v1;
				const Vector2 v2v0 = v0 - v2;

				const float invTriArea = 1.f / Vector2::Cross(v0v1, v0v2);

				Bounds aabb;
				aabb.min.x = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::min(v0.x, std::min(v1.x, v2.x)))), 0, m_Width - 1));
				aabb.min.y = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::min(v0.y, std::min(v1.y, v2.y)))), 0, m_Height - 1));
				aabb.max.x = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::max(v0.x, std::max(v1.x, v2.x)))), 0, m_Width - 1));
				aabb.max.y = static_cast<float>(std::clamp(static_cast<int>(std::ceilf(std::max(v0.y, std::max(v1.y, v2.y)))), 0, m_Height - 1));

				// SHADING LOGIC
				for (uint32_t px{ static_cast<uint32_t>(aabb.min.x) }; px < aabb.max.x; ++px)
				{
					for (uint32_t py{ static_cast<uint32_t>(aabb.min.y) }; py < aabb.max.y; ++py)
					{
						if (renderInfo.visualizeBoundingBox)
						{
							ColorRGB finalColor = { 1, 1, 1 };

							//Update Color in Buffer
							finalColor.MaxToOne();

							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(finalColor.r * 255),
								static_cast<uint8_t>(finalColor.g * 255),
								static_cast<uint8_t>(finalColor.b * 255));

							continue;
						}

						const Vector2 pixelPos{ static_cast<float>(px), static_cast<float>(py) };

						const Vector2 v1p = pixelPos - v1;
						float w0 = Vector2::Cross(v1v2, v1p);

						const Vector2 v2p = pixelPos - v2;
						float w1 = Vector2::Cross(v2v0, v2p);

						const Vector2 v0p = pixelPos - v0;
						float w2 = Vector2::Cross(v0v1, v0p);

						const bool isFrontFace{ w0 >= 0.f && w1 >= 0.f && w2 >= 0.f };
						const bool isBackFace{ w0 < 0.f && w1 < 0.f && w2 < 0.f };
						if (isFrontFace && cullMode == CullMode::FrontFace) continue;
						if (isBackFace && cullMode == CullMode::BackFace) continue;
						if (!isFrontFace && !isBackFace) continue;

						w0 *= invTriArea;
						w1 *= invTriArea;
						w2 *= invTriArea;

						// Depth test
						const float zBufferValue
						{
							1.f / (
								((1.f / verticesOut[i0].position.z) * w0) +
								((1.f / verticesOut[i1].position.z) * w1) +
								((1.f / verticesOut[i2].position.z) * w2)
								)
						};
						if (zBufferValue >= m_pDepthBufferPixels[px + (py * m_Width)]) continue;
						if (mesh.GetEffect()->GetEffectType() == EffectType::Diffuse) m_pDepthBufferPixels[px + (py * m_Width)] = zBufferValue;

						ColorRGB finalColor{};
						if (renderInfo.visualizeDepthBuffer)
						{
							const float remappedDepth{ Remap(zBufferValue, 0.995f, 1.f) };
							finalColor = { remappedDepth, remappedDepth, remappedDepth };

							//Update Color in Buffer
							finalColor.MaxToOne();

							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(finalColor.r * 255),
								static_cast<uint8_t>(finalColor.g * 255),
								static_cast<uint8_t>(finalColor.b * 255));
							continue;
						}

						Vertex_Out pixelVertex = Vertex_Out::Interpolate({ verticesOut[i0], verticesOut[i1], verticesOut[i2] }, w0, w1, w2);

						uint8_t r, g, b;
						SDL_GetRGB(m_pBackBufferPixels[(px + (py * m_Width))], m_pBackBuffer->format, &r, &g, &b);
						finalColor = ShadePixel(mesh, pixelVertex, renderInfo, ColorRGB{static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f });

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}

	Mesh dae::Renderer::ClipMesh(Mesh& mesh)
	{
		auto indices{ mesh.GetIndices() };
		auto verticesOut{ mesh.GetVerticesOut() };

		std::vector<Vertex_Out> newVerticesOut;
		std::vector<uint32_t> newIndices;

		for (int i = 0; i < indices.size(); i += 3)
		{
			const int i0 = indices[i + 0];
			const int i1 = indices[i + 1];
			const int i2 = indices[i + 2];

			const std::vector<Vector2> triangle =
			{
				verticesOut[i0].position.GetXY(),
				verticesOut[i1].position.GetXY(),
				verticesOut[i2].position.GetXY()
			};
			std::vector<Vector2> clipped_triangle{ triangle };

			// Check if triangle is entirly in- or outside viewport
			if (!ClipTriangle(clipped_triangle))
			{
				// Completely outside, skip triangle
				if(clipped_triangle.empty())
					continue;
				else
				{
					// Completely inside, just add unmodified triangle
					newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size()));
					newVerticesOut.emplace_back(verticesOut[i0]);

					newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size()));
					newVerticesOut.emplace_back(verticesOut[i1]);

					newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size()));
					newVerticesOut.emplace_back(verticesOut[i2]);
					continue;
				}
			}
			const Vector2 v0v1 = triangle[1] - triangle[0];
			const Vector2 v0v2 = triangle[2] - triangle[0];
			const Vector2 v1v2 = triangle[2] - triangle[1];
			const Vector2 v2v0 = triangle[0] - triangle[2];

			const float invTriArea = 1.f / Vector2::Cross(v0v1, v0v2);

			for (const auto& vertexPos : clipped_triangle)
			{
				const Vector2 v1p = vertexPos - triangle[1];
				float w0 = Vector2::Cross(v1v2, v1p);

				const Vector2 v2p = vertexPos - triangle[2];
				float w1 = Vector2::Cross(v2v0, v2p);

				const Vector2 v0p = vertexPos - triangle[0];
				float w2 = Vector2::Cross(v0v1, v0p);

				w0 *= invTriArea;
				w1 *= invTriArea;
				w2 *= invTriArea;

				Vertex_Out newVert = Vertex_Out::Interpolate({ verticesOut[i0], verticesOut[i1], verticesOut[i2] }, w0, w1, w2, true);

				newVert.position.x = vertexPos.x;
				newVert.position.y = vertexPos.y;

				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size()));
				newVerticesOut.emplace_back(newVert);
			}

			// TODO: Implement the Hossam-Hazel-Godfried ear clipping algorithm

			// Triangulize a 4 sided polygon
			if (clipped_triangle.size() == 4)
			{
				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size() - 4));
				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size() - 2));
			}

			// Triangulize a 5 sided polygon
			if (clipped_triangle.size() == 5)
			{
				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size() - 5));
				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size() - 3));
				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size() - 2));
				newIndices.emplace_back(static_cast<uint32_t>(newVerticesOut.size() - 5));
			}
		}

		Mesh clipped_mesh{ mesh };
		clipped_mesh.SetVerticesOut(newVerticesOut);
		clipped_mesh.SetIndices(newIndices);
		return clipped_mesh;
	}

	bool dae::Renderer::ClipTriangle(std::vector<Vector2>& triVerts)
	{
		const Bounds clipping_bounds{ {-1.f, -1.f}, {1.f, 1.f} };
		const int outcode0 = clipping_bounds.ComputeOutCode(triVerts[0]);
		const int outcode1 = clipping_bounds.ComputeOutCode(triVerts[1]);
		const int outcode2 = clipping_bounds.ComputeOutCode(triVerts[2]);

		// Triangle is entirely inside viewport
		if (!(outcode0 | outcode1 | outcode2)) return false;

		// Triangle is entirely outside viewport
		if (outcode0 != 0 && outcode1 != 0 && outcode2 != 0)
		{
			triVerts.clear();
			return false;
		}

		const std::vector<Vector2> clipping_area = { {-1.0f, -1.0f}, {1.0f, -1.0f},{1.0f, 1.0f}, {-1.0f, 1.0f} };
		std::vector<Vector2> ring = triVerts;

		Vector2 p1 = clipping_area[clipping_area.size() - 1];
		std::vector<Vector2> input_list;

		// Used to check if the new, intersected point lies just outside the viewport
		const float EPSILON{ 0.01f };
		for (const Vector2& p2 : clipping_area)
		{
			input_list.clear();
			input_list.insert(input_list.end(), ring.begin(), ring.end());

			if (input_list.empty())
			{
				triVerts = ring;
				return true;
			}
			
			Vector2 lineStartPoint = input_list[input_list.size() - 1];
			ring.clear();

			for (const Vector2& lineEndPoint : input_list)
			{
				if (Vector2::IsInside(lineEndPoint, p1, p2))
				{
					if (!Vector2::IsInside(lineStartPoint, p1, p2))
					{
						Vector2 intersection{ Vector2::Intersection(p1, p2, lineStartPoint, lineEndPoint) };
						if (intersection.x > -EPSILON && intersection.x < m_Width + EPSILON &&
							intersection.y > -EPSILON && intersection.y < m_Height + EPSILON)
						{
							intersection.x = std::clamp(intersection.x, 0.f, static_cast<float>(m_Width));
							intersection.y = std::clamp(intersection.y, 0.f, static_cast<float>(m_Height));
						}

						ring.emplace_back(intersection);
					}

					ring.emplace_back(lineEndPoint);
				}
				else if (Vector2::IsInside(lineStartPoint, p1, p2))
				{
					Vector2 intersection{ Vector2::Intersection(p1, p2, lineStartPoint, lineEndPoint) };
					if (intersection.x > -EPSILON && intersection.x < m_Width + EPSILON &&
						intersection.y > -EPSILON && intersection.y < m_Height + EPSILON)
					{
						intersection.x = std::clamp(intersection.x, 0.f, static_cast<float>(m_Width));
						intersection.y = std::clamp(intersection.y, 0.f, static_cast<float>(m_Height));
					}
					ring.emplace_back(intersection);
				}

				lineStartPoint = lineEndPoint;
			}

			p1 = p2;
		}

		triVerts = ring;
		return true;
	}

	ColorRGB Renderer::ShadePixel(const Mesh& mesh, const Vertex_Out& vertex, const RenderInfo& renderInfo, const ColorRGB& currPixelColor) const
	{
		// Light
		const Vector3 lightDirection{ .577f, -.577f, 0.577f };
		const float lightIntensity{ 7.f };
		const ColorRGB ambient{ 0.025f, 0.025f, 0.025f };

		const float kd{ 1.f };
		const float shininess{ 25.f };

		// Transparent material
		// Lerp current color in backbuffer with this material
		// Source: https://magcius.github.io/xplain/article/rast1.html
		if (mesh.GetEffect()->GetEffectType() == EffectType::Transparent)
		{
			// Lambert
			const Vector4 diffuseAlpha{ mesh.GetDiffuseMap()->SampleRGBA(vertex.uv) };
			if(diffuseAlpha.w <= 0.01f) return currPixelColor;

			const ColorRGB diffuse{ diffuseAlpha.x, diffuseAlpha.y, diffuseAlpha.z };
			return ColorRGB::Lerp(currPixelColor, diffuse, diffuseAlpha.w);
		}

		Vector3 sampled_normal{ vertex.normal };
		if (renderInfo.useNormalMap)
		{
			const Vector3 binormal{ Vector3::Cross(vertex.normal, vertex.tangent) };
			const Matrix tangent_space_axis{ vertex.tangent, binormal.Normalized(), vertex.normal, {0.f, 0.f, 0.f} };
			sampled_normal = mesh.GetNormalMap()->SampleNormal(vertex.uv);
			sampled_normal = 2.f * sampled_normal - Vector3{ 1.f, 1.f, 1.f };
			sampled_normal = tangent_space_axis.TransformVector(sampled_normal);
		}

		sampled_normal.Normalize();
		const float observed_area{ std::max(0.f, Vector3::Dot(sampled_normal, -lightDirection)) };

		switch (renderInfo.shadingMode)
		{
			case dae::ShadingMode::FinalColor:
			{
				// Lambert
				const ColorRGB lambert_diffuse{ (mesh.GetDiffuseMap()->SampleColor(vertex.uv) * kd) / PI };

				// Phong
				const float exp{ mesh.GetGlossMap()->SampleColor(vertex.uv).r * shininess };
				ColorRGB phong_color{ mesh.GetSpecularMap()->SampleColor(vertex.uv) * LightUtils::PhongSpecular(1.f, exp, lightDirection, vertex.viewDirection, sampled_normal) };

				return (lambert_diffuse * lightIntensity + phong_color + ambient) * observed_area;
			}
			case dae::ShadingMode::ObservedArea:
			{
				return { observed_area, observed_area, observed_area };
			}
			case dae::ShadingMode::Specular:
			{
				// Phong
				const float exp{ mesh.GetGlossMap()->SampleColor(vertex.uv).r * shininess };
				ColorRGB phong_color{ mesh.GetSpecularMap()->SampleColor(vertex.uv) * LightUtils::PhongSpecular(1.f, exp, lightDirection, vertex.viewDirection, sampled_normal) };

				return phong_color * observed_area;
			}
			case dae::ShadingMode::Diffuse:
			{
				// Lambert
				const ColorRGB lambert_diffuse{ (mesh.GetDiffuseMap()->SampleColor(vertex.uv) * kd) / PI };
				return (lambert_diffuse + ambient);
			}
		}

		return ColorRGB{};
	}
#pragma endregion

#pragma region Hardware
	HRESULT Renderer::InitializeDirectX()
	{
		// 1. Create Device (the device) & DeviceContext (how the device is used)
		D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;
		uint32_t create_device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flags, &feature_level,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
		if (FAILED(result))
			return result;

		// Create DXGI Factory
		IDXGIFactory1* pdxgi_factory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pdxgi_factory));
		if (FAILED(result))
			return result;

		// 2. Create SwapChain
		DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
		swap_chain_desc.BufferDesc.Width = m_Width;
		swap_chain_desc.BufferDesc.Height = m_Height;
		swap_chain_desc.BufferDesc.RefreshRate.Numerator = 1;
		swap_chain_desc.BufferDesc.RefreshRate.Denominator = 144;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.BufferCount = 1;
		swap_chain_desc.Windowed = true;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swap_chain_desc.Flags = 0;

		// Get the handle (HWND) from the SDL Backbuffer
		SDL_SysWMinfo sys_wm_info{};
		SDL_VERSION(&sys_wm_info.version);
		SDL_GetWindowWMInfo(m_pWindow, &sys_wm_info);
		swap_chain_desc.OutputWindow = sys_wm_info.info.win.window;

		// Create SwapChain
		result = pdxgi_factory->CreateSwapChain(m_pDevice, &swap_chain_desc, &m_pSwapChain);
		if (FAILED(result))
			return result;

		// 3. Create DepthStencil (DS) & DepthStencilView (DSV)
		// Resource (Buffer)
		D3D11_TEXTURE2D_DESC depth_stencil_desc{};
		depth_stencil_desc.Width = m_Width;
		depth_stencil_desc.Height = m_Height;
		depth_stencil_desc.MipLevels = 1;
		depth_stencil_desc.ArraySize = 1;
		depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_desc.SampleDesc.Count = 1;
		depth_stencil_desc.SampleDesc.Quality = 0;
		depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
		depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depth_stencil_desc.CPUAccessFlags = 0;
		depth_stencil_desc.MiscFlags = 0;

		// View
		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
		depth_stencil_view_desc.Format = depth_stencil_desc.Format;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result))
			return result;

		pdxgi_factory->Release();

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depth_stencil_view_desc, &m_pDepthStencilView);
		if (FAILED(result))
			return result;

		// 4. Create RenderTarget (RT) & RenderTargetView (RTV)

		// Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result))
			return result;

		// View
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result))
			return result;

		// 5. Bind RTV & DSV to Output Merger Stage
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		// 6. Set the viewport
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);
		return result;
	}

	void Renderer::RenderHardware(std::vector<std::shared_ptr<Mesh>>& pMeshes, const RenderInfo& renderInfo) const
	{
		if (!m_IsInitialized)
			return;

		// 1. Clear RTV & DSV
		ColorRGB clearColor{ renderInfo.clearColor };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		// 2. Set pipeline + invoke drawcalls
		std::for_each(begin(pMeshes), end(pMeshes), [&](std::shared_ptr<Mesh>& mesh)
			{
				if (!mesh->IsEnabled()) return;

				mesh->Render(m_pDeviceContext);
			}
		);

		// 3. Present backbuffer (SWAP)
		m_pSwapChain->Present(0, 0);
	}
#pragma endregion

	
}
