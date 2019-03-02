/************************************************************************************
*																					*
*		CatSQL by Jacob Brown (3/1/2019) - Written in C/C++							*
*																					*
*		QueryRow.cpp - Contains a dynamic number of cells to store result			*
*		information. A QueryResults will contain a vector array of QueryRows.		*
*																					*
************************************************************************************/
#include "QueryRow.h"

// Constructor
QueryRow::QueryRow(int numColumns) {

	numCells = numColumns;

	// Creates cells only when the number
	// of columns is greater than zero,
	if (numColumns > 0) {
		cells = new TCHAR * [numCells];

		for (int i = 0; i < numCells; i++)
			cells[i] = new TCHAR[STR_MAX];
	} else {
		cells = NULL;
	}
}

// Destructor
QueryRow::~QueryRow() {

	if (cells != NULL) {
		for (int i = 0; i < numCells; i++) {
			delete[] cells[i];
			cells[i] = NULL;
		}

		delete[] cells;
		cells = NULL;
	}
}