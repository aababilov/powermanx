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
#include <libpowermanx/signal_slot.hpp>

using std::string;
using std::list;
using std::map;
using std::set;

string escape_text(const string &s)
{
	char *esc = g_markup_escape_text(s.c_str(), s.length());
	string r = esc;
	free(esc);
	return r;
}

string make_condition(const string &key)
{
	return "\t\t\t<condition key=\"" + key + "\"/>\n";
}
string make_condition(const string &key, const string &value)
{
	return "\t\t\t<condition key=\"" + key + "\" value=\""
		+ escape_text(value) + "\"/>\n";
}
template<typename T>
string make_condition(const string &key, const T value)
{
	return "\t\t\t<condition key=\"" + key + "\" value=\""
		+ to_string(value) + "\"/>\n";
}


DEFINE_OBJ_MAP(signal_plugin_t);

signal_plugin_t::signal_plugin_t(const std::string &name)
	: named_object_t<signal_plugin_t>(name)
{
	m_can_timeout = false;
	m_enabled = false;
}

void signal_plugin_t::enable(signal_record_t *rec) {
	recs.insert(rec);
}

void signal_plugin_t::disable(signal_record_t *rec) {
	active_recs_t::iterator i = recs.find(rec);
	if (i != recs.end()) {
		recs.erase(rec);
	}
}

void signal_plugin_t::activate_records()
{
	FOREACH(active_recs_t, i, recs)
		(*i)->activate_slots();
}

void signal_plugin_t::new_int(int value)
{
	FOREACH(active_recs_t, i, recs)
		static_cast<signal_record_int_t*>(*i)->new_value(value);
}

void signal_plugin_t::new_bool(bool value)
{
	FOREACH(active_recs_t, i, recs)
		static_cast<signal_record_bool_t*>(*i)->new_value(value);
}


signal_record_t *signal_plugin_t::create_record()
{
	switch (settings_type()) {
	case SETTINGS_BOOL:
		return new signal_record_bool_t(this);
	case SETTINGS_INT:
		return new signal_record_int_t(this);
	case SETTINGS_HOTKEY:
		return new signal_record_hotkey_t(this);
	case SETTINGS_VOID:
		return new signal_record_void_t(this);
	default: //SETTINGS_OTHER
		return NULL;//
	}

}

signal_record_t::signal_record_t(signal_plugin_t *plugin)
{
	m_enabled = true;
	m_waits_timeout = false;
	m_timeout = 0;
	m_plugin = plugin;
}

signal_record_t::~signal_record_t()
{
	deactivate();
	FOREACH(slots_t, i, m_slots)
		delete *i;
}

void signal_record_t::activate()
{
	if (m_enabled)
		m_plugin->enable(this);
}

void signal_record_t::deactivate()
{
	if (m_enabled) {
		unregister_timeout_event();
		m_plugin->disable(this);
	}
}

void signal_record_t::activate_slots()
{
	print_debug("%s: %s\n", name().c_str(), __func__);
	m_waits_timeout = false;
	FOREACH(slots_t, i, m_slots)
		(*i)->activate();
}

string signal_record_t::xml_slot_list() const
{
	string s = "\t\t<slot_list>\n";
	FOREACH_CONST(slots_t, i, m_slots)
		s += "\t\t\t" + (*i)->to_xml();
	return s + "\t\t</slot_list>\n";
}

gboolean signal_record_t::activate_slots_callback(gpointer data)
{
	signal_record_t *rec = static_cast<signal_record_t*>(data);
	//TODO: recheck if condition is true??

	rec->activate_slots();
        return FALSE;
}

typedef map<signal_record_t *, guint> timeout_events_t;
static timeout_events_t timeout_events;

void signal_record_t::register_timeout_event()
{
	if (!can_timeout()) {
		activate_slots();
		return;
	}
	if (m_waits_timeout)
		return;
	m_source_tag = g_timeout_add_seconds
		(m_timeout, activate_slots_callback, this);
	m_waits_timeout = true;
	print_debug("%s %s\n", __func__, name().c_str());
}

void signal_record_t::unregister_timeout_event()
{
	if (!can_timeout())
		return;
	if (!m_waits_timeout)
		return;
	m_waits_timeout = false;
	g_source_remove(m_source_tag);
	print_debug("%s %s\n", __func__, name().c_str());
}

string signal_record_void_t::to_xml() const
{
	string s = "\t<signal name=\"" + escape_text(name()) + "\" enabled=\""
		+ to_string(enabled()) + "\"";
	if (can_timeout())
		s += " timeout=\"" + to_string(timeout()) + "\"";
	s += ">\n" + xml_slot_list() +
		"\t</signal>\n";
	return s;
}

