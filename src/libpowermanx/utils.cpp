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
#include <libpowermanx/utils.hpp>

#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>

using std::string;
using std::atexit;

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
	struct timeval tnow;
	struct tm *tloc;
	va_list l;
	va_start(l, fmt);
	gettimeofday(&tnow, NULL);
        tloc = localtime((time_t *) &tnow.tv_sec);
        fprintf(stderr, "%02d:%02d:%02d: ", tloc->tm_hour, tloc->tm_min, tloc->tm_sec);
	fflush(stderr);
	vfprintf(stderr, fmt, l);
	if (fmt[strlen(fmt) - 1] != '\n')
		fputc('\n', stderr);
	fflush(stderr);
	va_end(l);
}

LibHalContext *hal_provider_t::m_hal_ctx;
DBusConnection *hal_provider_t::m_dbus_conn;
hal_provider_t::listener_list_t hal_provider_t::listeners[EVENT_LAST];

void
hal_provider_t::init()
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
        if ((m_hal_ctx = libhal_ctx_new ()) == NULL) {
                fprintf(stderr, "error: libhal_ctx_new\n");
		goto free_all;
        }
	if (!libhal_ctx_set_dbus_connection(m_hal_ctx, m_dbus_conn)) {
                fprintf(stderr, "error: libhal_ctx_set_dbus_connection: %s: %s\n",
			error.name, error.message);
		goto free_all;
        }
        if (!libhal_ctx_init(m_hal_ctx, &error)) {
                if (dbus_error_is_set(&error)) {
                        fprintf(stderr, "error: libhal_ctx_init: %s: %s\n",
				error.name, error.message);
                }
                fprintf(stderr, "Could not initialise connection to hald.\n"
			"Normally this means the HAL daemon (hald) is not running or not ready.\n");
		goto free_all;
        }

	libhal_ctx_set_device_added(glob_hal_ctx, on_hal_device_added);
	libhal_ctx_set_device_removed(glob_hal_ctx, on_hal_device_removed);
	libhal_ctx_set_device_new_capability(glob_hal_ctx, on_hal_device_new_capability);
	libhal_ctx_set_device_lost_capability(glob_hal_ctx, on_hal_device_lost_capability);
	libhal_ctx_set_device_property_modified(glob_hal_ctx, on_hal_device_property_modified);
	libhal_ctx_set_device_condition(glob_hal_ctx, on_hal_device_condition);
	if (libhal_device_property_watch_all(glob_hal_ctx, &error) == FALSE) {
                fprintf (stderr, "libhal_device_property_watch_all: %s: %s\n",
                         error.name, error.message);
		dbus_error_free(&error);
        }
	atexit(fini);
free_all:
	if (dbus_error_is_set(&error))
		dbus_error_free(&error);
}

void
hal_provider_t::fini()
{
	DBusError error;
	dbus_error_init(&error);
	if (libhal_ctx_shutdown(m_hal_ctx, &error) == FALSE)
                LIBHAL_FREE_DBUS_ERROR(&error);
        libhal_ctx_free(m_hal_ctx);

        dbus_connection_unref(m_dbus_conn);
}

hal_listener_t::hal_listener_t()
{
	m_std_events = 0;
}


hal_listener_t::~hal_listener_t()
{
	unregister_listener();
}

void
hal_listener_t::register_event(signal_event_t event)
{
	register_event_list(1 << event);
}

void
hal_listener_t::register_event_list(int event_list)
{
	unregister_listener();
	m_std_events = event_list;
	for (int i = 0; i < EVENT_LAST; ++i)
		if (m_std_events & (1 << i))
			hal_provider_t::listeners[i].insert(this);
}

void
hal_listener_t::unregister_listener()
{
	for (int i = 0; i < EVENT_LAST; ++i)
		if (m_std_events & (1 << i))
			hal_provider_t::listeners[i].erase(this);
	m_std_events = 0;
}

void
hal_provider_t::on_hal_device_added(LibHalContext *ctx,
				    const char *udi)
{
	FOREACH(listener_list_t, i, listeners[EVENT_HAL_DEVICE_PRESENCE])
		(*i)->on_hal_device_presence(udi, true);
}

void
hal_provider_t::on_hal_device_removed(LibHalContext *ctx,
				      const char *udi)
{
	FOREACH(listener_list_t, i, listeners[EVENT_HAL_DEVICE_PRESENCE])
		(*i)->on_hal_device_presence(udi, false);
}

void
hal_provider_t::on_hal_device_new_capability(LibHalContext *ctx,
					     const char *udi,
					     const char *capability)
{
	FOREACH(listener_list_t, i, listeners[EVENT_HAL_DEVICE_CAPABILITY])
		(*i)->on_hal_device_capability(udi, capability, true);
}

void
hal_provider_t::on_hal_device_lost_capability(LibHalContext *ctx,
					      const char *udi,
					      const char *capability)
{
	FOREACH(listener_list_t, i, listeners[EVENT_HAL_DEVICE_CAPABILITY])
		(*i)->on_hal_device_capability(udi, capability, false);
}

void
hal_provider_t::on_hal_device_property_modified(LibHalContext *ctx,
						const char *udi,
						const char *key,
						dbus_bool_t is_removed,
						dbus_bool_t is_added)
{
	//print_debug("prop mod: %s, %s\n", udi, key);
	FOREACH(listener_list_t, i, listeners[EVENT_HAL_DEVICE_PROPERTY_MODIFIED])
		(*i)->on_hal_device_property_modified(udi, key, is_removed, is_added);
}

void
hal_provider_t::on_hal_device_condition(LibHalContext *ctx,
					const char *udi,
					const char *condition_name,
					const char *condition_detail)
{
	FOREACH(listener_list_t, i, listeners[EVENT_HAL_DEVICE_CONDITION])
		(*i)->on_hal_device_condition(udi, condition_name, condition_detail);
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
