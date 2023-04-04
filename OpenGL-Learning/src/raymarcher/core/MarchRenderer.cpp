#include "MarchRenderer.h"

Raymarching::MarchRenderer::MarchRenderer()
{
	m_ShaderBasic = new Shader("res/shader/shader_basic.vert", "res/shader/shader_basic.frag");
	m_MatProjectionVertex = glm::ortho(0.0f, (float)conf.WIN_WIDTH, 0.0f, (float)conf.WIN_HEIGHT, -1.0f, 1.0f);

	unsigned int index[] = {
		0, 1, 2, 2, 3, 0
	};

	m_IndexBuffer = std::make_unique<IndexBuffer>(index, 6);
	m_VertexBuffer = std::make_unique<VertexBuffer>(4, sizeof(Raymarching::Primitive::Vertex));

	Raymarching::Primitive::Vertex vert[] = {
		glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, conf.WIN_HEIGHT, 0.f),
		glm::vec3(conf.WIN_WIDTH, conf.WIN_HEIGHT, 0.f),
		glm::vec3(conf.WIN_WIDTH, 0.f, 0.f)
	};

	m_VertexBuffer->Bind();
	m_VertexBuffer->AddVertexData(vert, sizeof(Raymarching::Primitive::Vertex) * 4);

	m_VertexBufferLayout = std::make_unique<VertexBufferLayout>();
	m_VertexBufferLayout->Push<float>(3);	// Position

	m_VertexArray = std::make_unique<VertexArray>();
	m_VertexArray->AddBuffer(*m_VertexBuffer, *m_VertexBufferLayout);

	m_Camera.Position = { 0.f, 1.5f, -3.f };
	m_Camera.Front = { 0.f, 0.f, 1.f };

	m_ShaderBasic->Bind();
	m_ShaderBasic->SetUniformMat4f("u_Projection", m_MatProjectionVertex);
	m_ShaderBasic->SetUniform3f("u_CameraOrigin", m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z);
	m_ShaderBasic->SetUniform2f("u_Resolution", conf.WIN_WIDTH, conf.WIN_HEIGHT);

	m_CameraSpeed = 0.05f;
	m_TransformSpotlight[1] = 10.f;

	std::fill(m_ObjectsIds, m_ObjectsIds + MAX_OBJ, -1);
	std::fill(u_ObjectPattern, u_ObjectPattern + MAX_OBJ, 0);
	std::fill(m_ObjectTransforms, m_ObjectTransforms + MAX_OBJ, glm::vec3(0.f));
	std::fill(m_ObjectColor, m_ObjectColor + MAX_OBJ, glm::vec3(1.f));

	m_ComboOptions[0] = "Sphere";
	m_ComboOptions[1] = "Plane";
	m_ComboOptions[2] = "Mandelbulb";
}

void Raymarching::MarchRenderer::ProcessMouse(GLFWwindow* window)
{
	if (m_FirstMouseInit) {
		m_LastX = s_MouseX;
		m_LastY = s_MouseY;
		m_FirstMouseInit = false;
	}

	static bool keyPressed1 = false;
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !keyPressed1)
	{
		keyPressed1 = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		m_IsInViewMode = true;
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE && keyPressed1) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		keyPressed1 = false;
		m_IsInViewMode = false;
	}

	double xoffset = s_MouseX - m_LastX;
	double yoffset = m_LastY - s_MouseY;
	m_LastX = s_MouseX;
	m_LastY = s_MouseY;

	const double sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	if (keyPressed1) {
		m_Camera.yaw += (float)xoffset;
		m_Camera.pitch += (float)yoffset;

		if (m_Camera.pitch > 89.99f)
			m_Camera.pitch = 89.99f;
		if (m_Camera.pitch < -89.99f)
			m_Camera.pitch = -89.99f;

		glm::vec3 direction;

		direction.x = cos(glm::radians(m_Camera.yaw)) * cos(glm::radians(m_Camera.pitch));
		direction.y = sin(glm::radians(m_Camera.pitch));
		direction.z = sin(glm::radians(m_Camera.yaw)) * cos(glm::radians(m_Camera.pitch));

		m_Camera.Front = glm::normalize(direction);
	}
}

void Raymarching::MarchRenderer::OnMouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	Raymarching::MarchRenderer::s_MouseX = float(xpos);
	Raymarching::MarchRenderer::s_MouseY = float(ypos);
}

void Raymarching::MarchRenderer::OnScrollCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (ypos < 0.f) m_CameraSpeed *= 0.9f;
	else if (ypos > 0.f) m_CameraSpeed *= 1.1f;
}

Raymarching::MarchRenderer::~MarchRenderer()
{
	delete m_ShaderBasic;
}

void Raymarching::MarchRenderer::OnRender()
{
	m_ShaderBasic->Bind();
	m_VertexArray->Bind();
	m_IndexBuffer->Bind();
	GLCall(BatchRenderer::Draw(*m_VertexArray, *m_IndexBuffer, *m_ShaderBasic, GL_TRIANGLES));
}

