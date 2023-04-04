#pragma once

#include "glm/glm.hpp"

namespace Raymarching {
	struct Camera {
		glm::vec3 Position{};
		glm::vec3 Front{ 0.f, 0.f, -1.f };
		glm::vec3 Up{ 0.f, 1.f, 0.f };
		float yaw;
		float pitch;
		float FOV;
	};
}