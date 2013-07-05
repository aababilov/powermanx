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
#include <string>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libpowermanx/libpowermanx.hpp>

#include "xpm_manager.hpp"


static void dbus_server_init()
{
	DBusGConnection *connection;
	DBusError error;
	GError *gerror = NULL;

	dbus_error_init(&error);
	if (dbus_bus_request_name(glob_dbus_conn, POWERMAN_SERVICE,
				  DBUS_NAME_FLAG_DO_NOT_QUEUE, &error)
	    != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		if (dbus_error_is_set(&error)) {
			print_debug("%s\n",
				    error.message);
			dbus_error_free(&error);
		} else {
			print_debug("%s is busy\n", POWERMAN_SERVICE);
		}
		exit(0);
	}
	print_debug("registered %s", POWERMAN_SERVICE);
	g_type_init();
	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &gerror);
	if (gerror != NULL) {
		print_debug("Unable to connect to dbus: %s\n", gerror->message);
		g_error_free(gerror);
		return;
	}

	xpm_manager_install_info();
	dbus_g_connection_register_g_object(
		connection,  POWERMAN_PATH,
		G_OBJECT(g_object_new(XPM_TYPE_MANAGER, NULL)));
}

int
main(int argc, char *argv[])
{
	GMainLoop *loop;

	libpowermanx_init();
	dbus_server_init();
	profile_t::activate_default();

	loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(loop);
	return 0;
}
