#pragma once

bool ctcp_is_checksum_valid(_ctcp_packet* packet)
{
	static_assert_(offsetof(_ctcp_header, checksum) == 0u, "checksum offset is not zero");
	static_assert_(offsetof(_ctcp_header, guid) == 4u, "guid offset is not 4u");

	char* begin = (char*)&packet->header.guid;
	char* end	= packet->payload + packet->header.payload_size;
	size_t size = end - begin;

	_checksum checksum = calc_crc32(begin, size);
	return packet->header.checksum == checksum;
}

void ctcp_subscribe_packet(_ctcp_packet* packet)
{
	static_assert_(offsetof(_ctcp_header, checksum) == 0u, "checksum offset is not zero");
	static_assert_(offsetof(_ctcp_header, guid) == 4u, "guid offset is not 4u");

	char* begin = (char*)&packet->header.guid;
	char* end	= packet->payload + packet->header.payload_size;
	size_t size = end - begin;

	packet->header.checksum = calc_crc32(begin, size);
}

_ctcp_packet* ctcp_peak_frame_queue(_frames_queue* frames)
{
	if (frames->count == 0) return NULL;
	return &frames->queue[frames->rindex];
}

void ctcp_skip_frame_queue(_frames_queue* frames)
{
	if (frames->count == 0) return;
	frames->rindex = ++frames->rindex % CTCP_CONNECTION_FRAMES_MAX;
	frames->count -= 1u;
}

_ctcp_packet* ctcp_read_frame_queue(_frames_queue* frames, _ctcp_packet* packet)
{
	if (frames->count == 0) return NULL;

	*packet = frames->queue[frames->rindex];

	_ctcp_packet* p = &frames->queue[frames->rindex];

	frames->rindex = ++frames->rindex % CTCP_CONNECTION_FRAMES_MAX;
	frames->count -= 1u;

	return p;
}

_ctcp_packet* ctcp_write_frame_queue(_frames_queue* frames, _ctcp_packet* packet)
{
	if (frames->count >= CTCP_CONNECTION_FRAMES_MAX) return NULL;

	frames->queue[frames->windex] = *packet;

	_ctcp_packet* p = &frames->queue[frames->windex];

	frames->windex = ++frames->windex % CTCP_CONNECTION_FRAMES_MAX;
	frames->count += 1u;

	return p;
}

_ctcp_packet* ctcp_receive_packet(_connection* con, _ctcp_packet* packet);
_ctcp_packet* ctcp_transmit_packet(_connection* con, _ctcp_packet* packet);

_ctcp_packet* ctcp_receive_packet(_connection* con, _ctcp_packet* packet)
{
	_ctcp_packet* p = NULL;

	for (size_t i = 0u; i < CTCP_CONNECTION_TIMEOUT_SECONDS_MAX * 1000; ++i)
	{
		if (con->input.count)
			break;
		sleep(1);
	}

	sleep(10);

	p = ctcp_read_frame_queue(&con->input, packet);
	if (!con->is_active) thread_exit();

	return p;
}

_ctcp_packet* ctcp_transmit_packet(_connection* con, _ctcp_packet* packet)
{
	packet->header.ACK  = false;
	packet->header.guid = con->guid;
	_ctcp_packet* p = ctcp_write_frame_queue(&con->output, packet);

	for (size_t i = 0u; i < CTCP_CONNECTION_TIMEOUT_SECONDS_MAX * 1000; ++i)
	{
		if (p->header.ACK)
			break;
		sleep(1);
	}

	if (!con->is_active) thread_exit();

	return p;
}

bool ctcp_transmit_data(_connection* con, char* msg, size_t size)
{
	_ctcp_packet packet;
	memset(&packet, 0, sizeof(packet));

	packet.header.payload_size		 = 0u;
	packet.header.payload_size_total = size;

	if (!ctcp_transmit_packet(con, &packet)) return false;

	size_t offset	 = 0u;
	size_t size_free = packet.header.payload_size_total;
	size_t payload_size_max = con->encryption ? sizeof(packet.payload) / 4u : sizeof(packet.payload);

	while (size_free > 0)
	{
		size_t payload_size = __min(size_free, payload_size_max);
		memcpy(packet.payload, msg + offset, payload_size);

		packet.header.payload_size = payload_size;

		if (!ctcp_transmit_packet(con, &packet)) return false;

		offset    += payload_size;
		size_free -= payload_size;
	}

	return true;
}

