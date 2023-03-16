#pragma once

#ifndef __LIB_HANDLER__
#define __LIB_HANDLER__

#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _MSC_VER
#include <direct.h>
#include <windows.h>
#include <tchar.h>
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS
#else
#include <sys/types.h>
#include <dlfcn.h>
#endif

#if defined(_MSC_VER)
	wchar_t *convertCharArrayToLPCWSTR(const char* charArray)
	{
		wchar_t* wString = new wchar_t[4096];
		MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
		return wString;
	};
	#define LIB_HANDLE_TYPE	::HINSTANCE
	#define LOAD_LIBRARY(str) ::LoadLibraryW(convertCharArrayToLPCWSTR(str))
	#define FREE_LIBRARY(lib_handle) ::FreeLibrary(lib_handle)
	#define LOAD_SYMBOL(lib_handle,symbol) ::GetProcAddress(lib_handle,symbol)
	//#define LOAD_SYMBOL_ERROR_TYPE const char* 
	#define GET_LAST_ERROR() ((::GetLastError()==0) ? std::string() : std::to_string(::GetLastError()))

#elif defined(__GNUC__)
	std::string get_last_error()
	{
		const char* error_msg = ::dlerror();
		if(error_msg) {
			return std::string(error_msg);
		} else {
			return std::string();
		}
	}
	#define LIB_HANDLE_TYPE void*
	#define LOAD_LIBRARY(str) ::dlopen(str,RTLD_LAZY)
	#define FREE_LIBRARY(lib_handle) ((::dlclose(lib_handle) != 0) ? 0 : 1)
	#define LOAD_SYMBOL(lib_handle,symbol) ::dlsym(lib_handle,symbol)
	//#define LOAD_SYMBOL_ERROR_TYPE const char* 
	#define GET_LAST_ERROR() get_last_error() 
#endif

namespace lib
{

struct lib_handler
{
	lib_handler() = default;
	lib_handler(const std::string& lib_name);
	~lib_handler() {
		free_library();
	}
	
	void load_library(
		std::string lib_name,
		std::string version = ""
	) noexcept(false);
	void free_library() noexcept(false);

	template<typename function_type>
	function_type load_symbol(
		const std::string& lib_name
	) noexcept(false);
	
private:	
	std::string     m_lib_name;
	LIB_HANDLE_TYPE m_lib_handle = nullptr;
	
};


lib_handler::lib_handler(const std::string& lib_name)
{
	this->load_library(lib_name);
}


void lib_handler::load_library(
	const std::string lib_name, 
	const std::string version)
{
	// load the library by name

#if defined(_MSC_VER)
	m_lib_name = lib_name + ".dll";
#elif defined(__GNUC__) || defined(__clang__)
	m_lib_name = "lib" + lib_name + ".so";
#else
	static_assert(false,"unknown compiler");
#endif

	m_lib_handle = LOAD_LIBRARY(m_lib_name.c_str());

	if (!m_lib_handle) 
	{
		std::stringstream msg;
	   
#if defined(_MSC_VER)		
		msg << "Cannot load library: " 
			<< m_lib_name << " ";
#endif
		msg	<< GET_LAST_ERROR() << "\n";

		std::cerr << msg.str() << "\n";

		//FREE_LIBRARY(lib_handle);

		throw std::runtime_error(msg.str().c_str());
	}
	else
	{
		std::cout << "Library " 
				  << m_lib_name 
				  << " is loaded at address " 
				  << m_lib_handle 
				  << "\n";
	}
}


void lib_handler::free_library()
{
	//free library handler
	auto freelibrary = FREE_LIBRARY(m_lib_handle);

	if (freelibrary == 0)
	{	
		std::stringstream msg;
		
		msg << "free_library: can't free library "
		    << m_lib_name 
		    << " freelibrary = "  
		    << freelibrary 
		    << "\n";

		std::cerr << msg.str() << "\n";

		throw std::runtime_error(msg.str().c_str());
	}
	else
	{	
		std::cout << "Library " 
				  << m_lib_name 
				  << " is freed at address " 
				  << m_lib_handle 
				  << "\n";
	}
}


template<typename function_type>
function_type lib_handler::load_symbol(
	const std::string& symbol_name
) noexcept(false)
{
	function_type func= (function_type) 
						LOAD_SYMBOL(
							m_lib_handle, 
							symbol_name.c_str()
						);

	std::string symbol_error = GET_LAST_ERROR();
	
	if(!symbol_error.empty()) {

		//FREE_LIBRARY(lib_handle);

		std::stringstream msg;

		msg << "load_symbol: cannot load symbol: "
			<< symbol_name  << " "
			<< symbol_error << " "
			<< "\n";

		std::cerr << msg.str() << "\n";

		throw std::runtime_error(msg.str().c_str());
	}

	return func;
}


}//end namespace lib


#endif
