/*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* Implementation of the general matrix class.
* Copied from //depot/rs2_release/multimedia/Photos/RawCodec/EngineLib/Effects/GenMatrix.cpp
* Enlistment dir:          os\src\multimedia\Photos\RawCodec\EngineLib\Effects\GenMatrix.cpp
*/

#include "pch.h"  
#include <math.h>  
#include "GenMatrix.h"  

//  Constructors - - - - - - - - - - - - - - - - - - - -  

GenMatrix::GenMatrix(int nRowCount, int nColumnCount)
	: m_nRowCount(nRowCount),
	m_nColumnCount(nColumnCount),
	m_pflData(NULL)
{
	//ATLASSERT((m_nRowCount > 0) && (m_nColumnCount > 0));
	m_pflData = new double[static_cast<unsigned int>(m_nRowCount * m_nColumnCount)];
	//ATLASSERT(m_pflData != NULL);
	::memset(m_pflData, 0, m_nRowCount * m_nColumnCount * sizeof(double));    // binary 0 is double 0.0 in IEEE format  
}

GenMatrix::GenMatrix(int nRowCount)
	: m_nRowCount(nRowCount),
	m_nColumnCount(1),
	m_pflData(NULL)
{
	//ATLASSERT(m_nRowCount > 0);
	m_pflData = new double[static_cast<unsigned int>(m_nRowCount)];
	//ATLASSERT(m_pflData != NULL);
	::memset(m_pflData, 0, m_nRowCount * sizeof(double));     // binary 0 is double 0.0 in IEEE format  
}

GenMatrix::GenMatrix(const GenMatrix& A)
{
	this->Copy(A);
}

// Destructor - - - - - - - - - - - - - - - - - - - -  

GenMatrix::~GenMatrix()
{
	delete[] m_pflData;
	m_pflData = NULL;
}

// Overloaded operators - - - - - - - - - - - - - - - - - - - -  

GenMatrix& GenMatrix::operator=(const GenMatrix& rhs)
{
	if (this != &rhs)
	{
		delete[] m_pflData;
		m_pflData = NULL;
		this->Copy(rhs);
	}
	return(*this);
}

double& GenMatrix::operator()(int nRow, int nColumn)
{
	//ATLASSERT((nRow >= 0) && (nRow < m_nRowCount) && (nColumn >= 0) && (nColumn < m_nColumnCount));
	return(m_pflData[m_nColumnCount * nRow + nColumn]);
}

const double& GenMatrix::operator()(int nRow, int nColumn) const
{
	//ATLASSERT((nRow >= 0) && (nRow < m_nRowCount) && (nColumn >= 0) && (nColumn < m_nColumnCount));
	return(m_pflData[m_nColumnCount * nRow + nColumn]);
}

double& GenMatrix::operator()(int nElement)
{
	//ATLASSERT((nElement >= 0) && (nElement < (m_nRowCount * m_nColumnCount)));
	return(m_pflData[nElement]);
}

const double& GenMatrix::operator()(int nElement) const
{
	//ATLASSERT((nElement >= 0) && (nElement < (m_nRowCount * m_nColumnCount)));
	return(m_pflData[nElement]);
}

const GenMatrix GenMatrix::operator*(const GenMatrix& rhs) const
{
	// R = this * rhs  
	//ATLASSERT(m_nColumnCount == rhs.m_nRowCount);
	int nRhsColumnCount = rhs.m_nColumnCount;
	GenMatrix R(m_nRowCount, nRhsColumnCount);   // note: constructor initializes to zero  
	double* pflResult = R.m_pflData;
	for (int i = 0; i < m_nRowCount; i++)
	{
		for (int j = 0; j < nRhsColumnCount; j++)
		{
			// compute the dot product of row 'i' of 'this' and column 'j' of 'rhs'  
			double* pflRow = m_pflData + i * m_nColumnCount;    // start of row 'i' in 'this'  
			double* pflColumn = rhs.m_pflData + j;              // start of column 'j' in 'rhs'  
			double flDotProduct = 0.0;                          // dot product accumulator  
			for (int k = 0; k < m_nColumnCount; k++, pflColumn += nRhsColumnCount)
			{
				flDotProduct += ((*pflRow++) * (*pflColumn));
			}
			*pflResult++ = flDotProduct;
		}
	}
	return(R);
}

