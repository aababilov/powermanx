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
#ifndef PMX_PROFILE_EDIT_HPP
#define PMX_PROFILE_EDIT_HPP

#include <libpowermanx/utils.hpp>
#include <gtk/gtk.h>

class pmx_status_icon_t;

class pmx_profile_edit_t {
public:
	pmx_profile_edit_t(GtkBuilder *builder,
			   pmx_status_icon_t *status_icon);
	void show();
	void save_profiles();
private:
	pmx_status_icon_t *status_icon;
};

#endif
