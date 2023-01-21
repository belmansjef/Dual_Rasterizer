#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <algorithm>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}

		const float moveSpeed{ 15.f };
		const float sprintSpeed{ 30.f };
		const float lookSpeed{ 0.5f * TO_RADIANS };
		 
		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		float nearPlane{ 0.1f }, farPlane{100.f};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f}, float _aspectRatio = 1.333f)
		{
			fovAngle = _fovAngle;
			CalculateFov();

			origin = _origin;
			aspectRatio = _aspectRatio;
			CalculateProjectionMatrix();
		}

		void CalculateFov()
		{
			fov = tanf((fovAngle * TO_RADIANS) * 0.5f);
			CalculateProjectionMatrix();
		}

		void CalculateViewMatrix()
		{			
			viewMatrix = Matrix::CreateLookAtLH(origin, forward, up);
			invViewMatrix = Matrix::Inverse(viewMatrix);
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
		}

		void Update(const Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			mouseX = std::clamp(mouseX, -2, 2);
			mouseY = std::clamp(mouseY, -2, 2);
				
			// Ensurese framerate independent movement speed
			const float constMoveSpeed = pKeyboardState[SDL_SCANCODE_LSHIFT] ? sprintSpeed * deltaTime : moveSpeed * deltaTime;

			origin += (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT]) * constMoveSpeed * right;
			origin -= (pKeyboardState[SDL_SCANCODE_Q] || pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT]) * constMoveSpeed * right;

			origin += (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_Z] || pKeyboardState[SDL_SCANCODE_UP]) * constMoveSpeed * forward;
			origin -= (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN]) * constMoveSpeed * forward;

			// Mouse input is already framerate independent, so no need to multiply it by our frametime
			if (mouseState & SDL_BUTTON_LMASK)
			{
				if (mouseState & SDL_BUTTON_RMASK)
					origin.y += static_cast<float>(mouseY);
				else
				{
					origin   += static_cast<float>(mouseY) * forward;
					totalYaw += static_cast<float>(mouseX) * lookSpeed;
				}
			}

			if (mouseState & SDL_BUTTON_RMASK && !(mouseState & SDL_BUTTON_LMASK))
			{
				totalPitch	+= -static_cast<float>(mouseY) * lookSpeed;
				totalYaw	+= static_cast<float>(mouseX) * lookSpeed;
			}

			if (mouseState & SDL_BUTTON_LMASK || mouseState & SDL_BUTTON_RMASK)
				CalculateLookRotation();

			CalculateViewMatrix();
		}

		void CalculateLookRotation()
		{
			const Matrix finalRotation{ Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw) };
			forward = finalRotation.TransformVector(Vector3::UnitZ);
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right);
		}
	};
}