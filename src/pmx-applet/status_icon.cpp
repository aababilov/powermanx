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

#include "status_icon.hpp"
#include "app_utils.hpp"

#include <libpowermanx/libpowermanx.hpp>

using namespace std;

void
pmx_status_icon_t::clear_notify()
{
        if (notification == NULL)
                return;

        notify_notification_close (notification, NULL);
        g_object_unref(notification);
        notification = NULL;
}

void
pmx_status_icon_t::do_notify(NotifyUrgency urgency,
			     const char *summary,
			     const char *message,
			     const char *icon)
{
	GError *error = NULL;

	clear_notify();
	notification = notify_notification_new(
		summary,
		message,
		icon ? icon : GTK_STOCK_INFO);

	notify_notification_set_hint(notification, "transient", g_variant_new_boolean (TRUE));
	notify_notification_set_urgency(notification, urgency);
	notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);
	if (!notify_notification_show(notification, &error)) {
		g_warning("Failed to show notification: %s",
			  error && error->message ? error->message : "(unknown)");
		g_clear_error(&error);
	}
}

pmx_status_icon_t::pmx_status_icon_t(GtkBuilder *builder)
{
	g_signal_connect(
		glob_up_client,
		"device-changed",
		G_CALLBACK(on_upower_device_changed),
		this);
	g_signal_connect(
		glob_up_client,
		"device-added",
		G_CALLBACK(on_upower_device_added),
		this);
	g_signal_connect(
		glob_up_client,
		"device-removed",
		G_CALLBACK(on_upower_device_removed),
		this);

	create_menu_settings();
	create_menu_slots();

	SET_OBJECT(status_icon);
 	gtk_status_icon_set_tooltip_text(status_icon, "Power Manager Extensible");
 	CONNECT_SIGNAL(status_icon, popup_menu);
	CONNECT_SIGNAL(status_icon, activate);

	create_status_icon();

	up_client_enumerate_devices_sync(glob_up_client, NULL, NULL);
	GPtrArray *devices = up_client_get_devices(glob_up_client);
	for (guint i = 0; i < devices->len; i++) {
		battery_add((UpDevice*)g_ptr_array_index(devices, i));
	}
	g_ptr_array_unref(devices);
	on_battery = !up_client_get_on_battery(glob_up_client);

	icon_changed();
	profile_edit = new pmx_profile_edit_t(builder, this);
}

bool
pmx_status_icon_t::battery_add(UpDevice *device)
{
	UpDeviceKind kind;
	g_object_get(G_OBJECT(device), "kind", &kind, NULL);
	if (kind == UP_DEVICE_KIND_BATTERY) {
		const char *path = up_device_get_object_path(device);
		batteries.insert(path);
		return true;
	}
	return false;
}

void
pmx_status_icon_t::on_upower_device_changed(
	UpClient *client,
	UpDevice *device,
	pmx_status_icon_t *icon)
{
	bool old_on_battery = icon->on_battery;
	icon->icon_changed();
	if (icon->on_battery != old_on_battery) {
		icon->power_notify();
	}
}

void
pmx_status_icon_t::on_upower_device_added(
	UpClient *client,
	UpDevice *device,
	pmx_status_icon_t *icon)
{
	if (icon->battery_add(device)) {
		icon->icon_changed();
		icon->power_notify();
	}
}

void
pmx_status_icon_t::on_upower_device_removed(
	UpClient *client,
	UpDevice *device,
	pmx_status_icon_t *icon)
{
	string path = up_device_get_object_path(device);
	if (icon->batteries.count(path)) {
		icon->batteries.erase(path);
		icon->icon_changed();
		icon->power_notify();
	}
}

static const gchar *
get_icon_index(int percentage)
{
        if (percentage < 10) {
                return "000";
        } else if (percentage < 30) {
                return "020";
        } else if (percentage < 50) {
                return "040";
        } else if (percentage < 70) {
                return "060";
        } else if (percentage < 90) {
                return "080";
        }
        return "100";
}

