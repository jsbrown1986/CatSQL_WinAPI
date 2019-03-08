/************************************************************************************************
*												*									*
*		CatSQL by Jacob Brown (3/6/2019) - Written in C/C++				*
*												*
*		ServerHandler.cpp - When called upon from main.cpp, ServerHandler		*
*		allocates various connection handles, attempts a connection with the		*
*		server name and port provided (with possible add'l information), submits	*
*		queries, and returns the results. Frees all handles and closes the		*
*		connection when deleted.							*
*												*									*
************************************************************************************************/
#include "ServerHandler.h"

// Constructor
ServerHandler::ServerHandler() {
	sqlConnHandle = NULL;
	sqlStmtHandle = NULL;
	sqlEnvHandle  = NULL;
}

ServerHandler::~ServerHandler() {
	CloseConnection();
}

// AttemptAllocations - Attemps to allocate our various SQL handles.
// Returns false if any of them fail.
BOOL ServerHandler::AllocateHandles() {

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
		return FALSE;

	if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		return FALSE;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle))
		return FALSE;

	return TRUE;
}

// ConnectionStringBuilderWindAuth - Constructs a connection string that uses
// Windows authentication instead of a username/password authentication.
// Format: "DRIVER={SQL Server};SERVER=servername, portnum;DATABASE=databasename;TRUSTED_CONNECTION=yes;"
void ServerHandler::ConnectionStringBuilderWindAuth(ConnectionInfo * connectionInfo, TCHAR * finalStr) {

	TCHAR part1[] = TEXT("DRIVER={SQL Server};SERVER=");
	TCHAR part2[] = TEXT(", ");
	TCHAR part3[] = TEXT(";DATABASE=");
	TCHAR part4[] = TEXT(";TRUSTED_CONNECTION=yes;");

	// Assembles the final string
	swprintf(finalStr, STR_MAX, TEXT("%ls%ls%ls%ls%ls%ls%ls"), part1, connectionInfo->serverName, part2, connectionInfo->portNumber, part3,
		connectionInfo->database, part4);

	return;
}

// ConnectionStringBuilderUserAuth - Constructs a connection string that uses
// username/password authentication instead of Windows authentication.
// Format: "DRIVER={SQL Server};SERVER=servername, portnum;DATABASE=databasename;USER ID=username;PASSWORD=password;"
void ServerHandler::ConnectionStringBuilderUserAuth(ConnectionInfo * connectionInfo, TCHAR * finalStr) {

	TCHAR part1[] = TEXT("DRIVER={SQL Server};SERVER=");
	TCHAR part2[] = TEXT(", ");
	TCHAR part3[] = TEXT(";DATABASE=");
	TCHAR part4[] = TEXT(";USER ID=");
	TCHAR part5[] = TEXT(";PASSWORD=");
	TCHAR part6[] = TEXT(";");
	
	// Assembles the final string
	swprintf(finalStr, STR_MAX, TEXT("%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls"), part1, connectionInfo->serverName, part2, connectionInfo->portNumber, 
		part3, connectionInfo->database, part4, connectionInfo->username, part5, connectionInfo->password, part6);

	return;
}

// ConnectToInstance - Uses the connection info submitted by the user, assembles it
// into a connection string, and attempts to connect with it.
BOOL ServerHandler::ConnectToInstance(ConnectionInfo * connectionInfo, BOOL userAuth, HWND hWnd) {

	TCHAR finalConnStr[STR_MAX];
	userAuth ? ConnectionStringBuilderUserAuth(connectionInfo, finalConnStr) : 
			   ConnectionStringBuilderWindAuth(connectionInfo, finalConnStr);

	// Attempts to connect to our pre-defined server and port
	switch (SQLDriverConnect(sqlConnHandle, NULL, finalConnStr, SQL_NTS, retconstring, SQL_RETURN_CODE_LEN, NULL, SQL_DRIVER_NOPROMPT)) {

		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO: 
			return TRUE;

		case SQL_INVALID_HANDLE:
		case SQL_ERROR:
		default:
			return FALSE;
	}

	return FALSE;
}

