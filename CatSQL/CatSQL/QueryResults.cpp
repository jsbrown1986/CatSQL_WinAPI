/****************************************************************************************
*											*
*		CatSQL by Jacob Brown (3/1/2019) - Written in C/C++			*
*											*
*		QueryResults.cpp - Stores all the result information from an SQL	*
*		query to be displayed in main.cpp in a listview.			*
*											*
****************************************************************************************/
#include "QueryResults.h"

// Constructor
QueryResults::QueryResults() {
	headers    = NULL;
	numHeaders = 0;
	numRows    = 0;
}

// Destructor
QueryResults::~QueryResults() {

	// Deletes all the header info
	if (headers != NULL) {
		for (int i = 0; i < numHeaders; i++) {
			delete[] headers[i];
			headers[i] = NULL;
		}

		delete[] headers;
		headers = NULL;
	}

	// Deletes all the objects in the vector
	for (int i = 0; i < numRows; i++)
		delete rows[i];

	rows.clear();
}

// AllocateHeaders - Creates the headers once we get
// the number of columns from the query's results
void QueryResults::AllocateHeaders(int noHeaders) {

	numHeaders = noHeaders;
	if (noHeaders > 0) {
		headers = new TCHAR * [numHeaders];

		for (int i = 0; i < numHeaders; i++)
			headers[i] = new TCHAR[STR_MAX];
	} else {
		headers = NULL;
	}
}

// AddRow - Avoids the problem of casting a vector's 
// size to an int to determine number of rows
void QueryResults::AddRow(QueryRow * row) {

	rows.push_back(row);
	numRows++;
}
