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
#include <string>

#include <libpowermanx/libpowermanx.hpp>
#include "xpm_manager.hpp"

using std::string;

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

void xpm_manager_install_info(void)
{
	dbus_g_object_type_install_info(XPM_TYPE_MANAGER, &dbus_glib_powerman_object_info);
}
