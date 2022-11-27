#pragma once

_connection* ctcp_allocate_connection()
{
	_connection* con = malloc(sizeof(_connection));

	size_t guid_offset = sizeof(_guid) / 2;

	con->guid		= ((_guid)rand() << guid_offset) + ((_guid)rand());
	con->ping_last	= time(NULL);

	memset(&con->input, 0, sizeof(con->input));
	memset(&con->output, 0, sizeof(con->output));

	return con;
}

void ctcp_dispose_connection(_connection* con)
{
	free(con);
}