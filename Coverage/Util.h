#pragma once

#include <cstring>
#include <string>
#include <Windows.h>

struct Util
{
	static std::string GetLastErrorAsString()
	{
		//Get the error message, if any.
		DWORD errorMessageID = ::GetLastError();
		if (errorMessageID == 0)
		{
			return std::string(); //No error message has been recorded
		}

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
									 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		while (size > 0 && (messageBuffer[size - 1] == '\r' || messageBuffer[size - 1] == '\n')) { --size; }

		std::string message(messageBuffer, size);

		//Free the buffer.
		LocalFree(messageBuffer);

		return message;
	}

	static bool InvariantEquals(const std::string& lhs, const std::string& rhs)
	{
		if (lhs.size() == rhs.size())
		{
			for (size_t i = 0; i < lhs.size(); ++i)
			{
				char l = tolower(lhs[i]);
				char r = tolower(rhs[i]);

				if (l != r) { return false; }
			}
			return true;
		}
		return false;
	}
};