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
#include "signal_hal.hpp"

using std::map;
using std::string;

#define FOREACH_REC FOREACH(active_recs_t, i, recs)

signal_hal_t::signal_hal_t(const std::string &name)
	: signal_plugin_t(name) {
	m_sig_prop_type = 0;
}

void
signal_hal_t::build_watched_dev_list()
{
	int num_devices;
	char **device_names;
	DBusError error;

	dbus_error_init (&error);
	device_names = libhal_get_all_devices(glob_hal_ctx, &num_devices, &error);
	if (device_names == NULL) {
		LIBHAL_FREE_DBUS_ERROR (&error);
		print_debug("Couldn't obtain list of devices\n");
	}
	m_watched_devs.clear();
	for (int i = 0; i < num_devices; ++i) {
		if (our_device(device_names[i]))
			m_watched_devs.insert(device_names[i]);
	}
	libhal_free_string_array (device_names);
	if (dbus_error_is_set(&error))
		dbus_error_free(&error);

}

void
signal_hal_t::enable(signal_record_t *rec)
{
	signal_plugin_t::enable(rec);
	build_watched_dev_list();
	FOREACH(hal_watched_devs_t, i, m_watched_devs) {
		switch (m_hal_event) {
		case HAL_DEVICE_PRESENCE:
//		static_cast<signal_record_bool_t*>(rec)->new_value(libhal_device_exists(glob_hal_ctx, );
			break;
		case HAL_DEVICE_CAPABILITY:
			break;
		case HAL_PROPERTY_PRESENCE:
			break;
		case HAL_PROPERTY_VALUE:
			check_property_value(rec, i->c_str());
			break;
		case HAL_DEVICE_CONDITION:
			break;
		}
	}
}

void
signal_hal_t::disable(signal_record_t *rec)
{
	signal_plugin_t::disable(rec);
}

bool signal_hal_t::our_device(const char *udi) const
{
	int type;
	char *str;
	DBusError error;

	dbus_error_init (&error);
	//print_debug("check our: %s\n", udi);
	bool ret = false;
	FOREACH_CONST(property_map_t, i, m_identification) {
		const char *key = i->first.c_str();
		const variant_t &val = i->second;
		//print_debug("key %s\n", key);
		type = libhal_device_get_property_type(glob_hal_ctx, udi, key, &error);
		if (type == LIBHAL_PROPERTY_TYPE_INVALID ||
		    !(type == val.which_type() ||
		      (val.which_type() == DBUS_TYPE_STRING &&
		       (type == LIBHAL_PROPERTY_TYPE_STRING ||
			type == LIBHAL_PROPERTY_TYPE_STRLIST)))) {
			//print_debug("\t~type: %d %d\n", type, val.which_type());
			ret = false;
			goto checked;
		}
		switch (type) {
		case LIBHAL_PROPERTY_TYPE_STRING:
			str = libhal_device_get_property_string(glob_hal_ctx, udi, key, &error);
			ret = val == str;
			libhal_free_string (str);
			break;
		case LIBHAL_PROPERTY_TYPE_STRLIST:
		{
			unsigned int i;
			char **strlist;
			ret = false;
			strlist = libhal_device_get_property_strlist(glob_hal_ctx, udi, key, &error);
			/* may be NULL because property may have been removed */
			if (strlist == NULL)
				break;
			for (i = 0; strlist[i] != NULL; i++)
				if (val == strlist[i]) {
					//print_debug("met on %s\n", strlist[i]);
					ret = true;
					break;
				}
			libhal_free_string_array (strlist);
			break;
		}
		case LIBHAL_PROPERTY_TYPE_INT32:
			ret = val == libhal_device_get_property_int(glob_hal_ctx, udi, key, &error);
			break;
		case LIBHAL_PROPERTY_TYPE_UINT64:
			ret = val == libhal_device_get_property_uint64(glob_hal_ctx, udi, key, &error);
			break;
		case LIBHAL_PROPERTY_TYPE_DOUBLE:
			ret = val == libhal_device_get_property_double(glob_hal_ctx, udi, key, &error);
			break;
		case LIBHAL_PROPERTY_TYPE_BOOLEAN:
			ret = val == (bool)libhal_device_get_property_bool(glob_hal_ctx, udi, key, &error);
			break;
		default:
			ret = false;
			break;
		}
		if (!ret)
			goto checked;
	}
checked:
	if (dbus_error_is_set(&error))
		dbus_error_free(&error);
	return ret;
}

void
signal_hal_t::on_hal_device_presence(const char *udi,
						 bool present)
{
	if (!our_device(udi))
		return;
	new_bool(present);
}

void
signal_hal_t::on_hal_device_capability(const char *udi,
						   const char *capability,
						   bool present)
{
	if (!our_device(udi))
		return;
	if (strcmp(capability, sig_capability().c_str()) == 0) {
		new_bool(present);
	}
}

