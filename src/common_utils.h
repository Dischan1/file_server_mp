#pragma once

bool ctcp_is_checksum_valid(_ctcp_packet* packet)
{
	static_assert_(offsetof(_ctcp_header, checksum) == 0u, "checksum offset is not zero");
	static_assert_(offsetof(_ctcp_header, guid) == 4u, "guid offset is not 4u");

	char* begin = &packet->header.guid;
	char* end	= packet->payload + packet->header.payload_size;
	size_t size = end - begin;

	_checksum checksum = calc_crc32(begin, size);
	return packet->header.checksum == checksum;
}

void ctcp_subscribe_packet(_ctcp_packet* packet)
{
	static_assert_(offsetof(_ctcp_header, checksum) == 0u, "checksum offset is not zero");
	static_assert_(offsetof(_ctcp_header, guid) == 4u, "guid offset is not 4u");

	char* begin = &packet->header.guid;
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
	if (frames->count == 0) return NULL;
	frames->rindex = ++frames->rindex % CTCP_CONNECTION_FRAMES_MAX;
	frames->count -= 1u;
}

_ctcp_packet* ctcp_read_frame_queue(_frames_queue* frames, _ctcp_packet* packet)
{
	if (frames->count == 0) return NULL;

	*packet = frames->queue[frames->rindex];

	_ctcp_packet* p = &frames->queue[frames->windex];

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
	while (!ctcp_read_frame_queue(&con->input, packet)) sleep(1);
}

_ctcp_packet* ctcp_transmit_packet(_connection* con, _ctcp_packet* packet)
{
	packet->header.ACK   = false;
	packet->header.guid  = con->guid;
	_ctcp_packet* p = ctcp_write_frame_queue(&con->output, packet);
	while (!p->header.ACK) sleep(1);
	return p;
}

bool ctcp_transmit_data(_connection* con, char* data, size_t size)
{
	_ctcp_packet packet;
	memset(&packet, 0, sizeof(packet));

	packet.header.payload_size_total = size;

	if (!ctcp_transmit_packet(con, &packet)) return false;

	size_t offset	 = 0u;
	size_t size_free = packet.header.payload_size_total;

	while (size_free > 0)
	{
		size_t payload_size = __min(size_free, sizeof(packet.payload));
		memcpy(packet.payload, data + offset, payload_size);

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

	packet.header.guid  = con->guid;

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

void construct_packet(_packet* packet, const char* path, _packet_type type, int client_id, size_t size)
{
	memset(packet, 0, sizeof(packet));
	memcpy(packet->path, path, (strlen(path) + 1ull) * sizeof(*path));

	packet->type = type;
	packet->size = size;
	packet->client_id = client_id;
}

bool receive_data(_connection* con, char** data, size_t* size)
{
	return ctcp_receive_data(con, data, size);
}

bool transmit_data(_connection* con, char* data, size_t size)
{
	return ctcp_transmit_data(con, data, size);
}

bool receive_packet(_connection* con, _packet* packet)
{
	size_t size;
	_packet* temp;

	bool status = ctcp_receive_data(con, &temp, &size);
	if (!status) return false;

	if (size != sizeof(_packet))
	{
		free(temp);
		return false;
	}

	*packet = *temp;
	return true;
}

bool transmit_packet(_connection* con, _packet* packet)
{
	return ctcp_transmit_data(con, packet, sizeof(_packet));
}

bool load_file(char** buffer, long* size, const char* path)
{
	FILE* file = fopen(path, "rb");

	if (!file || file == INVALID_HANDLE_VALUE)
	{
		dp_error("load file error");
		return false;
	}

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

	if (!file || file == INVALID_HANDLE_VALUE)
	{
		dp_error("load file error");
		return false;
	}

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

	if (!file || file == INVALID_HANDLE_VALUE)
	{
		dp_error("save file error");
		return false;
	}

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