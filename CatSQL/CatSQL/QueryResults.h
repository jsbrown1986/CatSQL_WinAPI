#pragma once
#ifndef QUERYRESULTS_H
#define QUERYRESULTS_H

#include <windows.h>
#include <vector>

#include "QueryRow.h"
#include "resource.h"

using namespace std;

class QueryResults {

	private:

	public:

		// Constructor
		QueryResults();

		// Destructor
		~QueryResults();

		// Methods
		void AllocateHeaders(int noHeaders);
		void AddRow(QueryRow * row);

		// Arrays
		TCHAR ** headers;
		vector<QueryRow*> rows;

		// Variables
		int headerSize;
		int numHeaders;
		int numRows;

		// Result message
		TCHAR message[STR_MAX];
};

#endif