void Raymarching::MarchRenderer::OnImGuiRender(GLFWwindow* window)
{
	GUI(window);
}

void Raymarching::MarchRenderer::OnInput(GLFWwindow* window)
{
	//glfwSetCursorPosCallback(window, Raymarching::MarchRenderer::OnMouseCallback);

	glfwGetCursorPos(window, &s_MouseX, &s_MouseY);
	glfwSetScrollCallback(window, OnScrollCallback);
	ProcessMouse(window);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		m_Camera.Position += m_CameraSpeed * m_Camera.Front;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		m_Camera.Position -= m_CameraSpeed * m_Camera.Front;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		m_Camera.Position -= glm::normalize(glm::cross(m_Camera.Front, m_Camera.Up)) * m_CameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		m_Camera.Position += glm::normalize(glm::cross(m_Camera.Front, m_Camera.Up)) * m_CameraSpeed;
}

void Raymarching::MarchRenderer::OnUpdate()
{
	m_MatView = glm::lookAt(m_Camera.Position, m_Camera.Position + m_Camera.Front, m_Camera.Up);
	m_MatProjectionRay = glm::perspectiveFov(glm::radians(m_FOV), (float)conf.WIN_WIDTH, (float)conf.WIN_HEIGHT, 0.1f, 100.f);

	m_ApplicationTime = (float)glfwGetTime();
	m_ShaderBasic->Bind();
	m_ShaderBasic->SetUniform1f("u_Time", m_ApplicationTime);
	m_ShaderBasic->SetUniform3f("u_CameraOrigin", m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z);
	m_ShaderBasic->SetUniform2f("u_Mouse", (float)s_MouseX, (float)s_MouseY);

	m_ShaderBasic->SetUniformMat4f("u_InverseView", glm::inverse(m_MatView));
	m_ShaderBasic->SetUniformMat4f("u_InverseProjection", glm::inverse(m_MatProjectionRay));

	// Variables
	m_ShaderBasic->SetUniform3f("u_SpotLightPos", m_TransformSpotlight[0], m_TransformSpotlight[1], m_TransformSpotlight[2]);
	m_ShaderBasic->SetUniform1f("u_SoftShadow", m_SoftShadow);
	m_ShaderBasic->SetUniform1i("u_ReflectionIteration", m_ReflectionIterations);
	m_ShaderBasic->SetUniform1f("u_SpecularPower", m_SpecPower);
	m_ShaderBasic->SetUniform1f("u_ShadowStrength", m_ShadowStrength);

	m_ShaderBasic->SetUniform1f("u_BodyInterpolationEnabled", m_BodyInterpolationEnabled ? 1.0 : -1.0);
	m_ShaderBasic->SetUniform1f("u_BodyInterpolation", m_BodyInterpolation);
	m_ShaderBasic->SetUniform1i("u_MarchSteps", m_MarchSteps);
	m_ShaderBasic->SetUniform1f("u_SurfaceDistance", m_SurfaceDistance); 
	m_ShaderBasic->SetUniform1f("u_RenderDistance", m_RenderDistance); 
	m_ShaderBasic->SetUniform1f("u_RenderDistance", m_RenderDistance);
	m_ShaderBasic->SetUniform1f("u_NormalCheckOffset", m_NormalCheckOffset);

	m_ShaderBasic->SetUniform3f("u_SkyLowerGradient", m_SkyLowerGradient[0], m_SkyLowerGradient[1], m_SkyLowerGradient[2]);
	m_ShaderBasic->SetUniform3f("u_SkyUpperGradient", m_SkyUpperGradient[0], m_SkyUpperGradient[1], m_SkyUpperGradient[2]);

	m_ShaderBasic->SetUniform1f("u_RepeatSpaceX", m_RepeatSpaceX ? 1.0 : 0.0);
	m_ShaderBasic->SetUniform1f("u_RepeatSpaceZ", m_RepeatSpaceZ ? 1.0 : 0.0);

	m_ShaderBasic->SetUniform1f("u_MinAlbedo", m_MinAlbedo);

	m_ShaderBasic->SetUniform1f("u_Reflection", m_Reflection);
	m_ShaderBasic->SetUniform1f("u_Roughness", m_RoughnessEnabled ? 200.f - m_Roughness + 1.f : 100000.f);

	// m_ShaderBasic->SetUniform1i("u_ObjectCount", m_ObjectsInScene);
	m_ShaderBasic->SetUniform1iv("u_ObjectIDs", MAX_OBJ, m_ObjectsIds);
	m_ShaderBasic->SetUniform3fv("u_ObjectTransforms", MAX_OBJ, m_ObjectTransforms);
	m_ShaderBasic->SetUniform3fv("u_ObjectColor", MAX_OBJ, m_ObjectColor);
	m_ShaderBasic->SetUniform1fv("u_ObjectPattern", MAX_OBJ, u_ObjectPattern);
}

