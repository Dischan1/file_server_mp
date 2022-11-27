#define CLIENT_DEBUG
#ifndef CLIENT_DEBUG

#include "includes.h"

#include "common_utils.h"
#include "cryptography.h"
#include "ctcp_server.h"

char* get_directory_objects(const char* path)
{
	size_t size;
	if (!get_directory_object_list(path, NULL, &size)) return NULL;

	char** buffer = malloc(sizeof(char*) * size);
	if (!get_directory_object_list(path, buffer, &size)) return NULL;

	size_t list_size = 0u;

	for (size_t i = 0u; i < size; ++i)
		list_size += strlen(buffer[i]) + sizeof((char)'\n');

	char* list = malloc(list_size + sizeof((char)'\0'));
	size_t offset = 0u;

	for (size_t i = 0u; i < size; ++i)
	{
		size_t buffer_len = strlen(buffer[i]);
		memcpy(list + offset, buffer[i], buffer_len);
		(list + offset)[buffer_len] = '\n';
		offset += buffer_len + 1u;
	}

	list[list_size] = '\0';

	free(buffer);
	return list;
}

bool transmit_file(_connection* con, _packet* packet)
{
	char* buffer;
	long size;

	if (!load_file(&buffer, &size, packet->path))
		return false;

	bool status = load_keys_transmit_encrypt_data(con, buffer, size, packet->client_id);
	return free(buffer), status;
}

bool receive_file(_connection* con, _packet* packet)
{
	char* data;
	size_t size;

	if (!load_keys_receive_decrypt_data(con, &data, &size, packet->client_id))
		return false;

	bool status = save_file(data, size, packet->path);
	return free(data), status;
}

bool list_directory(_connection* con, _packet* packet)
{
	char* buffer = get_directory_objects(packet->path);
	if (!buffer) return false;

	size_t buffer_len = strlen(buffer) + 1u;

	bool status = load_keys_transmit_encrypt_data(con, buffer, buffer_len, packet->client_id);
	return free(buffer), status;
}

void on_client_connected(_connection* con)
{
	dp_success("%4x: connected\n", con->guid);

	_packet packet;

	if (receive_packet(con, &packet))
	{
		switch (packet.type)
		{
			case _packet_file_download:
			{
				dp_success("%4x: request: file download\n", con->guid);
				if (transmit_file(con, &packet))
					dp_success("%4x: response: file download\n", con->guid);
				break;
			}
			case _packet_file_upload:
			{
				dp_success("%4x: request: file upload\n", con->guid);
				if (receive_file(con, &packet))
					dp_success("%4x: response: file upload\n", con->guid);

				break;
			}
			case _packet_list_directory:
			{
				dp_success("%4x: request: directory listing\n", con->guid);
				if (list_directory(con, &packet))
					dp_success("%4x: response: directory listing\n", con->guid);
				break;
			}
		}
	}

	dp_success("%4x: disconnected\n", con->guid);
	ctcp_dispose_connection(con);
}

void process_server_receive(_socket sk)
{
	_sockaddr_in sa;
	_ctcp_packet packet;

	while (true)
	{
		int fromlen = sizeof(sa);
		int result	= recvfrom(sk, &packet, sizeof(packet), 0, &sa, &fromlen);

		bool is_packet_size_valid =
			result != SOCKET_DATA_EMPTY &&
			result >= sizeof(_ctcp_header) &&
			result <= sizeof(_ctcp_packet);

		if (!is_packet_size_valid) continue;

		_connection* con = ctcp_find_connection(packet.header.guid);

		if (!con)
		{
			con = ctcp_allocate_connection();
			if (!con) continue;

			con->guid			= packet.header.guid;
			con->socket			= sk;
			con->sockaddr_in	= sa;
			con->ping_last = time(NULL);

			memset(&con->input, 0, sizeof(con->input));
			memset(&con->output, 0, sizeof(con->output));

			thread_create(&on_client_connected, con);
		}
		else
			con->ping_last = time(NULL);

		con->packet = packet;

		if (con->packet.header.ACK) continue;

		if (!ctcp_is_checksum_valid(&con->packet))
		{
			dp_error("invalid checksum\n");
			continue;
		}

		dp_status(result > 0, "%4x: cl->sv->%x\n", con->guid, result);

		ctcp_write_frame_queue(&con->input, &con->packet);
		packet.header.ACK		   = true;
		packet.header.payload_size = 0u;

		ctcp_subscribe_packet(&packet);
		result = sendto(con->socket, &packet, sizeof(packet.header),
			0, &con->sockaddr_in, sizeof(con->sockaddr_in));
	}
}

void process_server_transmit(_socket sk)
{
	while (true)
	{
		ctcp_dispose_expired_connections();

		for (uint32_t i = 0u; i < _countof(gconnections); ++i)
		{
			_connection* con = &gconnections[i];

			if (!con->guid)			continue;
			if (!con->output.count)	continue;

			_ctcp_packet* cache = ctcp_peak_frame_queue(&con->output);

			int result = -1;

			con->packet.header.ACK = false;
			ctcp_subscribe_packet(cache);

			while (!con->packet.header.ACK)
			{
				result = sendto(con->socket, cache, sizeof(*cache),
					0, &con->sockaddr_in, sizeof(con->sockaddr_in));

				sleep(10);
			}

			cache->header.ACK = con->packet.header.ACK;
			ctcp_skip_frame_queue(&con->output);

			dp_status(result > 0, "%4x: sv->cl->%x\n", con->guid, result);
		}

		sleep(1);
	}
}

int print_usage()
{
	printf("usage:\n");
	printf("      server <port>\n");
	return 0;
}

int validate_arguments(int argc, char* argv[])
{
	return argc == 2 ? 0 : 1;
}

int main(int argc, char* argv[])
{
	srand(time(NULL));

	int code = 0;

	if (code = validate_arguments(argc, argv))
		return print_usage(), code;

	if (code = network_startup())
		return _last_error_code;

	_socket sk = socket(AF_INET, SOCK_DGRAM, 0);

	_sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(strtoull(argv[1], 0, 10));
	sa.sin_addr.s_addr = INADDR_ANY;

	int status = bind(sk, &sa, sizeof(sa));

	if (status == SOCKET_ERROR)
	{
		socket_close(sk);
		return _last_error_code;
	}

	_connection* con = malloc(sizeof(_connection));
	memset(&con->input, 0, sizeof(con->input));
	memset(&con->output, 0, sizeof(con->output));

	con->guid		 = ((_guid)rand() << 32u) + ((_guid)rand());
	con->ping_last	 = time(NULL);
	con->sockaddr_in = sa;

	thread_create(&process_server_receive, sk);
	thread_create(&process_server_transmit, sk);

	while (true);

	socket_close(sk);

	return network_shutdown();
}
#endif