const GenMatrix GenMatrix::operator+(const GenMatrix& rhs) const
{
	// R = this + rhs  
	//ATLASSERT((m_nRowCount == rhs.m_nRowCount) && (m_nColumnCount == rhs.m_nColumnCount));
	GenMatrix R(m_nRowCount, m_nColumnCount);
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		R.m_pflData[nElementCount] = m_pflData[nElementCount] + rhs.m_pflData[nElementCount];
	}
	return(R);
}

const GenMatrix GenMatrix::operator-(const GenMatrix& rhs) const
{
	// R = this - rhs  
	//ATLASSERT((m_nRowCount == rhs.m_nRowCount) && (m_nColumnCount == rhs.m_nColumnCount));
	GenMatrix R(m_nRowCount, m_nColumnCount);
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		R.m_pflData[nElementCount] = m_pflData[nElementCount] - rhs.m_pflData[nElementCount];
	}
	return(R);
}

GenMatrix& GenMatrix::operator*=(const GenMatrix& rhs)
{
	// this = this * rhs  
	//ATLASSERT(m_nColumnCount == rhs.m_nRowCount);
	int nRhsColumnCount = rhs.m_nColumnCount;
	GenMatrix R(m_nRowCount, nRhsColumnCount);   // note: constructor initializes to zero  
	double* pflResult = R.m_pflData;
	for (int i = 0; i < m_nRowCount; i++)
	{
		for (int j = 0; j < nRhsColumnCount; j++)
		{
			// compute the dot product of row 'i' of 'this' and column 'j' of 'rhs'  
			double* pflRow = m_pflData + i * m_nColumnCount;    // start of row 'i' in 'this'  
			double* pflColumn = rhs.m_pflData + j;              // start of column 'j' in 'rhs'  
			double flDotProduct = 0.0;                          // dot product accumulator  
			for (int k = 0; k < m_nColumnCount; k++, pflColumn += nRhsColumnCount)
			{
				flDotProduct += ((*pflRow++) * (*pflColumn));
			}
			*pflResult++ = flDotProduct;
		}
	}

	// replace 'this' matrix  
	delete[] m_pflData;
	m_pflData = NULL;
	this->Copy(R);
	return(*this);
}

GenMatrix& GenMatrix::operator+=(const GenMatrix& rhs)
{
	// this = this + rhs  
	//ATLASSERT((m_nRowCount == rhs.m_nRowCount) && (m_nColumnCount == rhs.m_nColumnCount));
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] += rhs.m_pflData[nElementCount];
	}
	return(*this);
}

GenMatrix& GenMatrix::operator-=(const GenMatrix& rhs)
{
	// this = this - rhs  
	//ATLASSERT((m_nRowCount == rhs.m_nRowCount) && (m_nColumnCount == rhs.m_nColumnCount));
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] -= rhs.m_pflData[nElementCount];
	}
	return(*this);
}

const GenMatrix GenMatrix::operator*(double rhs) const
{
	GenMatrix R(m_nRowCount, m_nColumnCount);
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		R.m_pflData[nElementCount] = m_pflData[nElementCount] * rhs;
	}
	return(R);
}

const GenMatrix GenMatrix::operator/(double rhs) const
{
	//ATLASSERT((rhs < -1.0e-20) || (rhs > 1.0e-20));
	double flRecip = 1.0 / rhs;
	GenMatrix R(m_nRowCount, m_nColumnCount);
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		R.m_pflData[nElementCount] = m_pflData[nElementCount] * flRecip;
	}
	return(R);
}

const GenMatrix GenMatrix::operator+(double rhs) const
{
	GenMatrix R(m_nRowCount, m_nColumnCount);
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		R.m_pflData[nElementCount] = m_pflData[nElementCount] + rhs;
	}
	return(R);
}

