/*****************************************************************************/
/*
 *      usbmisc.c  --  Misc USB routines
 *
 *      Copyright (C) 2003  Aurelien Jarno (aurelien@aurel32.net)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "usbmisc.h"

/* ---------------------------------------------------------------------- */

static const char *devbususb = "/dev/bus/usb";

/* ---------------------------------------------------------------------- */

static int readlink_recursive(const char *path, char *buf, size_t bufsize)
{
	char temp[PATH_MAX + 1];
	char *ptemp;
	int ret;

	ret = readlink(path, buf, bufsize);

	if (ret > 0) {
		buf[ret] = 0;
		if (*buf != '/') {
			strncpy(temp, path, sizeof(temp));
			ptemp = temp + strlen(temp);
			while (*ptemp != '/' && ptemp != temp)
				ptemp--;
			ptemp++;
			strncpy(ptemp, buf, bufsize + temp - ptemp);
		} else
			strncpy(temp, buf, sizeof(temp));
		return readlink_recursive(temp, buf, bufsize);
	} else {
		strncpy(buf, path, bufsize);
		return strlen(buf);
	}
}

static char *get_absolute_path(const char *path, char *result,
			       size_t result_size)
{
	const char *ppath;	/* pointer on the input string */
	char *presult;		/* pointer on the output string */

	ppath = path;
	presult = result;
	result[0] = 0;

	if (path == NULL)
		return result;

	if (*ppath != '/') {
		result = getcwd(result, result_size);
		presult += strlen(result);
		result_size -= strlen(result);

		*presult++ = '/';
		result_size--;
	}

	while (*ppath != 0 && result_size > 1) {
		if (*ppath == '/') {
			do
				ppath++;
			while (*ppath == '/');
			*presult++ = '/';
			result_size--;
		} else if (*ppath == '.' && *(ppath + 1) == '.' &&
			   *(ppath + 2) == '/' && *(presult - 1) == '/') {
			if ((presult - 1) != result) {
				/* go one directory upper */
				do {
					presult--;
					result_size++;
				} while (*(presult - 1) != '/');
			}
			ppath += 3;
		} else if (*ppath == '.'  &&
			   *(ppath + 1) == '/' &&
			   *(presult - 1) == '/') {
			ppath += 2;
		} else {
			*presult++ = *ppath++;
			result_size--;
		}
	}
	/* Don't forget to mark the end of the string! */
	*presult = 0;

	return result;
}

struct libusb_device *get_usb_device(const char *path)
{
	struct libusb_device **list;
	struct libusb_device *found = NULL;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	ssize_t i = 0;
	int bus, dev;
	char device_path[PATH_MAX + 1];
	char absolute_path[PATH_MAX + 1];

	if (cnt < 0)
		perror ("Can't get device list\n");

	readlink_recursive(path, device_path, sizeof(device_path));
	get_absolute_path(device_path, absolute_path, sizeof(absolute_path));
	
	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		
		bus = libusb_get_bus_number(device);
		dev = libusb_get_device_address(device);

		snprintf(device_path, sizeof(device_path), "%s/%d/%d", devbususb, bus, dev);
		if (!strcmp(device_path, absolute_path)) {
			found = device;
			break;
		}
	}

	libusb_free_device_list(list, 1);

	return found;
}	

