//This is an early work in progress.  Lots of work still to do.


#include <Windows.h>
using namespace std;

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>

#include "stb_image.h"

#include "Clout.h"
#include "Console.h"
#include "ModelViewer.h"


//The main Model viewer object
MODEL_VIEWER ModelViewer;

//Shadow Mapping
#define SHADOW_MAP_SIZE_FACTOR	2.0f //HOw much larger is the shadow map compared to the window

//Near and Far clipping planes
#define CLIP_PLANE_NEAR	10.0f
#define CLIP_PLANE_FAR	2000.0f

//Camera
#define CAMERA_OFFSET_X		140
#define CAMERA_OFFSET_Y		260
#define CAMERA_OFFSET_Z		-780
#define CAMERA_OFFSET_YAW	-90
#define CAMERA_OFFSET_PITCH	-14

//Debug stuff
int iDebug_SelectedView = 0;	//Which view is selected for viewing.  0=Free cam, 1=Light source
bool bDebug_DrawDebugData = false;
bool bDebug_DrawShadowMap = false;


unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Texture failed to load at path: %s", path);
		stbi_image_free(data);
	}

	return textureID;
}


//For debugging, draws a grid in OpenGL coordinates
void MODEL_VIEWER::DrawGrid()
{
	const float fStep = 20;	//Distance between each line

	//Reset all model matrix values
		Shader_Main->setMat4("model", glm::mat4(1.0f));

	//Shader stuff
		Shader_Main->setVec3("objectColor", glm::vec3(0.0f, 0.0f, 0.0f));
		Shader_Main->setFloat("Lighting_Ambient", 1.0f);

	//Draw the grid
		glBegin(GL_LINES);
			for (float x = -1000; x < 1000; x += fStep)
			{
				glVertex3f(x, 0, -1000.0f);
				glVertex3f(x, 0, 1000.0f);
			}

			for (float z = -1000; z < 1000; z += fStep)
			{
				glVertex3f(-1000, 0, z);
				glVertex3f(1000, 0, z);
			}
		glEnd();
}

//Draw the position of the OpenGL origin
void MODEL_VIEWER::DrawOrigin()
{
	//Reset all model matrix values
		Shader_Main->setMat4("model", glm::mat4(1.0f));

	//Shader stuff
		Shader_Main->setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
		Shader_Main->setFloat("Lighting_Ambient", 1.0f);

	//Draw the cube (TODO: this may draw weird because the shader is expecting normals, which we're not providing right now
		glBegin(GL_POLYGON);
			// BACK
				glVertex3f(5, -5, 5);
				glVertex3f(5, 5, 5);
				glVertex3f(-5, 5, 5);
				glVertex3f(-5, -5, 5);
			// RIGHT
				glVertex3f(5, -5, -5);
				glVertex3f(5, 5, -5);
				glVertex3f(5, 5, 5);
				glVertex3f(5, -5, 5);
			//  LEFT
				glVertex3f(-5, -5, 5);
				glVertex3f(-5, 5, 5);
				glVertex3f(-5, 5, -5);
				glVertex3f(-5, -5, -5);
			// TOP
				glVertex3f(5, 5, 5);
				glVertex3f(5, 5, -5);
				glVertex3f(-5, 5, -5);
				glVertex3f(-5, 5, 5);
			// BOTTOM
				glVertex3f(5, -5, -5);
				glVertex3f(5, -5, 5);
				glVertex3f(-5, -5, 5);
				glVertex3f(-5, -5, -5);
		glEnd();

	//Axis directions
		//Up (blue)
		Shader_Main->setVec3("objectColor", glm::vec3(0.0f, 0.0f, 1.0f));
			glBegin(GL_LINES);
				glVertex3f(0, 0, 0);
				glVertex3f(0, 100, 0);
			glEnd();

		//Right (red)
			Shader_Main->setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
			glBegin(GL_LINES);
				glVertex3f(0, 0, 0);
				glVertex3f(100, 0, 0);
			glEnd();

		//In (green)
			Shader_Main->setVec3("objectColor", glm::vec3(0.0f, 1.0f, 0.0f));
			glBegin(GL_LINES);
				glVertex3f(0, 0, 0);
				glVertex3f(0, 0, 100);
			glEnd();
}

