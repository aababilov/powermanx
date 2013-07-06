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

#include <set>
#include <string>

#include <gtk/gtk.h>
#include <libnotify/notify.h>

#include <libpowermanx/utils.hpp>

#include "profile_edit.hpp"

class pmx_status_icon_t {
public:
	pmx_status_icon_t(GtkBuilder *builder);

	void icon_changed();
	void profiles_changed();
private:
	GtkStatusIcon *status_icon;
	GtkMenu *menu_settings, *menu_slots;
	GtkWidget *change_profile_menu;
	NotifyNotification *notification;
	pmx_profile_edit_t *profile_edit;
	std::set<std::string> batteries;
	double highest_percentage;
	bool on_battery;

	void create_menu_settings();
	void create_menu_slots();
	void create_status_icon();
	bool battery_add(UpDevice *device);
	void do_notify(NotifyUrgency urgency,
		       const char *summary,
		       const char *message,
		       const char *icon);
        void clear_notify();
	void power_notify();

	static void on_upower_device_changed(
		UpClient *client,
		UpDevice *device,
		pmx_status_icon_t *icon);
	static void on_upower_device_added(
		UpClient *client,
		UpDevice *device,
		pmx_status_icon_t *icon);
	static void on_upower_device_removed(
		UpClient *client,
		UpDevice *device,
		pmx_status_icon_t *icon);

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
