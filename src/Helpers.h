#pragma once

#include <GL/glew.h>
#include <GL/GL.h>

bool LoadTextureFromFile(const char* filename, GLuint* out_texture);
void CommaStringTo3Doubles(char* str, double* d1, double* d2, double* d3);

struct DOUBLE_XYZ
{
	double x;
	double y;
	double z;
};