//Draw a cube where the 3D light is positioned
void MODEL_VIEWER::DrawLightSource()
{
	//Shader stuff
		Shader_Main->setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
		Shader_Main->setFloat("Lighting_Ambient", 1.0f);
		Shader_Main->setFloat("Lighting_Specular", 0.0f);

	//Move to the light position
		Shader_Main->setMat4("model", glm::translate(glm::mat4(1.0f), lightPos));

	//Draw the cube (TODO: this may draw weird because the shader is expecting normals, which we're not providing right now
		glBegin(GL_POLYGON);

		const float fSize = 5.0f; //Actual size is twice this

		// BACK
		glVertex3f(fSize, -fSize, fSize);
		glVertex3f(fSize, fSize, fSize);
		glVertex3f(-fSize, fSize, fSize);
		glVertex3f(-fSize, -fSize, fSize);
		// RIGHT
		glVertex3f(fSize, -fSize, -fSize);
		glVertex3f(fSize, fSize, -fSize);
		glVertex3f(fSize, fSize, fSize);
		glVertex3f(fSize, -fSize, fSize);
		//  LEFT
		glVertex3f(-fSize, -fSize, fSize);
		glVertex3f(-fSize, fSize, fSize);
		glVertex3f(-fSize, fSize, -fSize);
		glVertex3f(-fSize, -fSize, -fSize);
		// TOP
		glVertex3f(fSize, fSize, fSize);
		glVertex3f(fSize, fSize, -fSize);
		glVertex3f(-fSize, fSize, -fSize);
		glVertex3f(-fSize, fSize, fSize);
		// BOTTOM
		glVertex3f(fSize, -fSize, -fSize);
		glVertex3f(fSize, -fSize, fSize);
		glVertex3f(-fSize, -fSize, fSize);
		glVertex3f(-fSize, -fSize, -fSize);
		
		glEnd();
}


//Create new OpenGL target texture when the window is resized.
void MODEL_VIEWER::ResizeWindow()
{
	if (NewWindowSize.x == 0 || NewWindowSize.y == 0)
		return;

	//Save the new window size
		WindowSize = NewWindowSize;

	//Resize the render target texture
		glBindTexture(GL_TEXTURE_2D, RenderTargetTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)WindowSize.x, (GLsizei)WindowSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	//Resize the depth buffer
		glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)WindowSize.x, (GLsizei)WindowSize.y);

	//Resize the shadow map
		glBindTexture(GL_TEXTURE_2D, ShadowMapTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (GLsizei)(WindowSize.x * SHADOW_MAP_SIZE_FACTOR), (GLsizei)(WindowSize.y * SHADOW_MAP_SIZE_FACTOR), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
}