void Raymarching::MarchRenderer::GUI(GLFWwindow* window)
{
	ImGui::SetNextWindowPos({ 0.f, 0.f });
	ImGui::SetNextWindowSize({ (const float)conf.WIN_WIDTH / 4.f, (const float)conf.WIN_HEIGHT });
	
	ImGui::Begin("Scene");
	ImGui::Text("Press C to hide GUI");
	ImGui::Separator();

	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.7, 0.35, 0.0, 1.0));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9, 0.55, 0.0, 1.0));

	if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Scene");
		ImGui::SliderFloat3("Spotlight", m_TransformSpotlight, -15.f, 15.f);
		ImGui::SliderFloat("Specular Power", &m_SpecPower, 4.f, 512.f);

		ImGui::Separator();
		ImGui::Text("Shadows");
		ImGui::SliderFloat("Soft-Shadows", &m_SoftShadow, 2.f, 128);
		ImGui::SliderFloat("Shadow Strength", &m_ShadowStrength, 0.f, 1.f);
		ImGui::SliderFloat("Min. Albedo", &m_MinAlbedo, 0.f, 1.f);

		ImGui::Separator();
		ImGui::Text("Reflections");
		ImGui::InputInt("Reflection Iterations", &m_ReflectionIterations, 1, 1);
		ImGui::SliderFloat("Reflectiveness", &m_Reflection, 0.001f, 1.f);
		ImGui::Checkbox("Roughness Enabled", &m_RoughnessEnabled);
		if(m_RoughnessEnabled) ImGui::SliderFloat("Roughness", &m_Roughness, 0.f, 200.f);
	}

	if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Interpolation Enabled", &m_BodyInterpolationEnabled);
		if (m_BodyInterpolationEnabled) ImGui::SliderFloat("Body Interpolation", &m_BodyInterpolation, 0.f, 1.f);
		
		ImGui::InputInt("March Steps", &m_MarchSteps, 2, 16);
		ImGui::InputFloat("Surface Distance", &m_SurfaceDistance, 0.001f, 0.01f, "%.7f");
		ImGui::SliderFloat("Render Distance", &m_RenderDistance, 1.f, 750.f);
		ImGui::InputFloat("Normal Test Offset", &m_NormalCheckOffset, 0.000001f, 0.01f, "%.7f");

		ImGui::Separator();
		ImGui::Checkbox("Repeat Space On X", &m_RepeatSpaceX);
		ImGui::Checkbox("Repeat Space On Y", &m_RepeatSpaceZ);
	}

	if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Skybox Lower Gradient", m_SkyLowerGradient);
		ImGui::ColorEdit3("Skybox Upper Gradient", m_SkyUpperGradient);

		ImGui::Separator();
		ImGui::Text("Add an object:");
		ImGui::Combo("Object", &m_SelectedObj, m_ComboOptions, IM_ARRAYSIZE(m_ComboOptions));

		float button_width = ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2;
		if (ImGui::Button("Add to Scene", { button_width , 20.f })) {
			if (m_ObjectsInScene < MAX_OBJ) {
				for (int i = 0; i < MAX_OBJ; i++) {
					if (m_ObjectsIds[i] == -1) {
						m_ObjectsIds[i] = m_SelectedObj;
						m_ObjectsInScene++;
						break;
					}
				}
			}
			else {
				LOGC("Maximum object count in scene reached!", LOG_COLOR::FAULT);
			}
		}

		ImGui::Separator();
		ImGui::Text("Scene Objects:");

		for (int i = 0; i < MAX_OBJ; i++) {
			int id = m_ObjectsIds[i];
			if (id != -1) {
				std::string stri = IdToString(id) + " - " + std::to_string(i);
				const char* str = stri.c_str();
				if (ImGui::TreeNode(str)) {
					ImGui::Separator();
					ImGui::DragFloat3("Transform", &m_ObjectTransforms[i].x, 0.05f);
					ImGui::ColorEdit3("Albedo", &m_ObjectColor[i].x);

					ImGui::InputFloat("Apply Pattern", &u_ObjectPattern[i], 1.f, 1.f, "%.0f");

					ImGui::PushStyleColor(ImGuiCol_Button, { 0.8, 0.1, 0.1, 1.0 });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9, 0.2, 0.2, 1.0 });

					if (ImGui::Button("Remove", { button_width - 30.f , 20.f })) {
						m_ObjectsIds[i] = -1;
						m_ObjectColor[i] = { 1.f, 1.f, 1.f };
						m_ObjectTransforms[i] = { 0.f, 0.f, 0.f };
						m_ObjectsInScene--;
					}

					ImGui::PopStyleColor(2);

					ImGui::TreePop();
					ImGui::Separator();
				}
			}
		}
	}

	ImGui::PopStyleColor(2);

	ImGui::End();
}

std::string Raymarching::MarchRenderer::IdToString(int id) const
{
	switch (id) {
	case 0:
		return "Sphere";
	case 1:
		return "Plane";
	case 2:
		return "Mandelbulb";
	}
	return "test";
}
