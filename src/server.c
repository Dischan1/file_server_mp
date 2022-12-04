#ifndef ENABLE_CLIENT

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

bool transmit_file(_connection* con, _rpc* rpc)
{
	char* path = receive_string(con, 256u);
	if (path == NULL) return false;

	char* buffer;
	long  size;

	if (!load_file(&buffer, &size, path))
		return free(path), false;

	bool status = transmit_data(con, buffer, size);
	return free(path), free(buffer), status;
}

bool receive_file(_connection* con, _rpc* rpc)
{
	char* path = receive_string(con, 256u);
	if (path == NULL) return false;

	char*  data;
	size_t size;

	if (!receive_data(con, &data, &size))
		return free(path), false;

	bool status = save_file(data, size, path);
	return free(path), free(data), status;
}

bool list_directory(_connection* con, _rpc* rpc)
{
	char* path = receive_string(con, 256u);
	if (path == NULL) return false;

	char* buffer = get_directory_objects(path);
	if (!buffer) return free(path), false;

	size_t buffer_len = strlen(buffer) + 1u;

	bool status = transmit_data(con, buffer, buffer_len);
	return free(path), free(buffer), status;
}

bool receive_message(_connection* con, _rpc* rpc)
{
	char* message = receive_string(con, 1024u);
	if (message == NULL) return false;

	printf("\n%s\n\n", message);
	return free(message), true;
}

uint64_t calculate_hash(char* str, size_t size)
{
	uint8_t hash[8u];
	memset(hash, 0xff, sizeof(hash));

	for (uint32_t i = 0u; i < size; ++i)
		hash[i % sizeof(hash)] ^= str[i];

	return *(uint64_t*)hash;
}

char* calculate_hash_str(char* str, size_t size)
{
	uint64_t hash = calculate_hash(str, size);

	size_t hash_str_len = size * 2u + sizeof((char)'\0');
	char* hash_str = malloc(hash_str_len);
	hash_str[0] = '\0';

	for (uint32_t i = 0u; i < sizeof(uint64_t); ++i)
	{
		char buff[3];
		sprintf(buff, "%02x", ((uint8_t*)&hash)[i] & 0xffull);
		strcat(hash_str, buff);
	}

	return hash_str;
}

bool register_passphrase(char* passphrase, size_t size)
{
	size_t hash_str_len = sizeof(uint64_t) * 2u + sizeof((char)'\0');
	char* hash_str = calculate_hash_str(passphrase, size);

	char* file;
	long  file_size_in;

	if (!load_file(&file, &file_size_in, "passwd"))
	{
		save_file(&file, 0l, "passwd");
		if (!load_file(&file, &file_size_in, "passwd"))
			return free(hash_str), false;
	}

	long file_size_out = file_size_in + hash_str_len;

	char* buffer = malloc(file_size_out);
	memcpy(buffer, file, file_size_in);

	hash_str[hash_str_len - 1u] = '\n';
	memcpy(buffer + file_size_in, hash_str, hash_str_len);

	auto status = save_file(buffer, file_size_out, "passwd");
	return free(buffer), free(hash_str), status;
}

bool is_passphrase_registred(char* passphrase, size_t size)
{
	char* file;
	long  file_size;

	if (!load_file(&file, &file_size, "passwd"))
		return false;

	char* hash = calculate_hash_str(passphrase, size);
	char buffer[8u * 2u + 1u];

	for (uint32_t i = 0u; i < file_size; i += sizeof(buffer))
	{
		memcpy(buffer, file + i, sizeof(buffer) - 1u);
		buffer[sizeof(buffer) - 1u] = '\0';

		if (!_stricmp(buffer, hash))
		{
			free(hash);
			free(file);
			return true;
		}
	}

	return false;
}

bool on_client_connect(_connection* con, _rpc* rpc)
{
	_rsa_data rsa;
	rsa_generate_keys(&rsa);
	rsa_get_private(&rsa, &con->key.priv);

	_rsa_public pub;
	rsa_get_public(&rsa, &pub);

	if (!receive_data_sized(con, &con->key.pub, sizeof(con->key.pub)))
		return false;

	if (!transmit_data(con, &pub, sizeof(pub)))
		return false;

	return con->encryption = true;
}

bool on_client_disconnect(_connection* con, _rpc* rpc)
{
	ctcp_dispose_connection(con);
	return true;
}

bool on_client_login(_connection* con, _rpc* rpc)
{
	char* data = receive_string(con, 1024u);
	if (!data) return false;

	con->is_logged_in = is_passphrase_registred(data, strlen(data));
	return free(data), true;
}

bool on_client_register(_connection* con, _rpc* rpc)
{
	char* data = receive_string(con, 1024u);
	if (!data) return false;

	if (!is_passphrase_registred(data, strlen(data)))
		register_passphrase(data, strlen(data));

	return free(data), true;
}

