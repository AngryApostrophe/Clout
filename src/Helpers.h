#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>



#define GuiFileDialog	ImGuiFileDialog::Instance()

bool LoadTextureFromFile(const char* filename, GLuint* out_texture);
void CommaStringTo3Doubles(char* str, double* d1, double* d2, double* d3);

ImVec2 ScaledByFontSize(float x, float y);
float ScaledByFontSize(float in);
ImVec2 ScaledByWindowScale(float x, float y);
ImVec2 ScaledByWindowScale(ImVec2 size);
float ScaledByWindowScale(float in);

struct DOUBLE_XYZ
{
	double x;
	double y;
	double z;
};