bool get_directory_object_list(const char* path, char** buffer, size_t* size)
{
	WIN32_FIND_DATAA data;

	HANDLE hfile = FindFirstFileA(path, &data);
	if (!hfile || hfile == INVALID_HANDLE_VALUE) return false;

	size_t buffer_len = *size;
	*size = 0u;

	char temp[512u];

	for (size_t i = 0u; (i < buffer_len || !buffer) && FindNextFileA(hfile, &data); ++i, ++ * size)
	{
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			sprintf_s_(temp, _countof(temp), "%s/", data.cFileName);
		else
			sprintf_s_(temp, _countof(temp), "%s", data.cFileName);

		size_t temp_len = strlen(temp) + 1;

		if (buffer)
		{
			buffer[i] = malloc(temp_len);
			memcpy(buffer[i], temp, temp_len);
		}
	}

	FindClose(hfile);
	return true;
}