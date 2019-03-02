#pragma once
#ifndef CONNECTIONINFO_H
#define CONNECTIONINFO_H

#include <windows.h>
#include "resource.h"

class ConnectionInfo {

	public:

		// Constructor
		ConnectionInfo();

		// Destructor
		~ConnectionInfo();

		// Variables
		TCHAR serverName[IDD_TEXT_MAX];
		TCHAR portNumber[IDD_TEXT_MAX];
		TCHAR database	[IDD_TEXT_MAX];
		TCHAR username	[IDD_TEXT_MAX];
		TCHAR password	[IDD_TEXT_MAX];

	private:


};

#endif