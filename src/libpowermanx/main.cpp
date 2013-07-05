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
#include <libpowermanx/slot_exe.hpp>
#include <libpowermanx/profile.hpp>
#include <gmodule.h>

static void load_lib(const char *full_name,
		     const struct stat *fs)
{
	if (ends_with(full_name,  "." G_MODULE_SUFFIX)) {
		GModule *module =
			g_module_open(full_name,
				      static_cast<GModuleFlags>(0));
		if (!module) {
			print_debug("%s\n", g_module_error ());
			return;
		}
	}
}

void libpowermanx_init()
{
	static bool initialized = false;

	if (initialized)
		return;
	initialized = true;

	g_type_init();
	dbus_provider_t::init();
	slot_exe_t::init();
	foreach_file(PLUGINSODIR, load_lib);

	profile_t::load();
}
