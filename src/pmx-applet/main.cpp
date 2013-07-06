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

#include <gtk/gtk.h>
#include <libnotify/notify.h>

#include <libpowermanx/libpowermanx.hpp>

#include "status_icon.hpp"

int main(int argc, char *argv[])
{
	print_debug("client\n");
	libpowermanx_init();

	gtk_init(&argc, &argv);
        if (!notify_is_initted())
                notify_init("PowerManagerX");

	GtkBuilder *builder;
	const char *ui_file =  PKGDATADIR "/profile_edit.ui";

	GError* error = NULL;
	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, ui_file, &error)) {
		g_warning("Couldn't load builder file: %s\n", error->message);
		g_error_free(error);
		return 0;
	}

	pmx_status_icon_t *status_icon = new pmx_status_icon_t(builder);

	gtk_icon_theme_append_search_path(
		gtk_icon_theme_get_default(),
		PKGDATADIR "/icons");

	gtk_main();

	delete status_icon;

	return 0;
}