bool process_rpc(_connection* con, _rpc* rpc)
{
	switch (rpc->type)
	{
		case _rpc_file_download:
			return transmit_file(con, rpc);
		case _rpc_file_upload:
			return receive_file(con, rpc);
		case _rpc_list_directory:
			return list_directory(con, rpc);
		case _rpc_message:
			return receive_message(con, rpc);
		case _rpc_connect:
			return on_client_connect(con, rpc);
		case _rpc_disconnect:
			return on_client_disconnect(con, rpc);
		case _rpc_login:
			return on_client_login(con, rpc);
		case _rpc_register:
			return on_client_register(con, rpc);
	}

	dp_error("%4x: unknown rpc\n");
	return false;
}

bool on_rpc_received(_connection* con)
{
	_rpc rpc;

	if (!receive_rpc(con, &rpc))
		return false;

	auto is_rpc_only_logged =
		rpc.type != _rpc_connect &&
		rpc.type != _rpc_disconnect &&
		rpc.type != _rpc_login &&
		rpc.type != _rpc_register;

	if (!con->is_logged_in && is_rpc_only_logged)
	{
		dp_success("%4x: rpc %x rejected\n", con->guid, rpc.type);
		transmit_rpc_status(con, _rpc_status_rejected);
		return false;
	}

	dp_success("%4x: rpc %x accepted\n", con->guid, rpc.type);
	transmit_rpc_status(con, _rpc_status_accepted);

	return process_rpc(con, &rpc);
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
			con->index_next		= packet.header.index;
			con->socket			= sk;
			con->sockaddr_in	= sa;
			con->is_active		= true;
			con->encryption		= false;

			memset(&con->input, 0, sizeof(con->input));
			memset(&con->output, 0, sizeof(con->output));
		}

		con->ping_last = time(NULL);
		con->packet	   = packet;

		if (con->packet.header.ACK) continue;

		if (!ctcp_is_checksum_valid(&packet))
		{
			dp_error("invalid checksum\n");
			continue;
		}

		dp_status(result > 0, "%4x: cl->sv->%x\n", con->guid, result);

		if (packet.header.index == con->index_next)
		{
			if (con->encryption && con->packet.header.payload_size)
			{
				char* dec = decrypt_data(con, packet.payload, packet.header.payload_size);
				if (!dec) continue;

				memcpy(con->packet.payload, dec, con->packet.header.payload_size);
				free(dec);

				con->packet.header.payload_size		 = con->packet.header.payload_size / sizeof(int);
				con->packet.header.payload_size_total = con->packet.header.payload_size_total / sizeof(int);
			}

			ctcp_write_frame_queue(&con->input, &con->packet);
			con->index_next += 1u;

			bool is_rpc_size = con->packet.header.payload_size_total == sizeof(_rpc);
			_rpc* rpc		 = (_rpc*)con->packet.payload;

			if (is_rpc_size && rpc->magic == 0x12345678u)
				thread_create(&on_rpc_received, con);
		}

		ctcp_send_ack(con);
	}
}

void process_server_transmit(_socket sk)
{
	while (true)
	{
		for (uint32_t i = 0u; i < _countof(gconnections); ++i)
		{
			_connection* con = &gconnections[i];

			if (!con->guid)			continue;
			if (!con->output.count)	continue;

			_ctcp_packet* cache = ctcp_peak_frame_queue(&con->output);
			cache->header.index = con->index_next;

			if (con->encryption && cache->header.payload_size)
			{
				char* enc = encrypt_data(con, cache->payload, cache->header.payload_size);
				if (!enc) continue;

				cache->header.payload_size		 = cache->header.payload_size * sizeof(int);
				cache->header.payload_size_total = cache->header.payload_size_total * sizeof(int);

				memcpy(cache->payload, enc, cache->header.payload_size);
				free(enc);
			}

			con->packet.header.ACK = false;
			ctcp_subscribe_packet(cache);

			int result = -1;

			while (!con->packet.header.ACK || con->packet.header.index != cache->header.index)
			{
				result = sendto(con->socket, cache, sizeof(*cache),
					0, &con->sockaddr_in, sizeof(con->sockaddr_in));

				sleep(10);
			}

			con->index_next += 1u;

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
	sa.sin_port = htons((u_short)strtoul(argv[1], 0, 10));
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

	con->guid		 = ((_guid)rand() << 16u) + ((_guid)rand());
	con->ping_last	 = time(NULL);
	con->sockaddr_in = sa;

	thread_create(&process_server_receive, sk);
	thread_create(&process_server_transmit, sk);

	while (true);

	socket_close(sk);

	return network_shutdown();
}
#endif // ENABLE_CLIENT