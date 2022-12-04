#pragma once

_connection* ctcp_allocate_connection()
{
	_connection* con = malloc(sizeof(_connection));

	size_t guid_offset = sizeof(_guid) / 2;

	con->guid		  = ((_guid)rand() << guid_offset) + ((_guid)rand());
	con->ping_last	  = time(NULL);
	con->index_next   = 0u;
	con->is_active	  = true;
	con->encryption   = false;
	con->is_logged_in = true;

	memset(&con->key.pub, 0, sizeof(con->key.pub));
	memset(&con->key.priv, 0, sizeof(con->key.priv));

	memset(&con->input, 0, sizeof(con->input));
	memset(&con->output, 0, sizeof(con->output));

	return con;
}

void ctcp_dispose_connection(_connection* con)
{
	free(con);
}