#pragma once

_connection gconnections[CTCP_CONNECTIONS_MAX];

bool ctcp_is_connection_expired(_connection* con)
{
	return con->ping_last - time(NULL) >= CTCP_CONNECTION_TIMEOUT_SECONDS_MAX;
}

size_t ctcp_connections_count()
{
	size_t count = 0u;
	for (uint32_t i = 0u; i < _countof(gconnections); ++i)
		if (gconnections[i].guid != 0u) ++count;
	return count;
}

_connection* ctcp_find_connection(_guid guid)
{
	for (uint32_t i = 0u; i < _countof(gconnections); ++i)
	{
		if (gconnections[i].guid == guid)
			return &gconnections[i];
	}

	return NULL;
}

_connection* ctcp_allocate_connection()
{
	return ctcp_find_connection(0ul);
}

void ctcp_dispose_connection(_connection* con)
{
	for (uint32_t i = 0u; i < _countof(gconnections); ++i)
	{
		if (gconnections[i].guid != con->guid) continue;
		memset(&gconnections[i], 0, sizeof(gconnections[i]));
		break;
	}
}

_connection* ctcp_dispose_expired_connections()
{
	for (uint32_t i = 0u; i < _countof(gconnections); ++i)
	{
		if (ctcp_is_connection_expired(&gconnections[i]))
			ctcp_dispose_connection(&gconnections[i]);
	}

	return NULL;
}