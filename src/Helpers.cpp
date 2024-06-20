//#include <Windows.h>
using namespace std;

#include "Platforms/Platforms.h"

#include <chrono>

#include "imgui.h"
#include "Clout.h"

#include <stb_image.h>
#include <deprecated/stb.h> //This is supposedly going away, but it works for now.  He said that years ago.



//Return time elapsed (in ms) since a previous time
double TimeSince_ms(const std::chrono::steady_clock::time_point StartTime)
{
	const std::chrono::duration<double, std::milli> ElapsedTime = std::chrono::steady_clock::now() - StartTime;
	return ElapsedTime.count();
}

//Scale object sizes based on font size.  This is better than hard-coding pixels because things change depending on OS
ImVec2 ScaledByFontSize(float x, float y)
{
	return ImVec2(x * ImGui::GetFontSize(), y * ImGui::GetFontSize());
}

float ScaledByFontSize(float in)
{
	return in * ImGui::GetFontSize();
}

//This works based on glfw window scaling.
ImVec2 ScaledByWindowScale(float x, float y)
{
	float xscale, yscale;
	glfwGetWindowContentScale(glfwWindow, &xscale, &yscale);

	return ImVec2(x * xscale, y * yscale);
}
ImVec2 ScaledByWindowScale(ImVec2 size)
{
	float xscale, yscale;
	glfwGetWindowContentScale(glfwWindow, &xscale, &yscale);

	return ImVec2(size.x * xscale, size.y * yscale);
}
float ScaledByWindowScale(float in)
{
	float xscale, yscale;
	glfwGetWindowContentScale(glfwWindow, &xscale, &yscale);

	return in * xscale;
}


//Sets cursor position to center the string in the current column
void CenterTextInColumn(char *s)
{
	ImVec2 TextSize = ImGui::CalcTextSize(s);
	float fWidth = ImGui::GetColumnWidth();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((fWidth - TextSize.x) * 0.5f));
}


// Simple helper function to load an image into a OpenGL texture with common settings
static GLuint LoadTextureCommon()
{
	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

	return image_texture;
}

bool LoadTextureFromFile(const char* filename, GLuint* out_texture)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	*out_texture = LoadTextureCommon();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	return true;
}
// Simple helper function to load an image from memory into a OpenGL texture with common settings
bool LoadCompressedTextureFromMemory(const unsigned char* buf_data, const unsigned int size, GLuint * out_texture)
{
	if (buf_data == NULL)
		return false;

	//First decompress the image data
		const unsigned int buf_decompressed_size = stb_decompress_length((stb_uchar*)buf_data);
		unsigned char* buf_decompressed_data = (unsigned char*)malloc(buf_decompressed_size);
		stb_decompress(buf_decompressed_data, (stb_uchar*)buf_data, (unsigned int)size);

	if (buf_decompressed_data == NULL)
		return false;

	//Now load it from memory
		int c;
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load_from_memory(buf_decompressed_data, buf_decompressed_size, &image_width, &image_height, &c, 0);

	*out_texture = LoadTextureCommon(); 
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);
	stbi_image_free(buf_decompressed_data);

	return true;
}


void CommaStringTo3Doubles(char* str, double* d1, double* d2, double* d3)
{
	char* c = str;
	char sTemp[100];

	//1st value
	memset(sTemp, 0, 100);

	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat(sTemp, c, 1);
		c++;
	}

	*d1 = atof(sTemp);

	//2nd value
	memset(sTemp, 0, 100);
	c++; //Probably left off last time at a "," separating numbers
	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat(sTemp, c, 1);
		c++;
	}

	*d2 = atof(sTemp);

	//3rd value
	memset(sTemp, 0, 100);
	c++; //Probably left off last time at a "," separating numbers
	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat(sTemp, c, 1);
		c++;
	}

	*d3 = atof(sTemp);
}

void CommaStringTo3Ints(char* str, int* d1, int* d2, int* d3)
{
	char* c = str;
	char sTemp[100];

	//1st value
	memset(sTemp, 0, 100);

	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat(sTemp, c, 1);
		c++;
	}

	*d1 = atoi(sTemp);

	//2nd value
	memset(sTemp, 0, 100);
	c++; //Probably left off last time at a "," separating numbers
	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat(sTemp, c, 1);
		c++;
	}

	*d2 = atoi(sTemp);

	//3rd value
	memset(sTemp, 0, 100);
	c++; //Probably left off last time at a "," separating numbers
	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat(sTemp, c, 1);
		c++;
	}

	*d3 = atoi(sTemp);
}