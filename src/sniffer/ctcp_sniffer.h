#pragma once

bool ctcp_is_checksum_valid(_ctcp_packet* packet)
{
	static_assert_(offsetof(_ctcp_header, checksum) == 0u, "checksum offset is not zero");
	static_assert_(offsetof(_ctcp_header, guid) == 4u, "guid offset is not 4u");

	char* begin = (char*)&packet->header.guid;
	char* end = packet->payload + packet->header.payload_size;
	size_t size = end - begin;

	_checksum checksum = calc_crc32(begin, size);
	return packet->header.checksum == checksum;
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