#pragma once
#ifndef SERVERHANDLER_H
#define SERVERHANDLER_H

#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <iostream>
#include <string.h>
#include <locale.h>
#include <sstream>
#include <atlstr.h>

#include "ConnectionInfo.h"
#include "QueryResults.h"
#include "resource.h"

using namespace std;

class ServerHandler {

	private:

		// Variables
		SQLHANDLE sqlConnHandle;
		SQLHANDLE sqlStmtHandle;
		SQLHANDLE sqlEnvHandle;
		SQLWCHAR  retconstring[SQL_RETURN_CODE_LEN];

		// Methods
		void ConnectionStringBuilderUserAuth(ConnectionInfo * connectionInfo, TCHAR * finalStr);
		void ConnectionStringBuilderWindAuth(ConnectionInfo * connectionInfo, TCHAR * finalStr);
		void CloseConnection();

	public:

		// Constructor
		ServerHandler();

		// Destructor
		~ServerHandler();

		// Methods
		BOOL AllocateHandles();
		BOOL ConnectToInstance(ConnectionInfo * connectionInfo, BOOL userAuth, HWND hWnd);
		BOOL SubmitQuery(TCHAR * query, QueryResults * queryResults, HWND hWnd);
};

#endif