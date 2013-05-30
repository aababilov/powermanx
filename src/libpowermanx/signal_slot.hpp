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
#ifndef PMX_SIGNAL_SLOT_HPP
#define PMX_SIGNAL_SLOT_HPP

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include <libpowermanx/variant.hpp>
#include <libpowermanx/utils.hpp>

class signal_record_t;
class signal_plugin_t;
class slot_record_t;
class slot_plugin_t;

typedef std::map<std::string, variant_t> property_map_t;

enum signal_settings_t {
	SETTINGS_BOOL,
	SETTINGS_INT,
	SETTINGS_HOTKEY,
	SETTINGS_VOID,
	SETTINGS_OTHER
};

class signal_plugin_t : public named_object_t<signal_plugin_t> {
	DEFINE_PROPERTY_R_SIMPLE(bool, can_timeout);
	DEFINE_PROPERTY_R_SIMPLE(bool, enabled);
public:
	signal_plugin_t(const std::string &name);
	virtual void enable(signal_record_t *rec);
	virtual void disable(signal_record_t *rec);

	virtual signal_settings_t settings_type() const { return SETTINGS_VOID; }

	void activate_records();
	void new_int(int value);
	void new_bool(bool value);
	signal_record_t *create_record();
protected:
	void set_can_timeout(bool can) { m_can_timeout = can; }
	typedef std::set<signal_record_t*> active_recs_t;
	active_recs_t recs;
};

class signal_record_t {
public:
	typedef std::list<slot_record_t *> slots_t;
	DEFINE_PROPERTY_RW_COMPLEX(slots_t, slots);
	DEFINE_PROPERTY_RW_SIMPLE(bool, enabled);
	DEFINE_PROPERTY_R_SIMPLE(signal_plugin_t *, plugin);
	DEFINE_PROPERTY_RW_SIMPLE(int, timeout);
	DEFINE_PROPERTY_R_SIMPLE(bool, waits_timeout);
public:
	signal_record_t(signal_plugin_t *plugin);
	virtual ~signal_record_t();
	virtual std::string to_xml() const = 0;
	void activate();
	void deactivate();
	virtual void set_condition(const char *key, const char *value) = 0;
	virtual std::string description() { return name(); }
	void activate_slots();

	const std::string &name() const { return plugin()->name(); }
	bool can_timeout() const { return m_plugin->can_timeout(); }
	signal_settings_t settings_type() const { return m_plugin->settings_type(); }
	//int std_events() const { return m_plugin->std_events(); }
protected:
	std::string xml_slot_list() const;
	static gboolean activate_slots_callback(gpointer data);
	void register_timeout_event();
	void unregister_timeout_event();
	guint m_source_tag;
};

class signal_record_void_t: public signal_record_t {
public:
	signal_record_void_t(signal_plugin_t *plugin)
		: signal_record_t(plugin) {  }
	virtual std::string to_xml() const;
	virtual void set_condition(const char *key, const char *value) {}
};

class signal_record_ib_t: public signal_record_t {
public:
	enum react_t { ON_CHANGED, ON_SIG_VALUE, ON_INTERVAL };
	DEFINE_PROPERTY_RW_SIMPLE(react_t, react);
public:
	signal_record_ib_t(signal_plugin_t *plugin)
		: signal_record_t(plugin) { m_react = ON_CHANGED; }
};

class signal_record_bool_t: public signal_record_ib_t {
public:
	DEFINE_PROPERTY_RW_SIMPLE(bool, last_value);
	DEFINE_PROPERTY_RW_SIMPLE(bool, sig_value);
public:
	signal_record_bool_t(signal_plugin_t *plugin)
		: signal_record_ib_t(plugin) { m_last_value = m_sig_value = false; }
	virtual std::string to_xml() const;
	virtual void set_condition(const char *key, const char *value);
	virtual std::string description();
	void new_value(bool val);
};

class signal_record_int_t: public signal_record_ib_t {
public:
	DEFINE_PROPERTY_RW_SIMPLE(int, last_value);
	DEFINE_PROPERTY_RW_SIMPLE(int, sig_value);
	DEFINE_PROPERTY_RW_SIMPLE(bool, on_interval_from);
	DEFINE_PROPERTY_RW_SIMPLE(bool, on_interval_to);
	DEFINE_PROPERTY_RW_SIMPLE(int, interval_from);
	DEFINE_PROPERTY_RW_SIMPLE(int, interval_to);
public:
	signal_record_int_t(signal_plugin_t *plugin);
	virtual std::string to_xml() const;
	virtual void set_condition(const char *key, const char *value);
	void new_value(int val);
};

class signal_record_hotkey_t: public signal_record_t {
public:
	DEFINE_PROPERTY_RW_COMPLEX(std::string, key_name);
public:
	signal_record_hotkey_t(signal_plugin_t *plugin)
		: signal_record_t(plugin) {  }
	virtual std::string to_xml() const;
	virtual void set_condition(const char *key, const char *value);
	virtual std::string description();
};

class slot_plugin_t: public named_object_t<slot_plugin_t> {
public:
	DEFINE_PROPERTY_R_SIMPLE(bool, has_param);
	DEFINE_PROPERTY_R_SIMPLE(bool, any_param);
public:
	slot_plugin_t(const std::string &name);
	virtual void activate(const char *param) = 0;
	virtual void reload_params() {}

	PROPERTY_GETTER(const std::vector<std::string>&, avail_params);
	slot_record_t *create_record();
protected:
	void set_has_param(bool has) { m_has_param = has; }
	void set_any_param(bool any) { m_any_param = any; }
	std::vector<std::string> m_avail_params;
};

class slot_record_t {
public:
	DEFINE_PROPERTY_RW_SIMPLE(bool, enabled);
	DEFINE_PROPERTY_R_SIMPLE(slot_plugin_t*, plugin);
	DEFINE_PROPERTY_RW_COMPLEX(std::string, param);
public:
	slot_record_t(slot_plugin_t *plugin) { m_plugin = plugin; m_enabled = true; }
	void activate() { if (m_enabled) m_plugin->activate(m_param.c_str()); }
	std::string to_xml() const;
	void set_param(const char *param) { m_param = param ? param : ""; }
	bool has_param() const { return m_plugin->has_param(); }
	const std::string &name() const { return m_plugin->name(); }
};

#endif /*!defined PM_BASE_HPP*/