void
signal_hal_t::check_property_value(signal_record_t *rec, const char *udi)
{
	DBusError error;
	const char *key = sig_property().c_str();
	dbus_error_init (&error);

	char *str;
	if (m_sig_prop_type == 0)
		m_sig_prop_type = libhal_device_get_property_type(glob_hal_ctx, udi, key, &error);
	switch (m_sig_prop_type) {
	case LIBHAL_PROPERTY_TYPE_STRING:
		//TODO: string
		str = libhal_device_get_property_string (glob_hal_ctx, udi, key, &error);
		libhal_free_string(str);
		break;
	case LIBHAL_PROPERTY_TYPE_INT32:
		static_cast<signal_record_int_t*>(rec)->
			new_value(libhal_device_get_property_int(glob_hal_ctx, udi, key, &error));
		break;
	case LIBHAL_PROPERTY_TYPE_UINT64:
		static_cast<signal_record_int_t*>(rec)->
			new_value(libhal_device_get_property_uint64(glob_hal_ctx, udi, key, &error));
		break;
	case LIBHAL_PROPERTY_TYPE_DOUBLE:
		//TODO:double
		//libhal_device_get_property_double(glob_hal_ctx, udi, key, &error);
		break;
	case LIBHAL_PROPERTY_TYPE_BOOLEAN:
		static_cast<signal_record_bool_t*>(rec)->
			new_value(libhal_device_get_property_bool(glob_hal_ctx, udi, key, &error));
		break;
	}

}

void
signal_hal_t::check_property_values(const char *udi)
{
	DBusError error;
	const char *key = sig_property().c_str();
	dbus_error_init (&error);

	char *str;
	if (m_sig_prop_type == 0)
		m_sig_prop_type = libhal_device_get_property_type(glob_hal_ctx, udi, key, &error);
	switch (m_sig_prop_type) {
	case LIBHAL_PROPERTY_TYPE_STRING:
		//TODO: string
		str = libhal_device_get_property_string (glob_hal_ctx, udi, key, &error);
		libhal_free_string(str);
		break;
	case LIBHAL_PROPERTY_TYPE_INT32:
		new_int(libhal_device_get_property_int(glob_hal_ctx, udi, key, &error));
		break;
	case LIBHAL_PROPERTY_TYPE_UINT64:
		new_int(libhal_device_get_property_uint64(glob_hal_ctx, udi, key, &error));
		break;
	case LIBHAL_PROPERTY_TYPE_DOUBLE:
		//TODO:double
		//libhal_device_get_property_double(glob_hal_ctx, udi, key, &error);
		break;
	case LIBHAL_PROPERTY_TYPE_BOOLEAN:
		new_bool(libhal_device_get_property_bool(glob_hal_ctx, udi, key, &error));
		break;
	}

}

void
signal_hal_t::on_hal_device_property_modified(const char *udi,
							  const char *key,
							  dbus_bool_t is_removed,
							  dbus_bool_t is_added)
{
	if (!our_device(udi))
		return;
	if (strcmp(key, sig_property().c_str()) != 0)
		return;
	//print_debug("uour!\n");
	//print_debug("prop mod: %s %s, %s\n", name().c_str(), udi, key);

	if (m_hal_event == HAL_PROPERTY_PRESENCE) {
		if (is_removed) {
			new_bool(false);
		}
		else if (is_added) {
			new_bool(true);
		}
	} else {
		check_property_values(udi);
	}
}

void
signal_hal_t::on_hal_device_condition(const char *udi,
					   const char *condition_name,
					   const char *condition_detail)
{
	//print_debug("cond: %s %s\n",  condition_name, condition_detail);
	if (!our_device(udi))
		return;
	//print_debug("cond: %s %s\n",  condition_name, condition_detail);
	if (strcmp(condition_name, sig_condition_name().c_str()) == 0
	    && strcmp(condition_detail, sig_condition_detail().c_str()) == 0) {
		activate_records();
	}
}

signal_hal_t::hal_xml_pos_t signal_hal_t::parser_pos;
signal_hal_t *signal_hal_t::cur_plugin;

static signal_settings_t str2stype(const char *name)
{
	if (!name)
		return SETTINGS_BOOL;
	switch (name[0]) {
	case 'v':
		return SETTINGS_VOID;
	case 'b':
		return SETTINGS_BOOL;
	case 'i':
		return SETTINGS_INT;
	case 'h':
		return SETTINGS_HOTKEY;
	}
	return SETTINGS_BOOL;
}

