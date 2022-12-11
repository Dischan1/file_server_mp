#pragma once

#include "utils.hpp"

class executable_base
{
public:
	auto connect(const std::wstring& name) -> bool;
	auto disconnect() -> void;

	virtual auto get_connection() -> _connection*;

	auto read_connection(_connection* buffer, uint32_t index) -> bool;

protected:
	HMODULE hmodule;
	HANDLE  hprocess;
};

auto executable_base::connect(const std::wstring& name) -> bool
{
	auto process_id = utils::find_process_id_by_path(name);
	if (!process_id) return false;

	{
		if (hprocess && hprocess != INVALID_HANDLE_VALUE)
			throw std::runtime_error("already connected");

		hprocess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);

		if (!hprocess || hprocess == INVALID_HANDLE_VALUE)
			return disconnect(), false;
	}

	{
		if (hmodule && hmodule != INVALID_HANDLE_VALUE)
			throw std::runtime_error("already connected");

		auto path = fs::current_path() / name;
		hmodule = LoadLibraryW(path.wstring().c_str());

		if (!hmodule || hmodule == INVALID_HANDLE_VALUE)
			return disconnect(), false;
	}

	return true;
}

auto executable_base::disconnect() -> void
{
	if (hprocess && hprocess != INVALID_HANDLE_VALUE)
		CloseHandle(hprocess);

	if (hmodule && hmodule != INVALID_HANDLE_VALUE)
		FreeLibrary(hmodule);

	hmodule = nullptr;
	hprocess = nullptr;
}

auto executable_base::get_connection() -> _connection*
{
	throw std::runtime_error("not implemented");
}

auto executable_base::read_connection(_connection* buffer, uint32_t index) -> bool
{
	auto	con = get_connection() + index;
	size_t	bytes_read = 0u;

	auto status = ReadProcessMemory(hprocess, con, buffer, sizeof(*buffer), &bytes_read);
	return status && bytes_read == sizeof(*buffer);
}