void
pmx_status_icon_t::power_notify()
{
	if (batteries.size() == 0) {
		do_notify(NOTIFY_URGENCY_NORMAL, _("On AC power"), NULL, "pmx-ac-adapter");
		return;
	}

	string icon_name = string("pmx-primary-") +
		get_icon_index(highest_percentage)
		+ (on_battery ? "" : "-charging");
	char *summary = g_strdup_printf(
		_("On %s power"),
		on_battery ? _("battery") : _("AC"));
	char *message = g_strdup_printf(
			_("Batter level is %.0lf%%"),
			highest_percentage);
	do_notify(NOTIFY_URGENCY_NORMAL, summary, message, icon_name.c_str());
	g_free(message);
	g_free(summary);
}

void
pmx_status_icon_t::icon_changed()
{
	string icon_name;
	highest_percentage = 0;

	if (batteries.size() == 0) {
		gtk_status_icon_set_from_icon_name(
			status_icon,
			"pmx-ac-adapter");
		gtk_status_icon_set_tooltip_text(
			status_icon,
			_("On AC power\nNo battery"));
		return;
	}

	on_battery = up_client_get_on_battery(glob_up_client);
	UpDevice *battery = up_device_new();
	FOREACH_CONST (set<string>, i, batteries) {
		if (!up_device_set_object_path_sync(battery, i->c_str(), NULL, NULL))
			continue;
		double percentage;
		UpDeviceState state;
		g_object_get(battery, "state", &state, "percentage", &percentage, NULL);
		if (highest_percentage < percentage)
			highest_percentage = percentage;
		if (!on_battery && state == UP_DEVICE_STATE_FULLY_CHARGED) {
			gtk_status_icon_set_from_icon_name(
				status_icon,
				"pmx-primary-charged");
			char *msg = g_strdup_printf(
				_("On AC power\nBattery is fully charged (%.0lf%%)"),
				percentage);
			gtk_status_icon_set_tooltip_text(
				status_icon,
				msg);
			g_free(msg);
			return;
		}
	}

	icon_name = string("pmx-primary-") +
		get_icon_index(highest_percentage)
		+ (on_battery ? "" : "-charging");
	gtk_status_icon_set_from_icon_name(
		status_icon,
		icon_name.c_str());
	char *msg = g_strdup_printf(
		_("On %s power\nBattery level is %.0lf%%"),
		on_battery ? _("battery") : _("AC"),
		highest_percentage);
	gtk_status_icon_set_tooltip_text(
		status_icon,
		msg);
	g_free(msg);
}

void
pmx_status_icon_t::on_status_icon_popup_menu(
	GtkStatusIcon *status_icon,
	guint          button,
	guint32        timestamp,
	pmx_status_icon_t *icon)
{
	gtk_menu_popup(GTK_MENU(icon->menu_settings), NULL, NULL,
		       gtk_status_icon_position_menu, status_icon,
		       button, timestamp);

}

void
pmx_status_icon_t::on_status_icon_activate(
	GtkStatusIcon *status_icon,
		pmx_status_icon_t   *icon)
{
	gtk_menu_popup(GTK_MENU(icon->menu_slots), NULL, NULL,
		       gtk_status_icon_position_menu, status_icon,
		       1, gtk_get_current_event_time());
}

void request_activate_slot(const char *name, const char *param)
{
	DBusMessage *message, *result;
	DBusError error;

	//printf("act: %s %s\n", name, param);
	message = dbus_message_new_method_call(
			POWERMAN_SERVICE, POWERMAN_PATH,
			POWERMAN_INTERFACE, "ActivateSlot");
	dbus_message_append_args(
		message,
		DBUS_TYPE_STRING, &name,
		DBUS_TYPE_STRING, &param,
		DBUS_TYPE_INVALID);
	dbus_error_init(&error);
	result = dbus_connection_send_with_reply_and_block(
		glob_dbus_conn, message, -1, &error);
	if (dbus_error_is_set(&error)) {
		print_debug("Couldn't send dbus message: %s\n",
			    error.message);
		dbus_error_free(&error);
	} else {
		dbus_message_unref(result);
	}
	dbus_message_unref(message);
}

