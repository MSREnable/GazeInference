/*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* GenMatrix - Definition of a general matrix class
* Copied from search //depot/rs2_release/multimedia/Photos/RawCodec/EngineLib/Effects/GenMatrix.h
* Enlistment dir:                 os\src\multimedia\Photos\RawCodec\EngineLib\Effects\GenMatrix.h
*/

#pragma once  

class GenMatrix
{
public:
	// constructors  
	GenMatrix(int nRowCount, int nColumnCount); // all elements set to zero  
	GenMatrix(int nRowCount);                   // column vector, all elements set to zero  
	GenMatrix(const GenMatrix& A);              // copy constructor  

												// destructor  
	virtual ~GenMatrix() throw();

	// overloaded operators  
	GenMatrix& operator=(const GenMatrix& rhs);             // =  operator: assignment  
	double& operator()(int nRow, int nColumn);              // () operator: address matrix element, e.g. A(row,column)  
	const double& operator()(int nRow, int nColumn) const;  // () operator: address matrix element, const version  
	double& operator()(int nElement);                       // () operator: address matrix element A(element)  
	const double& operator()(int nElement) const;           // () operator: address matrix element, const version  
	const GenMatrix operator*(const GenMatrix& rhs) const;  // *  operator: matrix multiplication (R = this * rhs)  
	const GenMatrix operator+(const GenMatrix& rhs) const;  // +  operator: matrix addition       (R = this + rhs)  
	const GenMatrix operator-(const GenMatrix& rhs) const;  // -  operator: matrix subtraction    (R = this - rhs)  
	GenMatrix& operator*=(const GenMatrix& rhs);            // *= operator: matrix multiplication (this *= rhs)  
	GenMatrix& operator+=(const GenMatrix& rhs);            // += operator: matrix addition       (this += rhs)  
	GenMatrix& operator-=(const GenMatrix& rhs);            // -= operator: matrix subtraction    (this -= rhs)  
	const GenMatrix operator*(double rhs) const;            // *  operator: scalar multiplication (R = this * rhs)  
	const GenMatrix operator/(double rhs) const;            // /  operator: scalar division       (R = this / rhs)  
	const GenMatrix operator+(double rhs) const;            // +  operator: scalar addition       (R = this + rhs)  
	const GenMatrix operator-(double rhs) const;            // -  operator: scalar subtraction    (R = this - rhs)  
	GenMatrix& operator*=(double rhs);                      // *= operator: scalar multiplication (this *= rhs)  
	GenMatrix& operator/=(double rhs);                      // /= operator: scalar division       (this /= rhs)  
	GenMatrix& operator+=(double rhs);                      // += operator: scalar addition       (this += rhs)  
	GenMatrix& operator-=(double rhs);                      // -= operator: scalar subtraction    (this -= rhs)  

															// methods  
	double      GetMaxAbsElement() const;   // get maximum {|A(row, column)|}  
	double      GetMaxElement() const;      // get maximum { A(row, column) }  
	double      GetMinAbsElement() const;   // get minimum {|A(row, column)|}  
	double      GetMinElement() const;      // get minimum { A(row, column) }  

	int         GetRowCount() const;        // get the number of rows  
	int         GetColumnCount() const;     // get the number of columns  
	int         GetElementCount() const;    // get the number of elements  

	double      GetTrace() const;           // get the trace (must be square)  
	double      GetDeterminant() const;     // get the determinant (must be square)  

	bool        Invert();                   // invert the matrix in-place (must be square and non-singular)  
	GenMatrix   GetInverse() const;         // get matrix inverse (must be square and non-singular)  
	void        Transpose();                // transpose the matrix in-place  
	GenMatrix   GetTranspose() const;       // get matrix transpose  

	bool        IsDiagonal(double flEps = 1.0e-20) const;   // is this a diagonal matrix (to within +/- flEps)?  
	bool        IsSquare() const;                           // is this a square matrix?  
	bool        IsSymmetric(double flEps = 1.0e-20) const;  // is this a symmetric matrix (to within +/- flEps)?  

	void        SetDimensions(int nRowCount, int nColumnCount); // resize the matrix and initialize all elements to zero  
	void        SetIdentity();                                  // set matrix to identity (must be square)  
	void        SetAllElements(double flElementValue);          // set all elements to flElementValue  
	void        SetZero();                                      // set all elements to zero  

private:
	// methods  
	void        Copy(const GenMatrix& A);

	// data members  
	int         m_nRowCount;    // number of rows (must be > 0)  
	int         m_nColumnCount; // number of columns (must be > 0)  
	double* m_pflData;      // matrix data, in row-major order  
};
