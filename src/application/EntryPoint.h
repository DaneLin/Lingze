#pragma once

#include <iostream>
#include "../backend/Logging.h"

// Define entry point macro for quick application entry point generation
// Usage: LINGZE_MAIN(AppClassName)
// Where AppClassName is the name of a class that inherits from lz::App

#define LINGZE_MAIN(AppClass)                                                \
	int main()                                                               \
	{                                                                        \
		try                                                                  \
		{                                                                    \
			AppClass app;                                                    \
			return app.run();                                                \
		}                                                                    \
		catch (const std::exception &e)                                      \
		{                                                                    \
			LOGE("Program exception: {}", e.what());                         \
			return -1;                                                       \
		}                                                                    \
		catch (...)                                                          \
		{                                                                    \
			LOGE("Unknown program exception occurred!");                     \
			return -1;                                                       \
		}                                                                    \
	}