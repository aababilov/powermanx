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
#ifndef PMX_STATUS_ICON_HPP
#define PMX_STATUS_ICON_HPP

#include <libpowermanx/utils.hpp>
#include <gtk/gtk.h>
#include <set>
#include <string>

#include "profile_edit.hpp"

class pmx_status_icon_t: public hal_listener_t {
public:
	pmx_status_icon_t(GtkBuilder *builder);

	virtual void on_hal_device_presence(
		const char *udi,
		bool present);
	virtual void on_hal_device_capability(
		const char *udi,
		const char *capability,
		bool present);
	virtual void on_hal_device_property_modified(
		const char *udi,
		const char *key,
		dbus_bool_t is_removed,
		dbus_bool_t is_added);
	virtual void on_hal_device_condition(
		const char *udi,
		const char *condition_name,
		const char *condition_detail);

	void icon_changed();
	void profiles_changed();
private:
	GtkStatusIcon *status_icon;
	GtkMenu *menu_settings, *menu_slots;
	GtkWidget *change_profile_menu;
	pmx_profile_edit_t *profile_edit;
	std::set<std::string> batteries;

	void create_menu_settings();
	void create_menu_slots();
	void create_status_icon();

	static void on_status_icon_popup_menu(
		GtkStatusIcon *status_icon,
		guint          button,
		guint32        timestamp,
		pmx_status_icon_t *icon);
	static void on_status_icon_activate(
		GtkStatusIcon *status_icon,
		pmx_status_icon_t   *icon);
	static void on_menu_item_profiles(
		GtkMenuItem *menuitem,
		pmx_status_icon_t *icon);
};


#endif
