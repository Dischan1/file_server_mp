bool get_directory_object_list(const char* path, char** buffer, size_t* size)
{
	struct dirent* dir;
	DIR* d = opendir(path);
	if (!d) return false;

	size_t buffer_len = *size;
	*size = 0u;

	char temp[512u];

	for (size_t i = 0u; (i < buffer_len || !buffer) && (dir = readdir(d)); ++i, ++ * size)
	{
		if (dir->d_name == 'd')
			sprintf_s_(temp, _countof(temp), "%s/", dir->d_name);
		else
			sprintf_s_(temp, _countof(temp), "%s", dir->d_name);

		size_t temp_len = strlen(temp) + 1;

		if (buffer)
		{
			buffer[i] = malloc(temp_len);
			memcpy(buffer[i], temp, temp_len);
		}
	}

	closedir(d);
	return true;
}