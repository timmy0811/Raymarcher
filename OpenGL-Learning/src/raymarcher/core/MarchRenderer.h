#pragma once

#include <GLEW/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <algorithm>

#include "config.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "../Primitive.h"
#include "OpenGL_util/core/Renderer.h"
#include "Camera.h"

#include "OpenGL_util/core/VertexBuffer.h"
#include "OpenGL_util/core/VertexArray.h"
#include "OpenGL_util/core/IndexBuffer.h"
#include "OpenGL_util/core/VertexBufferLayout.h"
#include "OpenGL_util/core/Shader.h"

#define MAX_OBJ 4

namespace Raymarching {
	class MarchRenderer {
	private:
		// Render
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<VertexBufferLayout> m_VertexBufferLayout;
		std::unique_ptr<VertexArray> m_VertexArray;

		glm::mat4 m_MatProjectionVertex;

		glm::mat4 m_MatProjectionRay;
		glm::mat4 m_MatView;

		Shader* m_ShaderBasic;

		// Attributes
		Camera m_Camera;

		int m_ObjectsInScene;
		int m_ObjectsIds[MAX_OBJ];
		glm::vec3 m_ObjectTransforms[MAX_OBJ];
		glm::vec3 m_ObjectColor[MAX_OBJ];
		float u_ObjectPattern[MAX_OBJ];

		int m_SelectedObj;
		const char* m_ComboOptions[3];

		float m_FOV = 60.f;

		float m_ApplicationTime = 0.f;

		float m_TransformSpotlight[3];
		float m_SoftShadow = 6.f;
		float m_BodyInterpolation = 0.1f;
		bool m_BodyInterpolationEnabled = false;

		int m_MarchSteps = 256;
		int m_ReflectionIterations = 2;
		float m_Reflection = 0.05f;
		float m_Roughness = 90.f;
		bool m_RoughnessEnabled = false;
		float m_NormalCheckOffset = 0.01f;

		float m_MinAlbedo = 0.1f;
		float m_SurfaceDistance = 0.001f;
		float m_RenderDistance = 100.f;
		float m_SpecPower = 128.f;
		float m_ShadowStrength = 0.3f;

		float m_SkyLowerGradient[3];
		float m_SkyUpperGradient[3];

		bool m_RepeatSpaceX;
		bool m_RepeatSpaceZ;

		// Input
		bool m_IsInViewMode;
		inline static float m_CameraSpeed;

		double m_LastX = 400, m_LastY = 300;
		bool m_FirstMouseInit = true;

		inline static double s_MouseX = 0;
		inline static double s_MouseY = 0;

		void ProcessMouse(GLFWwindow* window);
		static void OnMouseCallback(GLFWwindow* window, double xpos, double ypos);
		static void OnScrollCallback(GLFWwindow* window, double xpos, double ypos);

		std::string IdToString(int id) const;

	public:
		explicit MarchRenderer();
		~MarchRenderer();

		void OnRender();
		void OnImGuiRender(GLFWwindow* window);
		void OnInput(GLFWwindow* window);
		void OnUpdate();

		void GUI(GLFWwindow* window);
	};
}