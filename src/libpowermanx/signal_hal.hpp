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
#ifndef SIGNAL_STD_HAL_HPP
#define SIGNAL_STD_HAL_HPP

#include <string>
#include <set>
#include <hal/libhal.h>
#include <libpowermanx/signal_slot.hpp>

enum sp_hal_event_t {
	HAL_DEVICE_PRESENCE,
	HAL_DEVICE_CAPABILITY,
	HAL_PROPERTY_PRESENCE,
	HAL_PROPERTY_VALUE,
	HAL_DEVICE_CONDITION,
};

class signal_hal_t: public signal_plugin_t, public hal_listener_t {
public:
	typedef std::set<std::string> hal_watched_devs_t;
	DEFINE_PROPERTY_RW_COMPLEX(property_map_t, identification);
	DEFINE_PROPERTY_R_COMPLEX(std::string, filename);
	DEFINE_PROPERTY_R_COMPLEX(hal_watched_devs_t, watched_devs);
public:
	signal_hal_t(const std::string &name);

	virtual signal_settings_t settings_type() const { return m_settings_type; }
	signal_settings_t &settings_type() { return m_settings_type; }
	void set_event_type(sp_hal_event_t t) { m_hal_event = t; set_can_timeout(t != HAL_DEVICE_CONDITION); }

	virtual void enable(signal_record_t *rec);
	virtual void disable(signal_record_t *rec);

	virtual void on_hal_device_presence(const char *udi,
					    bool present);
	virtual void on_hal_device_capability(const char *udi,
					      const char *capability,
					      bool present);
	virtual void on_hal_device_property_modified(const char *udi,
						     const char *key,
						     dbus_bool_t is_removed,
						     dbus_bool_t is_added);
	virtual void on_hal_device_condition(const char *udi,
					     const char *condition_name,
					     const char *condition_detail);
	bool our_device(const char *udi) const;

	std::string &sig_name() { return m_signalling[0]; }
	std::string &sig_capability() { return m_signalling[0]; }
	const std::string &sig_property() const { return m_signalling[0]; }
	std::string &sig_condition_name() { return m_signalling[0]; }
	std::string &sig_condition_detail() { return m_signalling[1]; }

	void build_watched_dev_list();
	void check_property_value(signal_record_t *rec, const char *udi);
	void check_property_values(const char *udi);
	void load_from_xml();

	static void init();
private:
	std::string m_signalling[2];
	signal_settings_t m_settings_type;
	int m_std_events;
	sp_hal_event_t m_hal_event;
	int m_sig_prop_type;

	enum hal_xml_pos_t {
		PARSER_OUT,
		PARSER_SIGNAL,
		PARSER_IDENTIFICATION,
	};
	static hal_xml_pos_t parser_pos;
	static signal_hal_t *cur_plugin;

	static void load_signal_hal(const char *full_name, const struct stat *fs);
	static void start_element(GMarkupParseContext *context,
				  const gchar         *element_name,
				  const gchar        **attribute_names,
				  const gchar        **attribute_values,
				  gpointer             user_data,
				  GError             **error);
	static void end_element(GMarkupParseContext *context,
				const gchar         *element_name,
				gpointer             user_data,
				GError             **error);
};

#endif /*!defined SIGNAL_STD_HAL_HPP*/
