// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>

char *strcpy(char *destination, const char *source)
{
	int i = 0;
    while (source[i] != '\0')
    {
        char a = source[i];
        destination[i] = a;
        i++;
    }
    destination[i] = '\0';
	return destination;
}

char *strncpy(char *destination, const char *source, size_t len)
{
	size_t i = 0;
    while (source[i] != '\0')
    {
        char a = source[i];
        destination[i] = a;
        i++;
		if (i == len)
		{
			break;
		}
    }
	if (i != len)
	{
		destination[i] = '\0';
	}
	return destination;
}

char *strcat(char *destination, const char *source)
{
	int i = 0;
	while (destination[i] != '\0')
	{
		i++;
	}
	int j = 0;
	while (source[j] != '\0')
	{
		destination[i] = source[j];
		i++;
		j++;
	}
	destination[i] = '\0';
	return destination;
}

char *strncat(char *destination, const char *source, size_t len)
{
	size_t i = 0;
	while (destination[i] != '\0')
	{
		i++;
	}
	size_t j = 0;
	while (source[j] != '\0')
	{
		destination[i] = source[j];
		i++;
		j++;
		if (j == len)
		{
			break;
		}
	}
	//if (j != len)
	{
		destination[i] = '\0';
	}
	return destination;
}

int strcmp(const char *str1, const char *str2)
{
	int i = 0, j = 0;
	while (str1[i] == str2[j])
	{
		i++;
		j++;
		if (str1[i] == '\0')
		{
			break;
		}
		if (str2[i] == '\0')
		{
			break;
		}
	}
	if (str1[i] > str2[j])
	{
		return 1;
	}
	if (str1[i] == str2[j])
	{
		return 0;
	}
	return -1;
}

int strncmp(const char *str1, const char *str2, size_t len)
{
	size_t i = 0;
	while (str1[i] == str2[i])
	{
		i++;
		if (str1[i] == '\0' || str2[i] == '\0' || i == (len - 1))
		{
			break;
		}
	}
	if (str1[i] > str2[i])
	{
		return 1;
	}
	if (str1[i] == str2[i])
	{
		return 0;
	}
	return -1;
}

size_t strlen(const char *str)
{
	size_t i = 0;

	for (; *str != '\0'; str++, i++)
		;

	return i;
}

char *strchr(const char *str, int c)
{
	int i = 0;
    while (str[i] != '\0')
    {
        if (c == str[i])
		{
			return (char *)&str[i];
		}
        i++;
    }
	return NULL;
}

char *strrchr(const char *str, int c)
{
	int i = 0, vf = 0;
    while (str[i] != '\0')
    {
        if (c == str[i])
		{
			vf = i;
		}
        i++;
    }
	if (vf)
	{
		return (char *)&str[vf];
	}
	return NULL;
}

char *strstr(const char *haystack, const char *needle)
{
	int i = 0, j = 0;
	while (haystack[i] != '\0')
	{
		int curent = i;
		j = 0;
		while (haystack[curent] == needle[j])
		{
			curent++;
			j++;
			if (needle[j] == '\0')
			{
				return (char *)&haystack[i];
			}
		}
		i++;
	}
	return NULL;
}

char *strrstr(const char *haystack, const char *needle)
{
	int i = 0, j = 0, vf = 0;
	while (haystack[i] != '\0')
	{
		int curent = i;
		j = 0;
		while (haystack[curent] == needle[j])
		{
			curent++;
			j++;
			if (needle[j] == '\0')
			{
				vf = i;
				break;
			}
		}
		i++;
	}
	if (vf)
	{
		return (char *)&haystack[vf];
	}
	return NULL;
}

void *memcpy(void *destination, const void *source, size_t num)
{
	size_t i;
	//int *dest = (int*) destination;
	//int *src = (int*) source;
	char *dest = (char*) destination;
	char *src = (char*) source;
	for(i = 0; i < num; i++)
	{
		dest[i] = src[i];
	}
	return destination;
}

void *memmove(void *destination, const void *source, size_t num)
{
	size_t i;
	char *dest = (char*) destination;
	char *src = (char*) source;
	//char *intermediar;
	if (dest < src)
	{
		for(i = 0; i < num; i++)
		{
			dest[i] = src[i];
		}
	} else {
		for(i = num; i > 0; i --)
		{
			dest[i - 1] = src[i - 1];
		}
	}
	return destination;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
	size_t i;
	char *p1 = (char*) ptr1;
	char *p2 = (char*) ptr2;
	for(i = 0; i < num; i++)
	{
		if (p1[i] > p2[i])
		{
			return 1;
		} else if (p1[i] < p2[i]) {
			return -1;
		}
	}
	return 0;
}

void *memset(void *source, int value, size_t num)
{
	size_t i;
	//int *dest = (int*) destination;
	//int *src = (int*) source;
	char *src = (char*) source;
	for(i = 0; i < num; i++)
	{
		src[i] = value;
	}
	return source;
}
