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
#ifndef BASEPLUGINS_HPP
#define BASEPLUGINS_HPP

#include <libpowermanx/libpowermanx.hpp>

class slot_exec_t: public slot_plugin_t {
public:
	slot_exec_t();
	virtual void activate(const char *param);
};

class slot_srsh_t: public slot_plugin_t {
public:
	slot_srsh_t(const std::string &name);
	virtual void activate(const char *param);
private:
	std::string action;
};

class slot_change_profile_t: public slot_plugin_t {
public:
	slot_change_profile_t();
	virtual void activate(const char *param);
	virtual	void reload_params();
};

class slot_lcd_brightness_t: public slot_plugin_t {
public:
	slot_lcd_brightness_t();
	virtual void activate(const char *param);
private:
	bool get_hw(int &val);
	bool set_hw(int val);

	int last_set_hw;
	int levels;
	bool does_own_updates;
	std::string udi;
};

class slot_dim_t: public slot_plugin_t {
public:
	slot_dim_t();
	virtual void activate(const char *param);
private:
};

class signal_activate_t: public signal_plugin_t {
public:
	signal_activate_t(bool act);
	virtual void enable(signal_record_t *rec);
	virtual void disable(signal_record_t *rec);
private:
	bool act;
};

class signal_hotkey_t: public signal_plugin_t, public hal_listener_t {
public:
	signal_hotkey_t();
	virtual signal_settings_t settings_type() const { return SETTINGS_HOTKEY; }

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

};

class signal_idle_t: public signal_plugin_t {
public:
	signal_idle_t();
	virtual signal_settings_t settings_type() const { return SETTINGS_VOID; }
	virtual void enable(signal_record_t *rec);
	virtual void disable(signal_record_t *rec);
private:
	static gboolean idle_timer_callback(gpointer data);
	inline gboolean idle_timer();
	guint src_tag;
	std::list<signal_record_t *> recs;
	std::list<signal_record_t *>::iterator next2run;
};

#endif /*!defined SLOT_BASE_HPP*/
