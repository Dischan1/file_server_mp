#pragma once

namespace utils
{
	auto load_file(_bytes& bytes, const std::wstring& path)	-> bool;
	auto load_packet_bytes(const std::wstring& path)		-> _bytes;
	auto find_process_id_by_path(const std::wstring& path)	-> uint32_t;

	auto find_pattern(void* basex, const char* pattern, const char* mask, uint32_t index = 0u) -> uint8_t*;
}
auto utils::load_file(_bytes& bytes, const std::wstring& path) -> bool
{
	auto file = std::ifstream(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) return false;

	file.seekg(0, std::ios::end);
	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	bytes.resize(size);

	auto data = reinterpret_cast<char*>(bytes.data());
	file.read(data, bytes.size());

	return file.close(), true;
}

auto utils::load_packet_bytes(const std::wstring& path) -> _bytes
{
	static _bytes bytes;

	if (!load_file(bytes, path))
		return bytes;

	if (bytes.size() % 2u != 0u)
		throw std::runtime_error("wrong file data");

	char buffer[3] = { 0 };

	for (auto i = 0u; i < bytes.size() / 2u; ++i)
	{
		buffer[0] = bytes[i * 2ull];
		buffer[1] = bytes[i * 2ull + 1ull];
		bytes[i] = std::strtoul(buffer, 0, 16);
	}

	bytes.resize(bytes.size() / 2u);
	return bytes;
}

auto utils::find_process_id_by_path(const std::wstring& path) -> uint32_t
{
	auto hsnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0u);
	if (!hsnapshot || hsnapshot == INVALID_HANDLE_VALUE) return 0ul;

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	Process32First(hsnapshot, &entry);

	uint32_t process_id = 0u;

	do
	{
		if (_wcsicmp(entry.szExeFile, path.c_str()) == 0)
		{
			process_id = entry.th32ProcessID;
			break;
		}
	}
	while (Process32Next(hsnapshot, &entry));

	CloseHandle(hsnapshot);

	return process_id;
}

auto utils::find_pattern(void* basex, const char* pattern, const char* mask, uint32_t index) -> uint8_t*
{
	auto base = reinterpret_cast<char*>(basex);
	auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
	auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);

	auto end = base + nt->OptionalHeader.SizeOfImage;
	auto mask_len = std::strlen(mask);

	auto psections = reinterpret_cast<PIMAGE_SECTION_HEADER>(
		reinterpret_cast<uint8_t*>(nt) + sizeof(nt->FileHeader) + sizeof(nt->Signature) + nt->FileHeader.SizeOfOptionalHeader);

	for (auto it = base; it != end; ++it)
	{
		for (auto i = 0u; i <= mask_len; ++i)
		{
			if (i == mask_len)
			{
				if (index-- == 0u)
					return reinterpret_cast<uint8_t*>(it);
			}

			if (mask[i] == '?')		 continue;
			if (it[i] != pattern[i]) break;
		}
	}

	return nullptr;
}