void MODEL_VIEWER::Init()
{	
	//Setup some OpenGL stuff
		glewExperimental = true;
		glewInit();
		glEnable(GL_DEPTH_TEST);
			
	//Setup camera stuff
		ActiveCameraPos = glm::vec3(0.0f, 0.0f, -100.0f);
		cameraDirection = glm::normalize(ActiveCameraPos - glm::vec3(0.0f, 0.0f, 0.0f));
		cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
		up = glm::vec3(0.0f, -1.0f, 0.0f);
		cameraRight = glm::normalize(glm::cross(up, cameraDirection));
		cameraUp = glm::cross(cameraDirection, cameraRight);

		View_MainCameraPos.x = 0.0f; //CAMERA_OFFSET_X
		View_MainCameraPos.y = 0.0f; //CAMERA_OFFSET_Y
		View_MainCameraPos.z = 0.0f; //CAMERA_OFFSET_Z
		fCameraFOV = 45.0f;

		fCameraPitch = 0.0f; //CAMERA_OFFSET_PITCH
		fCameraYaw = 0.0f; //CAMERA_OFFSET_YAW

	//Lighting and colors
		lightColor.r = 0.8f;
		lightColor.g = 0.8f;
		lightColor.b = 0.8f;

		lightPos = glm::vec3(100.0, 800.0f, -700.0f);

		bg_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//Load models
		BodyModel = new Model("./res/CarveraBody.stl");
		CarriageModel = new Model("./res/CarveraCarriage.3mf");
		BedModel = new Model("./res/CarveraBed.stl");
		SpindleModel = new Model("./res/CarveraSpindle.3mf");

	//Load shaders
		Shader_Main = new Shader("./res/MainView_Vert.glsl", "./res/MainView_Frag.glsl");
		Shader_ShadowMap = new Shader("./res/ShadowMap_Vert.glsl", 0); //No fragment shader needed since we're not rendering any color data
		Shader_ShadowMapViewer = new Shader("./res/ShadowMapViewer_Vert.glsl", "./res/ShadowMapViewer_Frag.glsl");

	//Render to texture
		Framebuffer = 0;
		glGenFramebuffers(1, &Framebuffer);	//Create a new framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer); //Bind the curent framebuffer to this new one

		// Generate the texture we're going to render to
			glGenTextures(1, &RenderTargetTexture);
			glBindTexture(GL_TEXTURE_2D, RenderTargetTexture);

		//Get the current window size
			WindowSize = ImVec2(640, 480); //The window isn't created yet, use a default size.  It'll get changed after the first pass through rendering.

		// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)WindowSize.x, (GLsizei)WindowSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		// Poor filtering. Needed !
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		//Enable culling of faces for better performance
			glEnable(GL_CULL_FACE); 
		
		// Create the depth buffer
			glGenRenderbuffers(1, &DepthBuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)WindowSize.x, (GLsizei)WindowSize.y);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, DepthBuffer);
		
		// Set "RenderTargetTexture" as our colour attachement #0
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, RenderTargetTexture, 0);

		// Set the list of draw buffers.
			GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

		//Setup shadow mapping
			//Generate the shadow map framebuffer
				glGenFramebuffers(1, &ShadowMapFramebuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, ShadowMapFramebuffer);

			//Generate the depthmap texture
				glGenTextures(1, &ShadowMapTexture);
				glBindTexture(GL_TEXTURE_2D, ShadowMapTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (GLsizei)(WindowSize.x * SHADOW_MAP_SIZE_FACTOR), (GLsizei)(WindowSize.y * SHADOW_MAP_SIZE_FACTOR), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); //This 640x480 is just a placeholder.  It gets resized later.
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			//Over sampling - softer shadow edges
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			//Attach the texture to this framebuffer
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ShadowMapTexture, 0);
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MODEL_VIEWER::Destroy()
{
	if (BodyModel != 0)
		delete BodyModel;
	if (CarriageModel != 0)
		delete CarriageModel;
	if (BedModel != 0)
		delete BedModel;
	if (SpindleModel != 0)
		delete SpindleModel;
	if (Shader_Main != 0)
		delete Shader_Main;
	if (Shader_ShadowMap != 0)
		delete Shader_ShadowMap;
	if (Shader_ShadowMapViewer != 0)
		delete Shader_ShadowMapViewer;
}

//Render the scene from the current camera view, using the given shader
void MODEL_VIEWER::RenderScene(Shader* shader)
{
	//Calculate 3D origin offset
		//TODO: This must be calibrated to MCS, which is not 0,0,0 at origin
		OriginOffset = glm::vec3(MachineStatus.Coord.Machine.x, MachineStatus.Coord.Machine.y, MachineStatus.Coord.Machine.z + MachineStatus.fToolLengthOffset);

		if (MachineStatus.Status == Carvera::Status::Homing) //The machine doesn't know where it is while homing, so don't show anything
			OriginOffset.x = 0;

		if (MachineStatus.Status == Carvera::Status::Homing) //The machine doesn't know where it is while homing, so don't show anything
			OriginOffset.z = 200;

	//Send data to the shaders
		shader->use();
		shader->setMat4("projection", ProjectionMatrix);
		shader->setMat4("view", ViewMatrix);
		shader->setMat4("lightSpaceMatrix", LightSpaceMatrix);

		shader->setVec3("lightPos", lightPos);
		shader->setVec3("lightColor", lightColor);
		
		shader->setVec3("cameraPos", glm::vec3(ActiveCameraPos.x, ActiveCameraPos.y, ActiveCameraPos.z));

		shader->setFloat("Lighting_Ambient", 0.2f);
		shader->setFloat("Lighting_Specular", 0.3f);

	//Draw main Carvera body
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-160.0f, -100.0f, 350.0f));
		shader->setMat4("model", ModelMatrix);
		
		shader->setVec3("objectColor", glm::vec3(0.6f, 0.6f, 0.6f));

		BodyModel->Draw(*shader);

	//Draw Carriage		
		ModelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(17, 80, -80));  //Put the model on the 3D origin
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-OriginOffset.x, 0, 0));  //Move the carriage to its current position			
		shader->setMat4("model", ModelMatrix);

		shader->setVec3("objectColor", glm::vec3(0.4f, 0.4f, 0.4f));

		CarriageModel->Draw(*shader);

	//Draw Spindle
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(OriginOffset.x, OriginOffset.z, 0));  //Move the carriage to its current position
		shader->setMat4("model", ModelMatrix);

		shader->setVec3("objectColor", glm::vec3(0.7f, 0.7f, 0.7f));

		SpindleModel->Draw(*shader);

	//Draw Bed
		ModelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-15, -15, 31.6f)); //Translate down so 0,0,0 is at home position.   //TODO: Fix the model so 0,0,0 is at the right spot
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, -OriginOffset.y, 0)); //Translate so the y axis moves relative to the origin, not the other way around 
		shader->setMat4("model", ModelMatrix);

		shader->setVec3("objectColor", glm::vec3(0.2f, 0.2f, 0.2f));

		BedModel->Draw(*shader);
}