const GenMatrix GenMatrix::operator-(double rhs) const
{
	GenMatrix R(m_nRowCount, m_nColumnCount);
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		R.m_pflData[nElementCount] = m_pflData[nElementCount] - rhs;
	}
	return(R);
}

GenMatrix& GenMatrix::operator*=(double rhs)
{
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] *= rhs;
	}
	return(*this);
}

GenMatrix& GenMatrix::operator/=(double rhs)
{
	//ATLASSERT((rhs < -1.0e-20) || (rhs > 1.0e-20));
	double flRecip = 1.0 / rhs;
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] *= flRecip;
	}
	return(*this);
}

GenMatrix& GenMatrix::operator+=(double rhs)
{
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] += rhs;
	}
	return(*this);
}

GenMatrix& GenMatrix::operator-=(double rhs)
{
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] -= rhs;
	}
	return(*this);
}

// Public methods - - - - - - - - - - - - - - - - - - - -  

double GenMatrix::GetMaxAbsElement() const
{
	double flMax = fabs(m_pflData[0]);  // initialize  
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 1)
	{
		double flVal = fabs(m_pflData[nElementCount]);
		if (flVal > flMax)
		{
			flMax = flVal;
		}
	}
	return(flMax);
}

double GenMatrix::GetMaxElement() const
{
	double flMax = m_pflData[0];  // initialize  
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 1)
	{
		if (m_pflData[nElementCount] > flMax)
		{
			flMax = m_pflData[nElementCount];
		}
	}
	return(flMax);
}

double GenMatrix::GetMinAbsElement() const
{
	double flMin = fabs(m_pflData[0]);  // initialize  
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 1)
	{
		double flVal = fabs(m_pflData[nElementCount]);
		if (flVal < flMin)
		{
			flMin = flVal;
		}
	}
	return(flMin);
}

double GenMatrix::GetMinElement() const
{
	double flMin = m_pflData[0];  // initialize  
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 1)
	{
		if (m_pflData[nElementCount] < flMin)
		{
			flMin = m_pflData[nElementCount];
		}
	}
	return(flMin);
}

int GenMatrix::GetRowCount() const
{
	return(m_nRowCount);
}

int GenMatrix::GetColumnCount() const
{
	return(m_nColumnCount);
}

int GenMatrix::GetElementCount() const
{
	return(m_nRowCount * m_nColumnCount);
}

double GenMatrix::GetTrace() const
{
	//ATLASSERT(IsSquare());
	double flTrace = 0.0;
	for (int i = 0, j = 0; i < m_nRowCount; i++, j += (m_nRowCount + 1))
	{
		flTrace += m_pflData[j];    // add up the diagonal elements  
	}
	return(flTrace);
}

double GenMatrix::GetDeterminant() const
{
	//ATLASSERT(IsSquare());
	double flDeterminant = 0.0;
	if (m_nRowCount == 1)
	{
		flDeterminant = m_pflData[0];   // special case: 1 x 1 matrix terminates recursion  
	}
	else
	{
		GenMatrix M(m_nRowCount - 1, m_nColumnCount - 1);
		double flSign = 1.0;
		for (int nColumn = 0; nColumn < m_nColumnCount; nColumn++)
		{
			// build the minor matrix (excludes row '0' and column 'nColumn')  
			for (int nSrcRow = 1, nDstRow = 0; nSrcRow < m_nRowCount; nSrcRow++, nDstRow++)
			{
				for (int nSrcColumn = 0, nDstColumn = 0; nSrcColumn < m_nColumnCount; nSrcColumn++)
				{
					if (nSrcColumn != nColumn)
					{
						M(nDstRow, nDstColumn++) = m_pflData[m_nColumnCount * nSrcRow + nSrcColumn];
					}
				}
			}
			flDeterminant += (flSign * m_pflData[nColumn] * M.GetDeterminant()); // recurse  
			flSign = -flSign;   // sign changes for next column  
		}
	}
	return(flDeterminant);
}

