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
#ifndef PMX_APP_UTILS_HPP
#define PMX_APP_UTILS_HPP

#include <glib/gi18n.h>

#define SET_OBJECT(name) do {						\
		name = reinterpret_cast<typeof(name)>(gtk_builder_get_object(builder, #name)); \
		if (name == NULL)  { printf("===============cannot find object " #name "!\n"); } \
	} while(0)

#define CONNECT_SIGNAL(sender, name)					\
	g_signal_connect(G_OBJECT(sender), #name,			\
			 G_CALLBACK(on_##sender##_##name), this)

#define CONNECT_SIGNAL_NAMED(sender, s, name)			\
	g_signal_connect(G_OBJECT(sender), #name,		\
			 G_CALLBACK(on_##s##_##name), this)

#define SETUP_COLUMN_SORTABILITY(name, id) do {			\
		GtkTreeViewColumn *col_##name;			\
		SET_OBJECT(col_##name);				\
		gtk_tree_view_column_set_sort_column_id(	\
			col_##name, id);			\
	} while (0)

#define SET_RENDERER_TEXT(column)					\
	do {								\
		GtkCellRenderer *ren_##column;				\
		SET_OBJECT(ren_##column);				\
		CONNECT_SIGNAL_NAMED(ren_##column, column, edited);	\
	} while (0)

#define SET_RENDERER_TEXT_SORTABLE(column, id)				\
	do {								\
		SET_RENDERER_TEXT(column);				\
		SETUP_COLUMN_SORTABILITY(column, id);			\
	} while (0)

#define SET_RENDERER_BOOL(column) do {					\
		GtkCellRenderer *ren_##column;				\
		SET_OBJECT(ren_##column);				\
		CONNECT_SIGNAL_NAMED(ren_##column, column, toggled);	\
	} while (0)

#endif