void on_menu_item_slot_param(
	GtkMenuItem *menuitem,
	gpointer     slot)
{
	request_activate_slot(
		static_cast<slot_plugin_t*>(slot)->name().c_str(),
		gtk_label_get_text(
			GTK_LABEL(gtk_bin_get_child(GTK_BIN(menuitem)))));
}

void on_menu_item_slot_noparam(
	GtkMenuItem *menuitem,
	gpointer     slot)
{
	request_activate_slot(
		static_cast<slot_plugin_t*>(slot)->name().c_str(),
		"");
}

void
pmx_status_icon_t::on_menu_item_profiles(
	GtkMenuItem *menuitem,
	pmx_status_icon_t *icon)
{
	icon->profile_edit->show();
}

void
pmx_status_icon_t::create_menu_settings()
{
	GtkWidget *item;
	GtkWidget *image;
	menu_settings = (GtkMenu*) gtk_menu_new();

	/* Preferences */
	item = gtk_image_menu_item_new_with_mnemonic(_("_Profiles"));
	image = gtk_image_new_from_icon_name(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	g_signal_connect(G_OBJECT(item), "activate",
			 G_CALLBACK(on_menu_item_profiles), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_settings), item);

	/* Separator for HIG? */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_settings), item);

	/* Help */
	item = gtk_image_menu_item_new_with_mnemonic(_("_Help"));
	image = gtk_image_new_from_icon_name(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	//g_signal_connect(G_OBJECT(item), "activate",  G_CALLBACK(gpm_tray_icon_show_help_cb), icon);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_settings), item);

	gtk_widget_show_all(GTK_WIDGET(menu_settings));
}

void menu_append_params(GtkWidget *param_menu, slot_plugin_t *plugin)
{
	gtk_menu_shell_append(GTK_MENU_SHELL(param_menu), gtk_tearoff_menu_item_new());
	plugin->reload_params();
	FOREACH_CONST(vector<string>, i, plugin->avail_params()) {
		GtkWidget *param_item = gtk_menu_item_new_with_label(
			i->c_str());
		g_signal_connect(G_OBJECT(param_item), "activate",
				 G_CALLBACK(on_menu_item_slot_param), plugin);
		gtk_menu_shell_append(GTK_MENU_SHELL(param_menu), param_item);
	}

}

void
pmx_status_icon_t::create_menu_slots()
{
	menu_slots = (GtkMenu*)gtk_menu_new();
	FOREACH(slot_plugin_t::map_t, i, slot_plugin_t::obj_map()) {
		slot_plugin_t *plugin = i->second;
		GtkWidget *plugin_item = gtk_menu_item_new_with_label(plugin->name().c_str());
		if (plugin->has_param()) {
			GtkWidget *param_menu = gtk_menu_new();
			menu_append_params(param_menu, plugin);
			if (plugin->name() == "change_profile") {
				change_profile_menu = param_menu;
			}
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(plugin_item), param_menu);
		} else {
			g_signal_connect(G_OBJECT(plugin_item), "activate",
					 G_CALLBACK(on_menu_item_slot_noparam), plugin);
		}
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_slots), plugin_item);
	}
	gtk_widget_show_all(GTK_WIDGET(menu_slots));
}

void
pmx_status_icon_t::create_status_icon()
{
}

void
pmx_status_icon_t::profiles_changed()
{
	gtk_container_foreach(GTK_CONTAINER(change_profile_menu),
			      (GtkCallback)(gtk_widget_destroy), NULL);
	menu_append_params(change_profile_menu,
			   slot_plugin_t::find("change_profile"));
	gtk_widget_show_all(change_profile_menu);
}