bool GenMatrix::Invert()
{
	// returns 'true' if success and 'false' if singular (in which case the matrix contains nonsense)  
	//ATLASSERT(IsSquare());
	int nSize = m_nRowCount;
	GenMatrix& Ainv = *this;
	GenMatrix A = Ainv;
	GenMatrix B(nSize, nSize);
	B.SetIdentity();

	GenMatrix S(nSize);     // row scale factors  
	int* pnIndex = new int[static_cast<unsigned int>(nSize)];
	//ATLASSERT(pnIndex != NULL);
	for (int nRow = 0; nRow < nSize; nRow++)
	{
		pnIndex[nRow] = nRow;
		double flMaxScale = 0.0;
		for (int nColumn = 0; nColumn < nSize; nColumn++)
		{
			double flVal = fabs(A(nRow, nColumn));
			flMaxScale = (flMaxScale >= flVal) ? flMaxScale : flVal;
		}
		S(nRow) = flMaxScale;
	}

	for (int k = 0; k < (nSize - 1); k++)
	{
		// select pivot row from max{|A(j,k)/S(j)|}  
		double flRatioMax = 0.0;
		int nPivot = k;
		for (int i = k; i < nSize; i++)
		{
			double flVal = S(pnIndex[i]);
			if (flVal < 1.0e-20)
			{
				delete[] pnIndex;
				pnIndex = NULL;
				return(false);  // failure: singular matrix  
			}
			double flRatio = fabs(A(pnIndex[i], k) / flVal);
			if (flRatio > flRatioMax)
			{
				nPivot = i;
				flRatioMax = flRatio;
			}
		}

		// perform pivoting  
		int nIndex = pnIndex[k];
		if (nPivot != k)
		{
			nIndex = pnIndex[nPivot];
			pnIndex[nPivot] = pnIndex[k];
			pnIndex[k] = nIndex;
		}

		// forward elimination  
		for (int i = (k + 1); i < nSize; i++)
		{
			double flVal = A(nIndex, k);
			if ((flVal < 1.0e-20) && (flVal > -1.0e-20))
			{
				delete[] pnIndex;
				pnIndex = NULL;
				return(false);  // failure: singular matrix  
			}
			double flCoeff = A(pnIndex[i], k) / flVal;
			for (int j = (k + 1); j < nSize; j++)
			{
				A(pnIndex[i], j) -= (flCoeff * A(nIndex, j));
			}
			A(pnIndex[i], k) = flCoeff;
			for (int j = 0; j < nSize; j++)
			{
				B(pnIndex[i], j) -= (A(pnIndex[i], k) * B(nIndex, j));
			}
		}
	}

	// back substitution  
	for (int k = 0; k < nSize; k++)
	{
		double flVal = A(pnIndex[nSize - 1], nSize - 1);
		if ((flVal < 1.0e-20) && (flVal > -1.0e-20))
		{
			delete[] pnIndex;
			pnIndex = NULL;
			return(false);  // failure: singular matrix  
		}
		Ainv(nSize - 1, k) = B(pnIndex[nSize - 1], k) / flVal;
		for (int i = (nSize - 2); i >= 0; i--)
		{
			double flSum = B(pnIndex[i], k);
			for (int j = (i + 1); j < nSize; j++)
			{
				flSum -= (A(pnIndex[i], j) * Ainv(j, k));
			}
			flVal = A(pnIndex[i], i);
			if ((flVal < 1.0e-20) && (flVal > -1.0e-20))
			{
				delete[] pnIndex;
				pnIndex = NULL;
				return(false);  // failure: singular matrix  
			}
			Ainv(i, k) = flSum / flVal;
		}
	}
	delete[] pnIndex;
	pnIndex = NULL;
	return(true);
}

GenMatrix GenMatrix::GetInverse() const
{
	GenMatrix A = (*this);
	A.Invert();
	//ATLENSURE_THROW(A.Invert(), E_UNEXPECTED);
	return(A);
}

