#pragma once
#ifndef QUERYROW_H
#define QUERYROW_H

#include <windows.h>
#include "resource.h"

class QueryRow {

	private:

		// Variables
		int numCells;

	public:

		// Constructor
		QueryRow(int numColumns);

		// Destructor
		~QueryRow();

		// Array of cells
		TCHAR ** cells;
};

#endif