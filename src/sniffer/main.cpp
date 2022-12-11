#include <map>
#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <optional>

#include <filesystem>
namespace fs = std::filesystem;

#include <WinSock2.h>

#include "../defines.h"
#include "../crc32.h"
#include "../cryptography.h"

#include <Windows.h>
#include <tlhelp32.h>
#include <winternl.h>

#include "defines.h"
#include "ctcp_sniffer.h"

#include "server.hpp"
#include "client.hpp"

namespace gl
{
	using _connections = std::map<_guid, _connection>;

	_connections sv_connections;
	_connections cl_connections;

	server* sv = new server();
	client* cl = new client();

	_connection* temp = new _connection();
}

auto handle_packet(_connection& con, _ctcp_packet* packet)
{
	ctcp_write_frame_queue(&con.input, packet);

	if (!packet->header.payload_size)
		return;

	auto rpc	= reinterpret_cast<_rpc*>(packet->payload);
	auto is_rpc = rpc->magic == 0x12345678u && packet->header.payload_size == sizeof(_rpc);

	if (is_rpc)
		printf("[sniffer] rpc received: %x\n", rpc->type);
	else
		printf("[sniffer] %04x bytes decrypted: %s\n", packet->header.payload_size, packet->payload);
}

auto handle_bytes(_bytes& bytes) -> void
{
	auto packet = reinterpret_cast<_ctcp_packet*>(bytes.data());

	if (!ctcp_is_checksum_valid(packet))
		return printf("checksum wrong\n"), void();

	if (packet->header.ACK)
		return;

	if (!gl::sv_connections.contains(packet->header.guid))
		return;

	auto& con = gl::sv_connections[packet->header.guid];

	if (!packet->header.payload_size)
	{
		handle_packet(con, packet);
		return;
	}

	auto rpc = reinterpret_cast<_rpc*>(packet->payload);

	if (rpc->magic == 0x12345678)
	{
		handle_packet(con, packet);
		return;
	}

	auto size = packet->header.payload_size;
	auto data = decrypt_data(&con, packet->payload, size);

	packet->header.payload_size		  = packet->header.payload_size / sizeof(int);
	packet->header.payload_size_total = packet->header.payload_size_total / sizeof(int);
	memcpy(packet->payload, data, packet->header.payload_size);

	handle_packet(con, packet);

	if (rpc->magic = 0x12345678 && rpc->type == _rpc_disconnect)
		gl::sv_connections.erase(packet->header.guid);

	free(data);
}

int print_usage()
{
	printf("usage:\n");
	printf("      sniffer <packets_path>\n");
	return 0;
}

int validate_arguments(int argc, char* argv[])
{
	if (argc != 2)
		return -1;

	return 0;
}

auto enum_server_connections() -> void
{
	std::memset(gl::temp, 0, sizeof(*gl::temp));

	if (gl::sv->connect(L"server.exe"))
	{
		for (auto i = 0u; i < CTCP_CONNECTIONS_MAX; ++i)
		{
			while (!gl::temp->key.priv.d)
			{
				if (!gl::sv->read_connection(gl::temp, 0))
					break;
			}

			if (!gl::temp->guid) continue;
			gl::sv_connections[gl::temp->guid] = *gl::temp;
		}

		gl::sv->disconnect();
	}
}

auto enum_client_connections() -> void
{
	std::memset(gl::temp, 0, sizeof(*gl::temp));

	if (gl::cl->connect(L"client.exe"))
	{
		while (!gl::temp->key.priv.d)
		{
			if (!gl::cl->read_connection(gl::temp, 0))
				break;
		}

		gl::cl_connections[gl::temp->guid] = *gl::temp;
		gl::cl->disconnect();
	}
}

auto process_packets(const std::string& packets_path) -> void
{
	static auto handled_packets_count = 0ull;

	for (auto& file : fs::directory_iterator(packets_path))
	{
		if (file.path().extension() != ".pkt") continue;

		auto file_name	  = file.path().filename().wstring();
		auto packet_index = std::wcstoull(file_name.c_str(), 0, 16);

		if (packet_index < handled_packets_count)
		{
			fs::remove(file);
			continue;
		}

		auto bytes = utils::load_packet_bytes(file.path().wstring());

		if (bytes.empty())
		{
			fs::remove(file);
			break;
		}

		handle_bytes(bytes);
		fs::remove(file);

		handled_packets_count = packet_index + 1;
	}
}

int main(int argc, char* argv[])
{
	srand(time(NULL));

	int code = 0;

	if (code = validate_arguments(argc, argv))
		return code;

	while (true)
	{
		enum_server_connections();
		enum_client_connections();

		process_packets(argv[1]);
	}

	return 0;
}