#pragma once

#include "executable_base.hpp"

class server : public executable_base
{
public:
	virtual auto get_connection() -> _connection* override;

private:
};

auto server::get_connection() -> _connection*
{
	if (!hprocess)
		throw std::runtime_error("not connected");

	auto mask	 = "xxx????xxxxxx";
	auto pattern = "\x48\x8D\x0D\xCC\xCC\xCC\xCC\x83\x3C\x01\x00\x74\x0B";

	auto va = utils::find_pattern(hmodule, pattern, mask);
	if (!va) return nullptr;

	auto rva = *reinterpret_cast<uint32_t*>(va + 3u);
	return reinterpret_cast<_connection*>(va + rva + 7u);
}