#include <Windows.h>
using namespace std;

#include <stb_image.h>

#include "imgui.h"

#include "Helpers.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;

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
		strncat_s(sTemp, 100, c, 1);
		c++;
	}

	*d1 = atof(sTemp);

	//2nd value
	memset(sTemp, 0, 100);
	c++; //Probably left off last time at a "," separating numbers
	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat_s(sTemp, 100, c, 1);
		c++;
	}

	*d2 = atof(sTemp);

	//3rd value
	memset(sTemp, 0, 100);
	c++; //Probably left off last time at a "," separating numbers
	while (*c == '-' || *c == '+' || *c == '.' || (*c >= '0' && *c <= '9')) //Only copy out the number characters
	{
		strncat_s(sTemp, 100, c, 1);
		c++;
	}

	*d3 = atof(sTemp);
}