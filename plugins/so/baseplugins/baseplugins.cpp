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
#include <cassert>
#include <libpowermanx/profile.hpp>
#include "baseplugins.hpp"
#include <gmodule.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>

#include <algorithm>

#include <utmpx.h>
#include <pwd.h>

#define HAVE_DPMS_EXTENSION

#include <X11/extensions/scrnsaver.h>

#ifdef HAVE_DPMS_EXTENSION
#include <X11/Xproto.h>
#include <X11/extensions/dpms.h>
#endif

#include "get_idletime.hpp"

using std::string;
using namespace std;

Display *find_x_display()
{
	static char env[1024];
	Display *dpy = NULL;
	passwd *passwd_entry;
	struct utmpx *utmp_info;

	while((utmp_info = getutxent()) != NULL) {
		if (utmp_info->ut_type != USER_PROCESS)
			continue;
		passwd_entry = getpwnam(utmp_info->ut_user);
		if(passwd_entry == NULL)
			continue;
		sprintf(env, "XAUTHORITY=%s/.Xauthority", passwd_entry->pw_dir);
		if(putenv(env) != 0)
			continue;
		dpy = XOpenDisplay(utmp_info->ut_host);
		if (dpy)
			break;
	}
	endutxent();
	return dpy;
}

void set_no_dpms_timeouts()
{
	Display *dpy = find_x_display();
	if (dpy == NULL)
		return;
	DPMSSetTimeouts(dpy, 0, 0, 0);
	XCloseDisplay(dpy);
}

signal_idle_t::signal_idle_t()
	: signal_plugin_t("idle")
{
	set_can_timeout(true);
	src_tag = 0;
	next2run = recs.end();
}

gboolean signal_idle_t::idle_timer_callback(gpointer data)
{
	return static_cast<signal_idle_t*>(data)->idle_timer();
}

// never called with an empty list
inline gboolean signal_idle_t::idle_timer()
{
	assert(!recs.empty());

	int idle_time = get_idletime();
	print_debug("%s: idletime = %d", __func__, idle_time);
	if (idle_time == -1)
		return TRUE;

	if (next2run != recs.begin()) {
		list<signal_record_t *>::iterator
			prev = next2run;
		--prev;
		/* NOTE: here I wrote <= */
		if ((*prev)->timeout() <= idle_time) {
			/* ok, system idle is prolongated */
			print_debug("%s: prolongated", __func__);
		} else {
			/* idle was interrupted */
			print_debug("%s: interrupted", __func__);
			next2run = recs.begin();
		}
	}

	while (next2run != recs.end() &&
	       (*next2run)->timeout() <= idle_time) {
		(*next2run)->activate_slots();
		++next2run;
	}
	int next_timeout  = recs.front()->timeout();
	if (next2run != recs.end()) {
		next_timeout = min(
			next_timeout,
			(*next2run)->timeout() - idle_time);
	}
	print_debug("%s: next_timeout = %d", __func__, next_timeout);
	src_tag = g_timeout_add_seconds
		(next_timeout, idle_timer_callback, this);

        return FALSE;
}

void
signal_idle_t::enable(signal_record_t *rec)
{
	if (!rec)
		return;
	int new_timeout = rec->timeout();
	if (new_timeout == 0)
		return;
	init_get_idletime();
	list<signal_record_t *>::iterator
		new_pos = recs.end();
	FOREACH (list<signal_record_t *>, i, recs) {
		if (*i == rec) /* reinserting the same record */
			return;
		if (new_timeout < (*i)->timeout()) {
			print_debug("%d added before %d\n",
				    rec->timeout(),
				    (*i)->timeout());
			new_pos = recs.insert(i, rec);
			break;
		}
	}
	if (new_pos == recs.end()) {
		new_pos = recs.insert(recs.end(), rec);
		print_debug("%d added at the end\n",
			    rec->timeout());
	}
	if (next2run == recs.end()) {
		next2run = new_pos;
	} else if (new_timeout < (*next2run)->timeout()) {
		next2run = new_pos;
	}

	if (src_tag != 0)
		g_source_remove(src_tag);

	idle_timer_callback(this);
}