void
signal_hal_t::start_element(GMarkupParseContext *context,
			       const gchar         *element_name,
			       const gchar        **attribute_names,
			       const gchar        **attribute_values,
			       gpointer             user_data,
			       GError             **error)
{
	//print_debug("%s %s %d\n", __func__, element_name, parser_pos);
	switch (parser_pos) {
	case PARSER_OUT:
		if (strcmp(element_name, "signal") == 0) {
			const char *type;
			gboolean res = g_markup_collect_attributes(
				element_name, attribute_names,
				attribute_values, error,
				G_MARKUP_COLLECT_STRING, "type", &type,
				G_MARKUP_COLLECT_INVALID);
			if (res) {
				cur_plugin->settings_type() = str2stype(type);
			} else {
				print_debug("~read signal %s\n", (*error)->message);
			}
			parser_pos = PARSER_SIGNAL;
		} else {
			print_debug("expected <signal>, got %s\n", element_name);
			g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, "expected <signal>\n");
			return;
		}
		break;
	case PARSER_SIGNAL:
		if (!strcmp(element_name, "identification")) {
			parser_pos = PARSER_IDENTIFICATION;
		} else if (!strcmp(element_name, "signalization")) {
			const char *type, *name, *detail;
			gboolean res = g_markup_collect_attributes(
				element_name, attribute_names,
				attribute_values, error,
				G_MARKUP_COLLECT_STRING, "type", &type,
				G_MARKUP_COLLECT_STRING, "name", &name,
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "detail", &detail,
				G_MARKUP_COLLECT_INVALID);
			if (res) {
				if (!strcmp(type, "device_presence")) {
					cur_plugin->set_event_type(HAL_DEVICE_PRESENCE);
					cur_plugin->register_event(EVENT_HAL_DEVICE_PRESENCE);
				} else if (!strcmp(type, "device_capability")) {
					cur_plugin->set_event_type(HAL_DEVICE_CAPABILITY);
					cur_plugin->register_event(EVENT_HAL_DEVICE_CAPABILITY);
				} else if (!strcmp(type, "property_presence")) {
					cur_plugin->set_event_type(HAL_PROPERTY_PRESENCE);
					cur_plugin->register_event(EVENT_HAL_DEVICE_PROPERTY_MODIFIED);
				} else if (!strcmp(type, "property_value")) {
					cur_plugin->set_event_type(HAL_PROPERTY_VALUE);
					cur_plugin->register_event(EVENT_HAL_DEVICE_PROPERTY_MODIFIED);
				} else if (!strcmp(type, "device_condition")) {
					cur_plugin->set_event_type(HAL_DEVICE_CONDITION);
					cur_plugin->register_event(EVENT_HAL_DEVICE_CONDITION);
					cur_plugin->sig_condition_detail() = detail ? detail : "";
				}
				cur_plugin->sig_name() = name;
			}
		} else {
			print_debug("expected <>\n");
			g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, "expected <>\n");
			return;
		}
		break;
	case PARSER_IDENTIFICATION:
		if (!strcmp(element_name, "property")) {
			const char *key, *value, *type;
			gboolean res = g_markup_collect_attributes(
				element_name, attribute_names,
				attribute_values, error,
				G_MARKUP_COLLECT_STRING, "key", &key,
				G_MARKUP_COLLECT_STRING, "value", &value,
				G_MARKUP_COLLECT_STRING, "type", &type,
				G_MARKUP_COLLECT_INVALID);
			if (res) {
				variant_t v(type);
				v.from_str(value);
				cur_plugin->identification()[key] = value;
			}
		}
		break;
	}
}

void
signal_hal_t::end_element(GMarkupParseContext *context,
			     const gchar         *element_name,
			     gpointer             user_data,
			     GError             **error)
{
	switch (parser_pos) {
	case PARSER_SIGNAL:
		if (!strcmp(element_name, "signal")) {
			parser_pos = PARSER_OUT;
		}
		break;
	case PARSER_IDENTIFICATION:
		if (!strcmp(element_name, "identification"))
			parser_pos = PARSER_SIGNAL;
		break;
	default:
		break;
	}
}

void
signal_hal_t::load_from_xml()
{
	static GMarkupParser parser = {
		start_element,
		end_element,
		NULL
	};

	GMarkupParseContext *ctx;
	GError *error = NULL;
	char *contents;
	gsize length;

	parser_pos = PARSER_OUT;
	g_file_get_contents(m_filename.c_str(), &contents, &length, &error);
	if (error != NULL) {
		print_debug("Unable to load profile: %s\n", error->message);
		g_error_free(error);
		return;
	}
	ctx = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, this, NULL);
	if (!g_markup_parse_context_parse(ctx, contents,
					  length, &error)) {
		print_debug("%s\n", error->message);
	}
}

void
signal_hal_t::load_signal_hal(const char *full_name, const struct stat *fs)
{
	const char *short_name = strrchr(full_name, '/') + 1;
	if (!ends_with(short_name, ".xml"))
		return;
	if (S_ISREG(fs->st_mode) && access(full_name, R_OK) == 0) {
		cur_plugin = new signal_hal_t(string(short_name, strlen(short_name) - 4));
		cur_plugin->m_filename = full_name;
		cur_plugin->load_from_xml();
	}
}

void
signal_hal_t::init()
{
	foreach_file(PLUGINHALDIR, load_signal_hal);
}

