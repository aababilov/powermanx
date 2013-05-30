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
#include <cassert>

#include <errno.h>

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libpowermanx/libpowermanx.hpp>

#define XPM_TYPE_MANAGER                 (xpm_manager_get_type())
#define XPM_MANAGER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), XPM_TYPE_MANAGER, XpmManager))
#define XPM_MANAGER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), XPM_TYPE_MANAGER, XpmManagerClass))
#define XPM_MANAGER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), XPM_TYPE_MANAGER, XpmManagerClass))

using std::string;

typedef struct _XpmManager XpmManager;
typedef struct _XpmManagerClass XpmManagerClass;
//const char *name;

struct _XpmManager {
	GObject parent;
//	int v;
	/* instance members */
};

struct _XpmManagerClass {
	GObjectClass parent;
	/* class members */
};

gboolean pm_svr_get_profile(XpmManager *pman, gchar **profile, GError **error)
{
	*profile = g_strdup(profile_t::active_profile() ?
			    profile_t::active_profile()->name().c_str() :
			    "");
	return TRUE;
}

gboolean pm_svr_set_profile(XpmManager *pman, gchar *profile, GError **error)
{
	profile_t::change_profile(profile);
	return TRUE;
}

gboolean pm_svr_activate_slot(XpmManager *pman, gchar *slot_name, gchar *param, GError **error)
{
	slot_plugin_t *slot = slot_plugin_t::find(slot_name);
	if (!slot) {
		g_set_error(error, g_quark_from_static_string("echo"),
			    0xef,
			    "Slot %s not found",
			    slot_name);
		return FALSE;

	}
	print_debug("activated by dbus: %s, %s\n", slot_name, param);
	slot->activate(param);
	return TRUE;
}

gboolean pm_svr_reload_profiles(XpmManager *pman, gchar **names, gchar **bodies, GError **error)
{
	char *p_name, *p_body;
	char *default_xml = NULL, *active_xml = NULL;
	string active_name = profile_t::active_profile()
		? profile_t::active_profile()->name()
		: string();
	print_debug("%s\n", __func__);
	for (int i = 0; names[i] && bodies[i]; ++i) {
		p_name = names[i];
		p_body = bodies[i];
		printf("%s\n", p_name);
		if (!strcmp(p_name, "default")) {
			default_xml = p_body;
			continue;
		}
		if (!strcmp(p_name, active_name.c_str())) {
			active_xml = p_body;
			continue;
		}
		profile_t::reload(p_name, p_body);
	}
	if (default_xml && default_xml[0]) {
		profile_t::reload("default", default_xml);
	}
	if (active_xml) {
		profile_t::reload(active_name.c_str(), active_xml);
	}
	return TRUE;
}

#include "pm_svr_bindings.h"

/* used by PM_TYPE_MANAGER */
GType xpm_manager_get_type (void);

GType xpm_manager_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (XpmManagerClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,//maman_bar_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (XpmManager),
			0,      /* n_preallocs */
			//maman_bar_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
					       "XpmManagerType",
					       &info, GTypeFlags(0));
	}
	return type;
}

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

	dbus_g_object_type_install_info(XPM_TYPE_MANAGER, &dbus_glib_powerman_object_info);
	dbus_g_connection_register_g_object(
		connection,  POWERMAN_PATH,
		G_OBJECT(g_object_new(XPM_TYPE_MANAGER, NULL)));
}

static void
delete_pid(void)
{
	unlink(PMXD_PID_FILE);
}

static void
create_pidfile()
{
        int pf;
	char pid[9];

        /* remove old pid file */
	unlink(PMXD_PID_FILE);

	/* Make a new one */
	if ((pf = open(PMXD_PID_FILE,
		       O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0644)) > 0) {
		snprintf(pid, sizeof(pid), "%lu\n",(long unsigned) getpid());
		write(pf, pid, strlen(pid));
		close(pf);
		atexit(delete_pid);
	}
}

static void
become_daemon()
{
	int child_pid;
	int dev_null_fd;

	if (chdir("/") < 0) {
		exit(1);
	}

	child_pid = fork();
	switch (child_pid) {
	case -1:
		fprintf(stderr, "Cannot fork(): %s\n", strerror(errno));
		break;
	case 0:
		/* child */
		dev_null_fd = open("/dev/null", O_RDWR);
		/* ignore if we can't open /dev/null */
		if (dev_null_fd >= 0) {
			/* attach /dev/null to stdout, stdin */
			dup2(dev_null_fd, 0);
			dup2(dev_null_fd, 1);
			//dup2(dev_null_fd, 2);
			close(dev_null_fd);
		}

		umask(022);
		break;
	default:
		/* parent */
		exit(0);
		break;
	}

	/* Create session */
	setsid();

	create_pidfile();
}

int
main(int argc, char *argv[])
{
	GMainLoop *loop;

	if (!(argc > 1 &&
	     !strcmp(argv[1], "--no-daemon"))) {
		become_daemon();
		freopen("/var/log/powermanx.log", "a+t", stderr);
	}


	libpowermanx_init();
	dbus_server_init();
	profile_t::activate_default();

	loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(loop);
	return 0;
}