string signal_record_bool_t::to_xml() const
{
	string s = "\t<signal name=\"" + escape_text(name()) +
		"\" enabled=\"" + to_string(enabled()) + "\"";
	if (can_timeout() && react() != ON_CHANGED)
		s += " timeout=\"" + to_string(timeout()) + "\"";
	s += ">\n\t\t<condition_list>\n";
	switch (react()) {
	case ON_CHANGED:
		s += make_condition("on_changed");
		break;
	default:
		s += make_condition("on_sig_value", m_sig_value);
		break;
	}
	s += "\t\t</condition_list>\n" +
		xml_slot_list() +
		"\t</signal>\n";
	return s;
}

void signal_record_bool_t::set_condition(const char *key, const char *value)
{
	if (!strcmp(key, "on_changed")) {
		react() = ON_CHANGED;
	} else if (!strcmp(key, "on_sig_value")) {
		react() = ON_SIG_VALUE;
		m_sig_value = value ?
			value[0] == '1' || value[0] == 't' || value[0] == 'y'
			|| !strcmp(value, "on")
			: false;
	}
}

string signal_record_bool_t::description()
{
	return name() + " " +
		(react() == ON_CHANGED
		 ? "changed" : m_sig_value ? "on":"off");
}

void signal_record_bool_t::new_value(bool value)
{
	switch (react()) {
	case ON_CHANGED:
		if (m_last_value != value)
			activate_slots();
		break;
	default:
		if (m_sig_value == value)
			register_timeout_event();
		else
			unregister_timeout_event();
		break;
	}
	m_last_value = value;
}

signal_record_int_t::signal_record_int_t(signal_plugin_t *plugin)
	: signal_record_ib_t(plugin)
{
	m_last_value = m_sig_value;
	m_interval_from = m_interval_to = 0;
	m_on_interval_from = m_on_interval_to = false;
}

string signal_record_int_t::to_xml() const
{
	string s = "\t<signal name=\"" + escape_text(name()) +
		"\" enabled=\"" + to_string(enabled()) + "\"";
	if (can_timeout() && react() != ON_CHANGED)
		s += " timeout=\"" + to_string(timeout()) + "\"";
	s += ">\n\t\t<condition_list>\n";
	switch (react()) {
	case ON_CHANGED:
		s += make_condition("on_changed");
		break;
	case ON_SIG_VALUE:
		s += make_condition("on_sig_value", m_sig_value);
		break;
	default:
		if (m_on_interval_from)
			s += make_condition("on_interval_from",	m_interval_from);
		if (m_on_interval_to)
			s += make_condition("on_interval_to", m_interval_to);
		break;
	}

	s += "\t\t</condition_list>\n" +
		xml_slot_list() +
		"\t</signal>\n";
	return s;
}

void signal_record_int_t::set_condition(const char *key, const char *value)
{
	if (!strcmp(key, "on_changed")) {
		react() = ON_CHANGED;
	} else if (!strcmp(key, "on_sig_value")) {
		react() = ON_SIG_VALUE;
		m_sig_value = atoi(value);
	} else if (!strcmp(key, "on_interval_to")) {
		react() = ON_INTERVAL;
		m_interval_to = atoi(value);
		m_on_interval_to = true;
	} else if (!strcmp(key, "on_interval_from")) {
		react() = ON_INTERVAL;
		m_interval_from = atoi(value);
		m_on_interval_from = true;
	}
}

void signal_record_int_t::new_value(int value)
{
	switch (react()) {
	case ON_CHANGED:
		if (m_last_value != value)
			activate_slots();
		break;
	case ON_SIG_VALUE:
		if (m_sig_value == value)
			register_timeout_event();
		else
			unregister_timeout_event();
		break;
	default:
		if ((!m_on_interval_from || m_interval_from <= value)
		    && (!m_on_interval_to || m_interval_to >= value))
			register_timeout_event();
		else
			unregister_timeout_event();
		break;
	}
	m_last_value = value;
}

string signal_record_hotkey_t::to_xml() const
{
	string s = "\t<signal name=\"" + escape_text(name()) + "\" enabled=\""
		+ to_string(enabled()) + "\">\n"
		"\t\t<condition_list>\n"
		+ make_condition("key_name", m_key_name)
		+ "\t\t</condition_list>\n"
		+ xml_slot_list()
		+ "\t</signal>\n";
	return s;
}

void signal_record_hotkey_t::set_condition(const char *key, const char *value)
{
	if (!strcmp(key, "key_name"))
		m_key_name = value;
}

string signal_record_hotkey_t::description()
{
	return "hotkey " + m_key_name;
}

DEFINE_OBJ_MAP(slot_plugin_t);

slot_plugin_t::slot_plugin_t(const std::string &name)
	: named_object_t<slot_plugin_t>(name)
{
	m_any_param = false;
	m_has_param = false;
}

slot_record_t *slot_plugin_t::create_record()
{
	return new slot_record_t(this);
}

string slot_record_t::to_xml() const
{
	string s = "<slot name=\"" + escape_text(name()) + "\" enabled=\""
		+ to_string(m_enabled) + "\"";
	if (has_param()) {
		s += " param=\"" + escape_text(m_param) + "\"";
	}
	s += "/>\n";
	return s;
}