void
signal_idle_t::disable(signal_record_t *rec)
{
	if (!rec || rec->timeout() == 0)
		return;
	FOREACH (list<signal_record_t *>, i, recs) {
		if (*i == rec) {
			if (src_tag != 0)
				g_source_remove(src_tag);
			if (i == next2run)
				++next2run;
			recs.erase(i);
			if (!recs.empty()) {
				idle_timer_callback(this);
			}
			return;
		}
	}
}

slot_change_profile_t::slot_change_profile_t()
	: slot_plugin_t ("change_profile")
{
	set_has_param(true);
}

void slot_change_profile_t::activate(const char *param)
{
	profile_t::change_profile(param);
}

void slot_change_profile_t::reload_params()
{
	m_avail_params.clear();
	FOREACH(profile_t::map_t, i, profile_t::obj_map())
		m_avail_params.push_back(i->second->name());
	sort(m_avail_params.begin(), m_avail_params.end());
}

slot_exec_t::slot_exec_t()
	: slot_plugin_t("exec")
{
	set_has_param(true);
	set_any_param(true);
}

void
slot_exec_t::activate(const char *param)
{
	g_spawn_command_line_async(param, NULL);
}

slot_srsh_t::slot_srsh_t(const std::string &name)
	: slot_plugin_t(name)
{
	set_has_param(false);
	action = name;
	action[0] = toupper(action[0]);
}

void slot_srsh_t::activate(const char *param)
{
	DBusMessage *message, *result;
	DBusError error;

	print_debug("try %s...", action.c_str());
	if (action == "Suspend" || action == "Hibernate") {
		message = dbus_message_new_method_call(
			"org.freedesktop.UPower",
			"/org/freedesktop/UPower",
			"org.freedesktop.UPower",
			action.c_str());
	} else {
		// shutdown, reboot
		message = dbus_message_new_method_call(
			"org.freedesktop.ConsoleKit",
			"/org/freedesktop/ConsoleKit/Manager",
			"org.freedesktop.ConsoleKit.Manager",
			action == "Reboot" ? "Restart": "Stop");
	}
	dbus_error_init(&error);
	result = dbus_connection_send_with_reply_and_block(glob_dbus_conn, message, -1, &error);
	if (dbus_error_is_set(&error)) {
		print_debug("Couldn't send dbus message: %s",
			    error.message);
		dbus_error_free(&error);
	} else {
		dbus_message_unref(result);
	}
	dbus_message_unref(message);
}

#define egg_warning(arg...) print_debug("warning: " arg)

guint
gpm_brightness_get_step (guint levels)
{
	if (levels > 20) {
		/* macbook pro has a bazzillion brightness levels, do in 5% steps */
		return levels / 20;
	}
	return 1;
}
guint
gpm_percent_to_discrete (guint percentage, guint levels)
{
	/* check we are in range */
	if (percentage > 100) {
		return levels;
	}
	if (levels == 0) {
		egg_warning ("levels is 0!");
		return 0;
	}
	return ((gfloat) percentage * (gfloat) (levels - 1)) / 100.0f;
}

guint
gpm_discrete_to_percent (guint discrete, guint levels)
{
	/* check we are in range */
	if (discrete > levels) {
		return 100;
	}
	if (levels == 0) {
		egg_warning ("levels is 0!");
		return 0;
	}
	return (guint) ((gfloat) discrete * (100.0f / (gfloat) (levels - 1)));
}

//slot_dim_t
slot_dim_t::slot_dim_t()
	: slot_plugin_t("dim")
{
	set_has_param(true);
	m_avail_params.push_back("on");
	m_avail_params.push_back("standby");
	m_avail_params.push_back("suspend");
	m_avail_params.push_back("off");
	set_no_dpms_timeouts();
}