void GenMatrix::Transpose()
{
	// in-place transpose using algorithm by Robin Becker  
	int nElements = m_nRowCount * m_nColumnCount;
	if (nElements > 3)
	{
		int nSize = nElements - 2;
		for (int i = 1; i < nSize; i++)
		{
			int nCurrent = i;
			do
			{
				int nColumn = nCurrent / m_nRowCount;
				int nRow = nCurrent % m_nRowCount;
				nCurrent = m_nColumnCount * nRow + nColumn;
			} while (nCurrent < i);

			if (nCurrent > i)
			{
				double flTemp = m_pflData[i];
				m_pflData[i] = m_pflData[nCurrent];
				m_pflData[nCurrent] = flTemp;
			}
		}
	}

	// swap row and column counts  
	int nTmp = m_nRowCount;
	m_nRowCount = m_nColumnCount;
	m_nColumnCount = nTmp;
}

GenMatrix GenMatrix::GetTranspose() const
{
	GenMatrix A = (*this);
	A.Transpose();
	return(A);
}

bool GenMatrix::IsDiagonal(double flEps) const
{
	if (!IsSquare())
	{
		return(false);
	}
	int i = 0;
	for (int nRow = 0; nRow < m_nRowCount; nRow++)
	{
		for (int nColumn = 0; nColumn < m_nColumnCount; nColumn++, i++)
		{
			if (nRow == nColumn)
			{
				continue;   // ignore diagonal  
			}
			double flVal = fabs(m_pflData[i]);
			if (flVal > flEps)
			{
				return(false);
			}
		}
	}
	return(true);
}

bool GenMatrix::IsSquare() const
{
	return(m_nRowCount == m_nColumnCount);
}

bool GenMatrix::IsSymmetric(double flEps) const
{
	if (!IsSquare())
		return(false);
	for (int nRow = 0; nRow < m_nRowCount; nRow++)
	{
		for (int nColumn = (nRow + 1); nColumn < m_nColumnCount; nColumn++)
		{
			double flUpper = m_pflData[m_nColumnCount * nRow + nColumn];
			double flLower = m_pflData[m_nColumnCount * nColumn + nRow];
			double flDiff = fabs(flUpper - flLower);
			if (flDiff > flEps)
			{
				return(false);
			}
		}
	}
	return(true);
}

void GenMatrix::SetDimensions(int nRowCount, int nColumnCount)
{
	//ATLASSERT((nRowCount > 0) && (nColumnCount > 0));
	int nElementCountOld = m_nRowCount * m_nColumnCount;
	int nElementCountNew = nRowCount * nColumnCount;
	if (nElementCountOld != nElementCountNew)
	{
		delete[] m_pflData;
		m_pflData = NULL;
		m_pflData = new double[static_cast<unsigned int>(nElementCountNew)];
		//ATLASSERT(m_pflData != NULL);
	}
	::memset(m_pflData, 0, nElementCountNew * sizeof(double));
	m_nRowCount = nRowCount;
	m_nColumnCount = nColumnCount;
}

void GenMatrix::SetIdentity()
{
	//ATLASSERT(IsSquare());
	for (int nRow = 0, i = 0; nRow < m_nRowCount; nRow++)
	{
		for (int nColumn = 0; nColumn < m_nColumnCount; nColumn++, i++)
		{
			m_pflData[i] = (nRow == nColumn) ? 1.0 : 0.0;
		}
	}
}

void GenMatrix::SetAllElements(double flElementValue)
{
	int nElementCount = m_nRowCount * m_nColumnCount;
	while (nElementCount-- > 0)
	{
		m_pflData[nElementCount] = flElementValue;
	}
}

void GenMatrix::SetZero()
{
	::memset(m_pflData, 0, m_nRowCount * m_nColumnCount * sizeof(double));
}

//  Private methods - - - - - - - - - - - - - - - - - - - -  

void GenMatrix::Copy(const GenMatrix& A)
{
	m_nRowCount = A.m_nRowCount;
	m_nColumnCount = A.m_nColumnCount;
	int nElementCount = m_nRowCount * m_nColumnCount;
	m_pflData = new double[static_cast<unsigned int>(nElementCount)];
	//ATLASSERT(m_pflData != NULL);
	::memcpy(m_pflData, A.m_pflData, nElementCount * sizeof(double));
}