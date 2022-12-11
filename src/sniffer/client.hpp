#pragma once

#include "executable_base.hpp"

class client : public executable_base
{
public:
	virtual auto get_connection() -> _connection* override;

private:
};

auto client::get_connection() -> _connection*
{
	if (!hprocess)
		throw std::runtime_error("not connected");

	auto mask	 = "xxx????x????xxxxx";
	auto pattern = "\x48\x8D\x0D\xCC\xCC\xCC\xCC\xE8\xCC\xCC\xCC\xCC\xB9\x0F\x7E\x0B\x00";

	auto va = utils::find_pattern(hmodule, pattern, mask);
	if (!va) return nullptr;

	auto rva = *reinterpret_cast<uint32_t*>(va + 2u);
	return reinterpret_cast<_connection*>(va + rva + 6u);
}