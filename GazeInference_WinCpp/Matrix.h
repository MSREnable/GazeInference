#pragma once
#include <math.h>


namespace GazeInference_WinCpp
{
	class Matrix
	{
	public:
		Matrix(int m, int n);

	private:
		~Matrix();

	public:
		int Rows()
		{
			return _m;
		}

		int Cols()
		{
			return _n;
		}

		int Size()
		{
			return _mn;
		}

		double Get(int i)
		{
			return _data[i];
		}

		void Set(int i, double value)
		{
			_data[i] = value;
		}

		double Get(int i, int j)
		{
			return _data[i * _n + j];
		}

		void Set(int i, int j, double value)
		{
			_data[i * _n + j] = value;
		}

		void Copy(int iThis, int jThis, Matrix* other, int iOther, int jOther, int rows, int cols)
		{
			for (int row = 0; row < rows; row++)
				for (int col = 0; col < cols; col++)
					Set(iThis + row, jThis + col, other->Get(iOther + row, jOther + col));
		}

		void Multiply(Matrix* a, Matrix* b)
		{
			for (int i = 0; i < a->Rows(); i++)
			{
				for (int j = 0; j < a->Cols(); j++)
				{
					Set(i, j, 0.0);
					for (int k = 0; k < a->Cols(); k++)
						Set(i, j, a->Get(i, k) * b->Get(k, j) + Get(i, j));
				}
			}
		}

		double Distance(Matrix* to)
		{
			double s = 0.0;
			double d;
			for (int i = 0; i < to->Size(); i++)
			{
				d = to->_data[i] - _data[i];
				s += d * d;
			}

			return sqrt(s);
		}


	private:



	private:

		int _m, _n, _mn;
		double* _data;
	};
}