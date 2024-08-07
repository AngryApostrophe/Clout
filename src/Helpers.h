#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>



#define GuiFileDialog	ImGuiFileDialog::Instance()

bool LoadTextureFromFile(const char* filename, GLuint* out_texture);
bool LoadCompressedTextureFromMemory(const unsigned char* buf_data, const unsigned int size, GLuint* out_texture);

void CommaStringTo3Doubles(char* str, double* d1, double* d2, double* d3);
void CommaStringTo3Ints(char* str, int* d1, int* d2, int* d3);

ImVec2 ScaledByFontSize(float x, float y);
float ScaledByFontSize(float in);
ImVec2 ScaledByWindowScale(float x, float y);
ImVec2 ScaledByWindowScale(ImVec2 size);
float ScaledByWindowScale(float in);

void CenterTextInColumn(char* s);	//Sets cursor position to center the string in the current column

#if defined(_CHRONO_) || defined(_GLIBCXX_CHRONO)
double TimeSince_ms(const std::chrono::steady_clock::time_point StartTime);
#endif

struct DOUBLE_XYZ
{
	double x;
	double y;
	double z;
};