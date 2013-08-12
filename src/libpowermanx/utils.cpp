/*  PowerManX - Extensible power manager
    Copyright (C) 2009  Alessio Ababilov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>

#include <libpowermanx/utils.hpp>


using std::string;
using std::atexit;


DBusConnection *dbus_provider_t::m_dbus_conn;
UpClient *dbus_provider_t::m_up_client;

void
dbus_provider_t::init()
{
	DBusError error;
	dbus_error_init(&error);
	m_dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (m_dbus_conn == NULL) {
                fprintf(stderr, "error: dbus_bus_get: %s: %s\n",
                       error.name, error.message);
		goto free_all;
        }
	dbus_connection_setup_with_g_main(m_dbus_conn, NULL);

	m_up_client = up_client_new ();

	atexit(fini);
free_all:
       if (dbus_error_is_set(&error))
               dbus_error_free(&error);
}

void
dbus_provider_t::fini()
{
	g_object_unref(m_up_client);
	dbus_connection_unref(m_dbus_conn);
}

string to_string(bool b)
{
	return b ? "true" : "false";
}

string to_string(int i)
{
	static char buf[16];
	snprintf(buf, 16, "%d", i);
	return buf;
}

void
print_debug(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	if (fmt[strlen(fmt) - 1] != '\n')
		fputc('\n', stderr);
	fflush(stderr);
	va_end(l);
}

int
foreach_file(const char *dir_name, void (*f)(const char *full_name, const struct stat *fs))
{
	struct dirent *dent;
	struct stat fs;
	DIR *dir = opendir(dir_name);

	if (!dir) {
		print_debug("Couldn't open the directory %s\n", dir_name);
		return 1;
	}
	char *fname = new char[strlen(dir_name) + 2 + NAME_MAX];
	while ((dent = readdir(dir)) != NULL) {
		if (dent->d_name[0] == '.')
			continue;
		sprintf(fname, "%s/%s", dir_name, dent->d_name);
		stat(fname, &fs);
		f(fname, &fs);
	}
	delete [] fname;
	closedir(dir);
	return 0;
}

int ends_with(const char *s, const char *e)
{
	return !strcmp(s + strlen(s) - strlen(e), e);
}
