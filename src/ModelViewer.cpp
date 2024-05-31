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



void MODEL_VIEWER::Init()
{	
	//Setup some OpenGL stuff
		glewExperimental = true;
		glewInit();
		glEnable(GL_DEPTH_TEST);

	//Setup the 3D matrixes
		ModelMatrix = glm::mat4(1.0f);
		ModelMatrix = glm::rotate(ModelMatrix, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		ViewMatrix = glm::mat4(1.0f);
		ViewMatrix = glm::translate(ViewMatrix, glm::vec3(0.0f, 0.0f, -10.0f));

		ProjectionMatrix = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
	
	//Setup camera stuff
		cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
		cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		cameraDirection = glm::normalize(cameraPos - cameraTarget);
		cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		cameraRight = glm::normalize(glm::cross(up, cameraDirection));
		cameraUp = glm::cross(cameraDirection, cameraRight);

		cameraPos.x = -1.7f;
		cameraPos.y = -2.2f;
		cameraPos.z = 7.0f;
		fCameraFOV = 45.0f;

		fCameraPitch = 14.0f;
		fCameraYaw = -90.0f;

	//Lighting and colors
		objectColor.r = 0.5f;
		objectColor.g = 0.5f;
		objectColor.b = 0.5f;

		lightColor.r = 0.8f;
		lightColor.g = 0.8f;
		lightColor.b = 0.8f;

		lightPos = glm::vec3(5.0f, 10.0f, 30.0f);

		bg_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//Models
		BodyModel = new Model("./res/CarveraBody.stl");
		CarriageModel = new Model("./res/CarveraCarriage.stl");
		BedModel = new Model("./res/CarveraBed.stl");

	//Shaders
		BodyShader = new Shader("./res/vshader.glsl", "./res/fshader.glsl");


	//Render to texture
		FramebufferName = 0;
		glGenFramebuffers(1, &FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

		// The texture we're going to render to
			glGenTextures(1, &renderedTexture);

		// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, renderedTexture);

		// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		// Poor filtering. Needed !
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		// The depth buffer
			glGenRenderbuffers(1, &depthrenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 640, 480);
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
	if (BodyShader != 0)
		delete BodyShader;
}

void MODEL_VIEWER::Draw()
{
	ImGuiIO& io = ImGui::GetIO();

	


		//ImGui::IsMouseDragging(3) //Center button dragging
		



	//Draw the 3D scene on a texture
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0, 0, 640, 480); // Render on the whole framebuffer, complete from the lower left corner to the upper right
		glClearColor(bg_color.x * bg_color.w, bg_color.y * bg_color.w, bg_color.z * bg_color.w, bg_color.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ProjectionMatrix = glm::perspective(glm::radians(fCameraFOV), 640.0f / 480.0f, 0.1f, 100.0f);

		//Recalculate where the camera is looking
			glm::vec3 direction;
			direction.x = cos(glm::radians(fCameraYaw)) * cos(glm::radians(fCameraPitch));
			direction.y = sin(glm::radians(fCameraPitch));
			direction.z = sin(glm::radians(fCameraYaw)) * cos(glm::radians(fCameraPitch));
			cameraFront = glm::normalize(direction);
			
		//Calculate the camera view matrix		
			ViewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		//Send data to the shaders
			BodyShader->use();
			BodyShader->setMat4("projection", ProjectionMatrix);
			BodyShader->setMat4("view", ViewMatrix);

			BodyShader->setVec3("lightPos", lightPos);
			BodyShader->setVec3("lightColor", lightColor);
			BodyShader->setVec3("viewPos", cameraPos);

		//Main body
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.006f, 0.006f, 0.006f));	// it's a bit too big for our scene, so scale it down
			BodyShader->setMat4("model", ModelMatrix);

			objectColor = glm::vec3(0.6f, 0.6f, 0.6f);
			BodyShader->setVec3("objectColor", objectColor);

			BodyModel->Draw(*BodyShader);

		//Carriage		
			float xpos = MachineStatus.Coord.Machine.x;
			if (MachineStatus.Status == Carvera::Status::Homing) //The machine doesn't know where it is while homing, so don't show anything
				xpos = 0;
			xpos = ((1.0f - (xpos / -360.0f)) * 2.0) + -3;  //2 units is full travel (-3 to -1)
		
			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(xpos, 1.4f, 0.3f)); //This is XZY?
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.006f, 0.006f, 0.006f));	// it's a bit too big for our scene, so scale it down
			BodyShader->setMat4("model", ModelMatrix);

			objectColor = glm::vec3(0.4f, 0.4f, 0.4f);
			BodyShader->setVec3("objectColor", objectColor);

			CarriageModel->Draw(*BodyShader);

		//Bed
			//235mm?
			float ypos = MachineStatus.Coord.Machine.y;
			if (MachineStatus.Status == Carvera::Status::Homing) //The machine doesn't know where it is while homing, so don't show anything
				ypos = 0;
			ypos = ((1.0f-(ypos / -235.0f)) * 0.8f) + 1;

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			//ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-2.6, 1.85f, 0.5f)); //This is XZY?
			//ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-2.6, 1.0f, 0.5f)); //This is XZY?
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-2.6, ypos, 0.5f)); //This is XZY?
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.006f, 0.006f, 0.006f));	// it's a bit too big for our scene, so scale it down
			BodyShader->setMat4("model", ModelMatrix);

			objectColor = glm::vec3(0.2f, 0.2f, 0.2f);
			BodyShader->setVec3("objectColor", objectColor);

			BedModel->Draw(*BodyShader);


		// Disable to avoid OpenGL reading from arrays bound to an invalid ptr
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);

	//Switch back to the GUI window
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//The preview window itself
			if (ImGui::Begin("Preview"))
			{
				ImGui::Image((void*)(intptr_t)renderedTexture, ImVec2(640, 480));
				
				//Keyboard controls, only while mouse hovering for now
					if (ImGui::IsItemHovered())
					{
						const float cameraSpeed = 0.005f; // adjust accordingly
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
					ImGui::SliderFloat("Left/Right", &cameraPos.x, -5.0f, 5.0f);

				//Y
					if (ImGui::Button("Reset##UpDown"))
						cameraPos.y = 0.0f;
					ImGui::SameLine();
					ImGui::SliderFloat("Up/Down", &cameraPos.y, -5.0f, 5.0f);

				//Z
					if (ImGui::Button("Reset##InOut"))
						cameraPos.z = 0.0f;
					ImGui::SameLine();
					ImGui::SliderFloat("In/Out", &cameraPos.z, 5.0f, 20.0f);

				//FOV
					if (ImGui::Button("Reset"))
						fCameraFOV = 45.0f;
					ImGui::SameLine();
					ImGui::SliderFloat("FOV", &fCameraFOV, 69.0f, 4.20f);
			}
			ImGui::End();

}