bool ctcp_receive_data(_connection* con, char** data, size_t* size)
{
	_ctcp_packet packet;

	if (!ctcp_receive_packet(con, &packet)) return false;

	packet.header.guid = con->guid;

	*data = malloc(packet.header.payload_size_total);
	*size = packet.header.payload_size_total;

	size_t offset	 = 0u;
	size_t size_free = *size;

	while (size_free > 0)
	{
		while (!ctcp_receive_packet(con, &packet)) sleep(10);

		size_t payload_size = __min(packet.header.payload_size, sizeof(packet.payload));
		memcpy(*data + offset, packet.payload, __min(payload_size, size_free));

		packet.header.payload_size = payload_size;

		offset    += payload_size;
		size_free -= payload_size;
	}

	return true;
}

bool ctcp_send_ack(_connection* con)
{
	con->packet.header.ACK = true;
	con->packet.header.payload_size = 0u;

	ctcp_subscribe_packet(&con->packet);

	return sendto(con->socket, &con->packet, sizeof(con->packet.header),
		0, &con->sockaddr_in, sizeof(con->sockaddr_in)) >= 0u;
}

bool receive_data(_connection* con, char** data, size_t* size)
{
	return ctcp_receive_data(con, data, size);
}

bool transmit_data(_connection* con, char* data, size_t size)
{
	return ctcp_transmit_data(con, data, size);
}

bool receive_data_sized(_connection* con, char* data, size_t target)
{
	size_t size_2;
	char* data_2;

	auto status = ctcp_receive_data(con, &data_2, &size_2);
	if (!status) return false;

	if (size_2 != target)
		return free(data_2), false;

	memcpy(data, data_2, size_2);
	return free(data_2), true;
}

typedef enum
{
	_rpc_status_none,
	_rpc_status_unknown,
	_rpc_status_accepted,
	_rpc_status_rejected,
} _rpc_status;

_rpc_status receive_rpc_status(_connection* con)
{
	_rpc_status status;

	if (!receive_data_sized(con, (char*)&status, sizeof(status)))
		return _rpc_status_unknown;

	return status;
}

bool transmit_rpc_status(_connection* con, _rpc_status status)
{
	return ctcp_transmit_data(con, (char*)&status, sizeof(status));
}

bool receive_rpc(_connection* con, _rpc* rpc)
{
	return receive_data_sized(con, rpc, sizeof(*rpc));
}

_rpc_status transmit_rpc(_connection* con, _rpc_type type)
{
	_rpc rpc;
	rpc.magic = 0x12345678u;
	rpc.type  = type;
	
	if (!ctcp_transmit_data(con, (char*)&rpc, sizeof(rpc)))
		return _rpc_status_unknown;

	return receive_rpc_status(con);
}

char* receive_string(_connection* con, size_t length_max)
{
	char* path;
	size_t size;

	if (!receive_data(con, &path, &size))
		return NULL;

	if (size >= length_max || size == 0u)
		return free(path), NULL;

	if (path[size - 1u] != L'\0')
		return free(path), NULL;

	return path;
}

bool transmit_string(_connection* con, char* data)
{
	return transmit_data(con, data, strlen(data) + 1u);
}

bool load_file(char** buffer, long* size, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file || file == INVALID_HANDLE_VALUE) return false;

	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (*size > 0)
	{
		*buffer = malloc(*size);
		fread(*buffer, *size, 1, file);
	}

	return fclose(file), true;
}

bool load_file_static(char* buffer, long size, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file || file == INVALID_HANDLE_VALUE) return false;

	fseek(file, 0, SEEK_END);

	if (size != ftell(file))
	{
		fclose(file);
		return false;
	}

	fseek(file, 0, SEEK_SET);
	fread(buffer, size, 1, file);

	return fclose(file), true;
}

bool save_file(char* buffer, long size, const char* path)
{
	FILE* file = fopen(path, "wb+");
	if (!file || file == INVALID_HANDLE_VALUE) return false;

	fwrite(buffer, size, 1, file);
	return fclose(file), true;
}

int network_startup()
{
#if defined(_WIN32)
	WSADATA wsadata;
	return WSAStartup(MAKEWORD(2, 2), &wsadata);
#else
	return 0;
#endif
}

int network_shutdown()
{
#if defined(_WIN32)
	return WSACleanup();
#else
	return 0;
#endif
}