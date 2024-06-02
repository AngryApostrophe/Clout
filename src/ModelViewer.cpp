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

//Camera
#define CAMERA_OFFSET_X		140
#define CAMERA_OFFSET_Y		260
#define CAMERA_OFFSET_Z		-780
#define CAMERA_OFFSET_YAW	-90

bool bDrawDebugData = false;


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
	const float fStep = 20;
	ModelMatrix = glm::mat4(1.0f); //Reset all model matrix values
	CloutShader->setMat4("model", ModelMatrix);

	objectColor = glm::vec3(0.0f, 0.0f, 0.0f);
	CloutShader->setVec3("objectColor", objectColor);
	CloutShader->setFloat("Lighting_Ambient", 1.0f);

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
	ModelMatrix = glm::mat4(1.0f); //Reset all model matrix values
	CloutShader->setMat4("model", ModelMatrix);

	CloutShader->setFloat("Lighting_Ambient", 1.0f);

	//Draw the cube (TODO: this may draw weird because the shader is expecting normals, which we're not providing right now
	CloutShader->setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
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
			CloutShader->setVec3("objectColor", glm::vec3(0.0f, 0.0f, 1.0f));
			glBegin(GL_LINES);
				glVertex3f(0, 0, 0);
				glVertex3f(0, 100, 0);
			glEnd();

		//Right (red)
			CloutShader->setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
			glBegin(GL_LINES);
				glVertex3f(0, 0, 0);
				glVertex3f(100, 0, 0);
			glEnd();

		//In (green)
			CloutShader->setVec3("objectColor", glm::vec3(0.0f, 1.0f, 0.0f));
			glBegin(GL_LINES);
				glVertex3f(0, 0, 0);
				glVertex3f(0, 0, 100);
			glEnd();
}

//Draw a cube where the 3D light is positioned
void MODEL_VIEWER::DrawLightSource()
{
	ModelMatrix = glm::mat4(1.0f); //Reset all model matrix values	

	CloutShader->setFloat("Lighting_Ambient", 1.0f);
	CloutShader->setFloat("Lighting_Specular", 0.0f);

	//Move to the light position
		ModelMatrix = glm::translate(ModelMatrix, lightPos);
		CloutShader->setMat4("model", ModelMatrix);

	//Draw the cube (TODO: this may draw weird because the shader is expecting normals, which we're not providing right now
		CloutShader->setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
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

	WindowSize = NewWindowSize;

	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WindowSize.x, WindowSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WindowSize.x, WindowSize.y);
}

void MODEL_VIEWER::Init()
{	
	//Setup some OpenGL stuff
		glewExperimental = true;
		glewInit();
		glEnable(GL_DEPTH_TEST);
			
	//Setup camera stuff
		cameraPos = glm::vec3(0.0f, 0.0f, -100.0f);
		cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		cameraDirection = glm::normalize(cameraPos - cameraTarget);
		cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
		up = glm::vec3(0.0f, -1.0f, 0.0f);
		cameraRight = glm::normalize(glm::cross(up, cameraDirection));
		cameraUp = glm::cross(cameraDirection, cameraRight);

		cameraPos.x = 0.0f; //CAMERA_OFFSET_X
		cameraPos.y = 0.0f; //CAMERA_OFFSET_Y
		cameraPos.z = 0.0f; //CAMERA_OFFSET_Z
		fCameraFOV = 45.0f;

		fCameraPitch = -14.0f;
		fCameraYaw = 0.0f; //CAMERA_OFFSET_YAW

	//Lighting and colors
		lightColor.r = 0.8f;
		lightColor.g = 0.8f;
		lightColor.b = 0.8f;

		lightPos = glm::vec3(100.0f, 500.0f, -800.0f);

		bg_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//Models
		BodyModel = new Model("./res/CarveraBody.stl");
		CarriageModel = new Model("./res/CarveraCarriage.3mf");
		BedModel = new Model("./res/CarveraBed.stl");
		SpindleModel = new Model("./res/CarveraSpindle.3mf");

	//Shaders
		CloutShader = new Shader("./res/vshader.glsl", "./res/fshader.glsl");

	//Render to texture
		FramebufferName = 0;
		glGenFramebuffers(1, &FramebufferName);	//Create a new framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName); //Bind the curent framebuffer to this new one

		glEnable(GL_CULL_FACE); //Enable culling of faces for better performance

		// Generate the texture we're going to render to
			glGenTextures(1, &renderedTexture);
			glBindTexture(GL_TEXTURE_2D, renderedTexture);

		//Get the current window size
			WindowSize = ImVec2(640, 480); //The window isn't created yet, use a default size.  It'll get changed after the first pass through rendering.

		// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WindowSize.x, WindowSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		// Poor filtering. Needed !
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		// The depth buffer
			glGenRenderbuffers(1, &depthrenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WindowSize.x, WindowSize.y);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);
		
		// Set "renderedTexture" as our colour attachement #0
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

		// Set the list of draw buffers.
			GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

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
	if (CloutShader != 0)
		delete CloutShader;
}