void slot_dim_t::activate(const char *param)
{
#ifdef HAVE_DPMS_EXTENSION
	CARD16 current_mode;
	CARD16 needed_mode;
	BOOL current_enabled;

	//mode is DPMSModeOn, DPMSModeStandby, DPMSModeSuspend, or DPMSModeOff
	if (!strcasecmp(param, "on"))
		needed_mode = DPMSModeOn;
	else if (!strcasecmp(param, "standby"))
		needed_mode = DPMSModeStandby;
	else if (!strcasecmp(param, "suspend"))
		needed_mode = DPMSModeSuspend;
	else
		needed_mode = DPMSModeOff;

	Display *dpy = find_x_display();
	if (dpy == NULL) {
		print_debug("couldn't open display");
		return;
	}

	if (!DPMSCapable(dpy)) {
		print_debug("display is not DPMS-capable");
		goto close_disp;
	}
	if (!DPMSInfo(dpy, &current_mode, &current_enabled)) {
		print_debug("couldn't get DPMS info");
		goto close_disp;
	}

	if (!current_enabled) {
		DPMSEnable(dpy);
		print_debug("DPMS was not enabled - enabling");
	}
	DPMSSetTimeouts(dpy, 0, 0, 0);
	if (current_mode != needed_mode) {
		sleep(1);
		if (!DPMSForceLevel(dpy, needed_mode)) {
			print_debug("Could not change DPMS mode");
		} else  {
			XSync(dpy, FALSE);
		}
	}
close_disp:
	XCloseDisplay(dpy);
#endif /* HAVE_DPMS_EXTENSION */
}

// signal_activate_t
signal_activate_t::signal_activate_t(bool act)
	: signal_plugin_t(
		act ? "profile_activated"
		: "profile_deactivated")
{
	this->act = act;
}

void
signal_activate_t::enable(signal_record_t *rec)
{
	if (act)
		rec->activate_slots();
}

void
signal_activate_t::disable(signal_record_t *rec)
{
	if (!act)
		rec->activate_slots();
}

// signal_hotkey_t
/*
signal_hotkey_t::signal_hotkey_t()
	:signal_plugin_t("hotkey")
{
	register_event(EVENT_HAL_DEVICE_CONDITION);
}

void
signal_hotkey_t::on_hal_device_presence(
	const char *udi,
	bool present)
{
}

void
signal_hotkey_t::on_hal_device_capability(
	const char *udi,
	const char *capability,
	bool present)
{
}

void
signal_hotkey_t::on_hal_device_property_modified(
	const char *udi,
	const char *key,
	dbus_bool_t is_removed,
	dbus_bool_t is_added)
{
}

void
signal_hotkey_t::on_hal_device_condition(
	const char *udi,
	const char *condition_name,
	const char *condition_detail)
{
	if (!strcmp(condition_name, "ButtonPressed")) {
		FOREACH(active_recs_t, i, recs) {
			const char *key_name =
				static_cast<signal_record_hotkey_t*>(*i)->
				key_name().c_str();
			if (!strcmp(condition_detail, key_name)) {
				(*i)->activate_slots();
			}
		}
	}
}
*/

signal_line_power_t::signal_line_power_t()
	: signal_plugin_t("line_power")
{
	g_signal_connect(glob_up_client, "notify::on-battery",
			 G_CALLBACK (up_client_on_battery_cb), this);
}

void
signal_line_power_t::up_client_on_battery_cb(
	UpClient *client,
	GParamSpec *pspec,
	signal_line_power_t *signal)
{
        signal->new_bool(up_client_get_on_battery(glob_up_client));
}

signal_battery_percent_t::signal_battery_percent_t()
	: signal_plugin_t("battery_percent")
{
	g_signal_connect(
		glob_up_client,
		"device-changed",
		G_CALLBACK(on_upower_device_changed),
		this);
}

void
signal_battery_percent_t::on_upower_device_changed(
	UpClient *client,
	UpDevice *device,
	signal_battery_percent_t *signal)
{
	UpDeviceKind kind;
	g_object_get(G_OBJECT(device), "kind", &kind, NULL);
	if (kind == UP_DEVICE_KIND_BATTERY) {
		double percentage;
		g_object_get(G_OBJECT(device), "percentage", &percentage, NULL);
		signal->new_int(percentage);
	}

}

extern "C" const gchar *G_MODULE_EXPORT
g_module_check_init(GModule *module)
{
	static bool initialized = false;

	if (initialized)
		return NULL;
	initialized = true;

	new slot_exec_t();
	new slot_srsh_t("suspend");
	new slot_srsh_t("hibernate");
	new slot_srsh_t("shutdown");
	new slot_srsh_t("reboot");
	new slot_change_profile_t();
	new slot_dim_t();

	new signal_activate_t(true);
	new signal_activate_t(false);
	new signal_idle_t();
	new signal_line_power_t();
	new signal_battery_percent_t();
//	new signal_hotkey_t();

	return NULL;
}