//Draw the model viewer page in ImGui
void MODEL_VIEWER::Draw()
{
	ImGuiIO& io = ImGui::GetIO();

	//Draw the 3D scene onto a texture
		//Check if the window size has changed (NewWindowSize is updated down below, where the window is actually drawn)
			if (WindowSize.x != NewWindowSize.x || WindowSize.y != NewWindowSize.y)
				ResizeWindow();

		//Render first pass from the light's perspective for shadows
			//Switch to the shadow map framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, ShadowMapFramebuffer);
				glClear(GL_DEPTH_BUFFER_BIT);
				glCullFace(GL_FRONT); //Fix peter-panning

			//Setup the viewport
				glViewport(0, 0, (GLsizei)(WindowSize.x * SHADOW_MAP_SIZE_FACTOR), (GLsizei)(WindowSize.y * SHADOW_MAP_SIZE_FACTOR));

			//Calculate the view matrixes
				glm::mat4 lightProjection = glm::ortho( - 500.0f, 500.0f, -500.0f, 500.0f, CLIP_PLANE_NEAR, CLIP_PLANE_FAR);
				glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), cameraUp);
				LightSpaceMatrix = lightProjection * lightView;

			//Render the scene
				RenderScene(Shader_ShadowMap);

			//Switch to culling the rear face for normal rendering
				glCullFace(GL_BACK);

		//2nd Pass, draw the scene from the camera view
			//Recalculate where the camera is looking
				if (iDebug_SelectedView == 0) //Which view are we using?
				{
					glm::vec3 direction;
					direction.x = cos(glm::radians(-(fCameraYaw + CAMERA_OFFSET_YAW))) * cos(glm::radians(fCameraPitch + CAMERA_OFFSET_PITCH));
					direction.y = sin(glm::radians(fCameraPitch + CAMERA_OFFSET_PITCH));
					direction.z = sin(glm::radians(-(fCameraYaw + CAMERA_OFFSET_YAW))) * cos(glm::radians(fCameraPitch + CAMERA_OFFSET_PITCH));
					cameraFront = glm::normalize(direction);

					ActiveCameraPos = glm::vec3(View_MainCameraPos.x + CAMERA_OFFSET_X, View_MainCameraPos.y + CAMERA_OFFSET_Y, View_MainCameraPos.z + CAMERA_OFFSET_Z);
					ViewMatrix = glm::lookAt(ActiveCameraPos, ActiveCameraPos + cameraFront, cameraUp);

					ProjectionMatrix = glm::perspective(glm::radians(fCameraFOV), WindowSize.x / WindowSize.y, CLIP_PLANE_NEAR, CLIP_PLANE_FAR);
				}
				else if (iDebug_SelectedView == 1) //Light source
				{
					ActiveCameraPos = lightPos;
					ViewMatrix = glm::lookAt(ActiveCameraPos, glm::vec3(0.0f, 0.0f, 0.0f), cameraUp);

					ProjectionMatrix = glm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, CLIP_PLANE_NEAR, CLIP_PLANE_FAR);
				}
				
		//Switch to the main framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer); //Render to our texture framebuffer
			glViewport(0, 0, (GLsizei)WindowSize.x, (GLsizei)WindowSize.y); // Render on the whole framebuffer, complete from the lower left corner to the upper right
			glClearColor(bg_color.x * bg_color.w, bg_color.y * bg_color.w, bg_color.z * bg_color.w, bg_color.w);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Bind the shadow map
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ShadowMapTexture);

		//Render from the camera's perspective
			RenderScene(Shader_Main);

		//Draw debug stuff
			if (bDebug_DrawDebugData)
			{
				DrawGrid();
				DrawOrigin();
				DrawLightSource();
			}

		//Draw the depth buffer from the light's view if requested
			if (bDebug_DrawShadowMap)
			{
				glViewport(0, 0, (GLsizei)WindowSize.x, (GLsizei)WindowSize.y);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				Shader_ShadowMapViewer->use();

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, ShadowMapTexture);
				renderQuad();
			}

		// Disable to avoid OpenGL reading from arrays bound to an invalid ptr
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);

	//Unbind the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//The preview window itself
		if (ImGui::Begin("Preview"))
		{
			//Save the window size for next time through.  We draw the texture before creating the window so we'll resize it next time around.
				NewWindowSize = ImGui::GetContentRegionAvail();

			ImGui::Image((void*)(intptr_t)RenderTargetTexture, WindowSize);
				
			//Keyboard control
				if (ImGui::IsWindowFocused())
				{
					const float cameraSpeed = 10.0f; // adjust accordingly
					if (ImGui::IsKeyDown((ImGuiKey)515)) //Up arrow
						View_MainCameraPos += cameraSpeed * cameraFront;
					else if (ImGui::IsKeyDown((ImGuiKey)516)) //Up arrow
						View_MainCameraPos -= cameraSpeed * cameraFront;
					if (ImGui::IsKeyDown((ImGuiKey)513)) //Left arrow
						View_MainCameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
					else if (ImGui::IsKeyDown((ImGuiKey)514)) //Right arrow
						View_MainCameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
				}

			//Mouse viewing
				if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) //ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0))
				{
					//Hide the cursor while we're dragging
						ImGui::SetMouseCursor(ImGuiMouseCursor_None);

					//If we're using mouse look, make this window focused so we can use keyboard shortcuts as well
						if (ImGui::IsWindowFocused() == false)
							ImGui::SetWindowFocus();

					if (fabs(io.MousePos.x - MouseStartPos.x) > 0 || fabs(io.MousePos.y - MouseStartPos.y) > 0) //ImGui::IsMouseDragging() doesn't work here when we're changing the position of the cursor
					{
						fCameraPitch -= io.MouseDelta.y * 0.2f;
						fCameraYaw += io.MouseDelta.x * 0.2f;
						io.MousePos = MouseStartPos;
						io.WantSetMousePos = true;
					}
				}
				else
					MouseStartPos = io.MousePos;
		}
		ImGui::End();
		
	//Preview controls
		if (ImGui::Begin("Preview Controls")) 
		{
			ImGui::SameLine();
			ImGui::ColorEdit4("Color", (float*)&bg_color);

			//X
				ImGui::SliderFloat("Left/Right", &View_MainCameraPos.x, -200.0f, 200.0f);

			//Y
				ImGui::SliderFloat("Up/Down", &View_MainCameraPos.y, -300.0f, 300.0f);

			//Z
				ImGui::SliderFloat("In/Out", &View_MainCameraPos.z, -500.0f, 500.0f);

			//Pitch
				ImGui::SliderFloat("Pitch", &fCameraPitch, -90, 90);

			//Yaw
				ImGui::SliderFloat("Yaw", &fCameraYaw, -360.0f, 360);

			//FOV
				ImGui::SliderFloat("FOV", &fCameraFOV, 69.0f, 4.20f);

			//Reset
				if (ImGui::Button("Reset View"))
				{
					View_MainCameraPos.x = 0.0f;
					View_MainCameraPos.y = 0.0f;
					View_MainCameraPos.z = 0.0f;
					fCameraYaw = 0.0f;
					fCameraPitch = 0.0f;
					fCameraFOV = 45.0f;
				}

#ifndef NDEBUG
				const char szViewChoices[][15] = { "Main", "Light Source" };
				if (ImGui::BeginCombo("Active view", szViewChoices[iDebug_SelectedView]))
				{
					for (int x = 0; x < 2; x++)
					{
						//Add the item
						const bool is_selected = (iDebug_SelectedView == x);
						if (ImGui::Selectable(szViewChoices[x], is_selected))
							iDebug_SelectedView = x;

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				
				ImGui::Checkbox("Draw Debug Data", &bDebug_DrawDebugData);
				ImGui::Checkbox("Draw Shadow Map", &bDebug_DrawShadowMap);
#endif

		}
		ImGui::End();

}