// SubmitQuery - Takes the assembled query and a pointer to a QueryResults object, and
// attaches the results of that query to the QueryResults objects.
BOOL ServerHandler::SubmitQuery(TCHAR * query, QueryResults * queryResults, HWND hWnd) {

	// Checks the size of the query first
	if (_tcslen(query) == 0)
		return FALSE;

	// Attempts to allocate statement handle
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle))
		return FALSE;

	// If there is a problem executing the query then exit application else display query result
	// NOTE: SQLExecDirect is preferred over SQLPrepare + SQLExecDirect if we're only executing queries once.
	if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)query, SQL_NTS)) {
		return FALSE;
	} else {

		// If our query has only altered the database...
		SQLINTEGER rowsAffectedCount;
		if (SQL_SUCCESS != SQLRowCount(sqlStmtHandle, &rowsAffectedCount))
			return FALSE;

		// ...we return a message showing the number of rows altered.
		if (rowsAffectedCount >= 0) {
			TCHAR msg[STR_MAX];
			_itot_s(rowsAffectedCount, msg, 10);
			swprintf(queryResults->message, STR_MAX, TEXT("%ls%ls"), TEXT("Rows affected: "), msg);
			return TRUE;
		}

		// Gets the number of columns and allocates
		// space for them in the query results
		SQLSMALLINT numColumns;
		SQLNumResultCols(sqlStmtHandle, &numColumns);
		queryResults->AllocateHeaders(numColumns);

		// Gets the column names and types for
		// retrieving and displaying data
		SQLSMALLINT bufferLen = 64;
		SQLSMALLINT ptrNameLen = 0;
		SQLULEN		ptrColSize;

		// Used to gather the data types for each column
		SQLSMALLINT * dataType = new SQLSMALLINT[numColumns];

		// Sets all the column names
		for (int i = 0; i < numColumns; i++)
			SQLDescribeCol(sqlStmtHandle, i + 1, queryResults->headers[i], bufferLen, &ptrNameLen, &dataType[i], &ptrColSize, NULL, NULL);

		while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {

			QueryRow * queryRow = new QueryRow(numColumns);

			for (int i = 0; i < numColumns; i++) {

				switch (dataType[i]) {

					case SQL_CHAR:
					case SQL_VARCHAR:
						{
							TCHAR sqlOutput[SQL_RESULT_LEN];
							SQLLEN ptrSqlOutput;
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_CHAR, &sqlOutput, SQL_RESULT_LEN, &ptrSqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, sqlOutput);
							}						
					}
					break;

					case SQL_BINARY:
					case SQL_VARBINARY:
					case SQL_LONGVARBINARY:
						{
							TCHAR sqlOutput[SQL_RESULT_LEN];
							SQLLEN ptrSqlOutput;
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_BINARY, &sqlOutput, SQL_RESULT_LEN, &ptrSqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, sqlOutput);
							}
						}
						break;

					case SQL_NUMERIC:
					case SQL_DECIMAL:
					case SQL_REAL:
					case SQL_DOUBLE:
						{
							SQLDOUBLE sqlOutput;
							SQLLEN ptrSqlOutput;
							TCHAR temp[STR_MAX];
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_DOUBLE, &sqlOutput, sizeof(SQLDOUBLE), &ptrSqlOutput);
							swprintf(temp, STR_MAX, TEXT("%f"), sqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, temp);
							}						
						}
						break;

					case SQL_INTEGER:
					case SQL_SMALLINT:
						{
							SQLINTEGER sqlOutput;
							SQLLEN ptrSqlOutput;
							TCHAR temp[STR_MAX];
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_SLONG, &sqlOutput, sizeof(SQLINTEGER), &ptrSqlOutput);
							swprintf(temp, STR_MAX, TEXT("%d"), sqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, temp);
							}						
						}
						break;

					case SQL_FLOAT:
						{
							SQLREAL sqlOutput;
							SQLLEN ptrSqlOutput;
							TCHAR temp[STR_MAX];
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_FLOAT, &sqlOutput, sizeof(SQLREAL), &ptrSqlOutput);
							swprintf(temp, STR_MAX, TEXT("%f"), sqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, temp);
							}						
						}
						break;

					case SQL_BIGINT:
						{
							SQLBIGINT sqlOutput;
							SQLLEN ptrSqlOutput;
							TCHAR temp[STR_MAX];
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_SBIGINT, &sqlOutput, sizeof(SQLBIGINT), &ptrSqlOutput);
							swprintf(temp, STR_MAX, TEXT("%d"), sqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, temp);
							}						
						}
						break;

					case SQL_TINYINT:
						{
							SQLSCHAR sqlOutput;
							SQLLEN ptrSqlOutput;
							TCHAR temp[STR_MAX];
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_STINYINT, &sqlOutput, sizeof(SQLSCHAR), &ptrSqlOutput);
							swprintf(temp, STR_MAX, TEXT("%d"), sqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, temp);
							}						
						}
						break;

					case SQL_WLONGVARCHAR:
					case SQL_WVARCHAR:
					case SQL_WCHAR:
					case SQL_LONGVARCHAR:
						{
							TCHAR sqlOutput[SQL_RESULT_LEN];
							SQLLEN ptrSqlOutput;
							SQLGetData(sqlStmtHandle, i + 1, SQL_WCHAR, &sqlOutput, SQL_RESULT_LEN, &ptrSqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, sqlOutput);
							}
						}
						break;

					case SQL_DATE:
						{
							TCHAR sqlOutput[SQL_RESULT_LEN];
							SQLLEN ptrSqlOutput;
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_TYPE_DATE, &sqlOutput, sizeof(DATE_STRUCT), &ptrSqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, sqlOutput);
							}
						}
					break;

					case SQL_TIME:
						{
							TCHAR sqlOutput[SQL_RESULT_LEN];
							SQLLEN ptrSqlOutput;
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_TYPE_TIME, &sqlOutput, sizeof(TIME_STRUCT), &ptrSqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, sqlOutput);
							}
						}
						break;

					case SQL_TIMESTAMP:
						{
							TCHAR sqlOutput[SQL_RESULT_LEN];
							SQLLEN ptrSqlOutput;
							SQLGetData(sqlStmtHandle, i + 1, SQL_C_TYPE_TIMESTAMP, &sqlOutput, sizeof(TIMESTAMP_STRUCT), &ptrSqlOutput);

							// Checks if the data returned is null, sets cell to "NULL" if so.
							if (ptrSqlOutput == SQL_NULL_DATA) {
								_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
							} else {
								_tcscpy_s(queryRow->cells[i], STR_MAX, sqlOutput);
							}
						}
						break;

					case SQL_UNKNOWN_TYPE:
					default:
						{
							_tcscpy_s(queryRow->cells[i], STR_MAX, TEXT("NULL"));
						}
						break;
				}
			}
			
			queryResults->AddRow(queryRow);
		}

		TCHAR tempMsg[STR_MAX];
		_itot_s(queryResults->numRows, tempMsg, 10);
		swprintf(queryResults->message, STR_MAX, TEXT("%ls%ls"), TEXT("Rows returned: "), tempMsg);

		delete[] dataType;
		dataType = NULL;
		
		return TRUE;
	}
	return FALSE;
}

// CloseConnection - Frees all the handles allocated
// and attempts to close the connection to the server.
void ServerHandler::CloseConnection() {
	SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
	SQLDisconnect(sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
	return;
}