void MODEL_VIEWER::Draw()
{
	ImGuiIO& io = ImGui::GetIO();

	//Draw the 3D scene onto a texture
		//Check if the window size has changed (NewWindowSize is updated down below, where the window is actually drawn)
			if (WindowSize.x != NewWindowSize.x || WindowSize.y != NewWindowSize.y)
				ResizeWindow();

		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0, 0, WindowSize.x, WindowSize.y); // Render on the whole framebuffer, complete from the lower left corner to the upper right
		glClearColor(bg_color.x * bg_color.w, bg_color.y * bg_color.w, bg_color.z * bg_color.w, bg_color.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ProjectionMatrix = glm::perspective(glm::radians(fCameraFOV), WindowSize.x / WindowSize.y, 10.0f, 10000.0f);

		//Recalculate where the camera is looking
			glm::vec3 direction;
			direction.x = cos(glm::radians(-(fCameraYaw+ CAMERA_OFFSET_YAW))) * cos(glm::radians(fCameraPitch));
			direction.y = sin(glm::radians(fCameraPitch));
			direction.z = sin(glm::radians(-(fCameraYaw+ CAMERA_OFFSET_YAW))) * cos(glm::radians(fCameraPitch));
			cameraFront = glm::normalize(direction);
			
		//Calculate the camera view matrix		
			ViewMatrix = glm::lookAt(glm::vec3(cameraPos.x+CAMERA_OFFSET_X, cameraPos.y + CAMERA_OFFSET_Y, cameraPos.z + CAMERA_OFFSET_Z), glm::vec3(cameraPos.x + CAMERA_OFFSET_X, cameraPos.y + CAMERA_OFFSET_Y, cameraPos.z + CAMERA_OFFSET_Z) + cameraFront, cameraUp);

		//Calculate 3D origin offset
				//TODO: This must be calibrated to MCS, which is not 0,0,0 at origin
			OriginOffset = glm::vec3(MachineStatus.Coord.Machine.x, MachineStatus.Coord.Machine.y, MachineStatus.Coord.Machine.z + MachineStatus.fToolLengthOffset);

			if (MachineStatus.Status == Carvera::Status::Homing) //The machine doesn't know where it is while homing, so don't show anything
				OriginOffset.x = 0;

			if (MachineStatus.Status == Carvera::Status::Homing) //The machine doesn't know where it is while homing, so don't show anything
				OriginOffset.z = 200;

		//Send data to the shaders
			CloutShader->use();
			CloutShader->setMat4("projection", ProjectionMatrix);
			CloutShader->setMat4("view", ViewMatrix);

			CloutShader->setVec3("lightPos", lightPos);
			CloutShader->setVec3("lightColor", lightColor);
			CloutShader->setVec3("cameraPos", glm::vec3(cameraPos.x + CAMERA_OFFSET_X, cameraPos.y + CAMERA_OFFSET_Y, cameraPos.z + CAMERA_OFFSET_Z));

			CloutShader->setFloat("Lighting_Ambient", 0.2f);
			CloutShader->setFloat("Lighting_Specular", 0.3f);

		//Draw main Carvera body
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-160.0f, -100.0f, 350.0f));
			CloutShader->setMat4("model", ModelMatrix);
			CloutShader->setVec3("objectColor", glm::vec3(0.6f, 0.6f, 0.6f));
			BodyModel->Draw(*CloutShader);

		//Draw Carriage		
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(17, 80, -80));  //Put the model on the 3D origin
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-OriginOffset.x, 0, 0));  //Move the carriage to its current position			
			CloutShader->setMat4("model", ModelMatrix);

			objectColor = glm::vec3(0.4f, 0.4f, 0.4f);
			CloutShader->setVec3("objectColor", objectColor);

			CarriageModel->Draw(*CloutShader);

		//Draw Spindle
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(OriginOffset.x, OriginOffset.z, 0));  //Move the carriage to its current position
			CloutShader->setMat4("model", ModelMatrix);

			objectColor = glm::vec3(0.7f, 0.7f, 0.7f);
			CloutShader->setVec3("objectColor", objectColor);

			SpindleModel->Draw(*CloutShader);

		//Draw Bed
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-15, -15, 31.6f)); //Translate down so 0,0,0 is at home position.   //TODO: Fix the model so 0,0,0 is at the right spot
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0, -OriginOffset.y, 0)); //Translate so the y axis moves relative to the origin, not the other way around 
			CloutShader->setMat4("model", ModelMatrix);

			objectColor = glm::vec3(0.2f, 0.2f, 0.2f);
			CloutShader->setVec3("objectColor", objectColor);

			BedModel->Draw(*CloutShader);

		//Draw debug stuff
			if (bDrawDebugData)
			{
				DrawGrid();
				DrawOrigin();
				DrawLightSource();
			}

		// Disable to avoid OpenGL reading from arrays bound to an invalid ptr
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);

	//Switch back to the GUI window
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//The preview window itself
		if (ImGui::Begin("Preview"))
		{
			//Save the window size for next time through.  We draw the texture before creating the window so we'll resize it next time around.
				NewWindowSize = ImGui::GetContentRegionAvail();

			ImGui::Image((void*)(intptr_t)renderedTexture, WindowSize);
				
			//Keyboard controls, only while mouse hovering for now
				if (ImGui::IsItemHovered())
				{
					const float cameraSpeed = 10.0f; // adjust accordingly
					if (ImGui::IsKeyDown((ImGuiKey)515)) //Up arrow
						cameraPos += cameraSpeed * cameraFront;
					else if (ImGui::IsKeyDown((ImGuiKey)516)) //Up arrow
						cameraPos -= cameraSpeed * cameraFront;
					if (ImGui::IsKeyDown((ImGuiKey)513)) //Left arrow
						cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
					else if (ImGui::IsKeyDown((ImGuiKey)514)) //Right arrow
						cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
				}

			//Mouse viewing
				if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
				{
					fCameraPitch += io.MouseDelta.y * 0.1f;
					fCameraYaw += io.MouseDelta.x * 0.1f;
				}
		}
		ImGui::End();
		
	//Preview controls
		if (ImGui::Begin("Preview Controls")) 
		{
			ImGui::SameLine();
			ImGui::ColorEdit4("Color", (float*)&bg_color);

			//X
				if (ImGui::Button("Reset##LeftRight"))
					cameraPos.x = 0.0f;
				ImGui::SameLine();
				ImGui::SliderFloat("Left/Right", &cameraPos.x, -200.0f, 200.0f);

			//Y
				if (ImGui::Button("Reset##UpDown"))
					cameraPos.y = 0.0f;
				ImGui::SameLine();
				ImGui::SliderFloat("Up/Down", &cameraPos.y, -300.0f, 300.0f);

			//Z
				if (ImGui::Button("Reset##InOut"))
					cameraPos.z = 0.0f;
				ImGui::SameLine();
				ImGui::SliderFloat("In/Out", &cameraPos.z, -500.0f, 500.0f);

			//Yaw
				if (ImGui::Button("Reset##Yaw"))
					fCameraYaw = 0.0f;
				ImGui::SameLine();
				ImGui::SliderFloat("Yaw", &fCameraYaw, -360.0f, 360);

			//FOV
				if (ImGui::Button("Reset"))
					fCameraFOV = 45.0f;
				ImGui::SameLine();
				ImGui::SliderFloat("FOV", &fCameraFOV, 69.0f, 4.20f);

				ImGui::Checkbox("Debug Data", &bDrawDebugData);
		}
		ImGui::End();

}