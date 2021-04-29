#include "Matrix.h"


GazeInference_WinCpp::Matrix::Matrix(int m, int n)
{
	_m = m;
	_n = n;
	_mn = m * n;
	_data = new double[_mn];
}

GazeInference_WinCpp::Matrix::~Matrix()
{
	delete _data;
}









