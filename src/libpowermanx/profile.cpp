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
#include "profile.hpp"

#include <cstdarg>
#include <cstdio>

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <glib.h>

using std::string;
using std::list;

DEFINE_OBJ_MAP(profile_t);

profile_t *profile_t::m_active_profile = NULL,
	*profile_t::profile2activate = NULL;
bool profile_t::in_activation = false;
profile_t::parser_pos_t profile_t::parser_pos;
signal_record_t *profile_t::cur_signal;
int profile_t::cp_source_tag = -1;

profile_t::profile_t(const std::string &name)
	: named_object_t<profile_t>(name)
{
	m_enabled = false;
	m_edited = false;
}

profile_t::~profile_t()
{
	if (m_active_profile == this)
		m_active_profile = NULL;
	unreg();
	deactivate();
	free_signals();
}

bool profile_t::is_default() const
{
	return !strcmp(name().c_str(), "default");
}

std::string profile_t::to_xml() const
{
	string s = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<profile>\n";
	FOREACH_CONST(signals_t, i, m_signals) {
		s += (*i)->to_xml();
	}
	s += "</profile>\n";
	return s;
}

void profile_t::start_element(GMarkupParseContext *context,
			      const gchar         *element_name,
			      const gchar        **attribute_names,
			      const gchar        **attribute_values,
			      gpointer             user_data,
			      GError             **error)
{
	switch (parser_pos) {
	case PARSER_OUT:
		if (strcmp(element_name, "profile") == 0) {
			parser_pos = PARSER_PROFILE;
		} else {
			print_debug("expected <profile>, got %s", element_name);
			g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, "expected <profile>\n");
			return;
		}
		break;
	case PARSER_PROFILE:
		if (!strcmp(element_name, "signal")) {
			const char *name, *timeout;
			gboolean enabled;
			gboolean res = g_markup_collect_attributes(
				element_name, attribute_names,
				attribute_values, error,
				G_MARKUP_COLLECT_STRING, "name", &name,
				G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "enabled", &enabled,
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "timeout", &timeout,
				G_MARKUP_COLLECT_INVALID);
			cur_signal = NULL;
			if (res) {
				signal_plugin_t *sig = signal_plugin_t::find(name);
				if (sig) {
					cur_signal = sig->create_record();
					cur_signal->timeout() = timeout ? atoi(timeout) : 0;
					cur_signal->enabled() = enabled;
				} else {
					print_debug("cannot find signal %s", name);
				}
			} else {
				print_debug("~ signal %s %s", name, (*error)->message);
			}
			parser_pos = PARSER_SIGNAL;
		} else {
			print_debug("expected <signal>");
			g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, "expected <signal>\n");
			return;
		}
		break;
	case PARSER_SIGNAL:
		if (!strcmp(element_name, "condition_list")) {
			parser_pos = PARSER_CONDITION_LIST;
		} else if (!strcmp(element_name, "slot_list")) {
			parser_pos = PARSER_SLOT_LIST;
		}
		break;
	case PARSER_CONDITION_LIST:
		if (!cur_signal)
			break;
		if (!strcmp(element_name, "condition")) {
			const char *key, *value;
			gboolean res = g_markup_collect_attributes(
				element_name, attribute_names,
				attribute_values, error,
				G_MARKUP_COLLECT_STRING, "key", &key,
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "value", &value,
				G_MARKUP_COLLECT_INVALID);
			if (res) {
				cur_signal->set_condition(key, value);
			} else {
				print_debug("~ <condition>");
			}
		} else {
			print_debug("expected <condition>");
		}
		break;
	case PARSER_SLOT_LIST:
		if (!cur_signal)
			break;
		if (!strcmp(element_name, "slot")) {
			const char *name, *param;
			gboolean enabled;
			gboolean res = g_markup_collect_attributes(
				element_name, attribute_names,
				attribute_values, error,
				G_MARKUP_COLLECT_STRING, "name", &name,
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "enabled", &enabled,
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "param", &param,
				G_MARKUP_COLLECT_INVALID);
			if (res) {
				slot_plugin_t *slot = slot_plugin_t::find(name);
				if (slot) {
					slot_record_t *rec = slot->create_record();
					rec->enabled() = enabled;
					rec->set_param(param);
					cur_signal->slots().push_back(rec);
				} else {
					print_debug("cannot find slot %s", name);
				}
			} else {
				print_debug("~ slot %s %s", name, (*error)->message);
			}
		} else {
			print_debug("expected <slot>");
		}
		break;
	}
}

void profile_t::end_element(GMarkupParseContext *context,
			    const gchar         *element_name,
			    gpointer             user_data,
			    GError             **error)
{
	profile_t *p = static_cast<profile_t *>(user_data);
	//print_debug("%s %s", __func__, element_name);
	switch (parser_pos) {
	case PARSER_OUT:
		break;
	case PARSER_PROFILE:
		if (!strcmp(element_name, "profile")) {
			parser_pos = PARSER_OUT;
		}
		break;
	case PARSER_SIGNAL:
		if (!strcmp(element_name, "signal")) {
			parser_pos = PARSER_PROFILE;
			if (cur_signal) {
				p->m_signals.push_back(cur_signal);
				cur_signal = NULL;
			}
		}
		break;
	case PARSER_CONDITION_LIST:
		if (!strcmp(element_name, "condition_list")) {
			parser_pos = PARSER_SIGNAL;
		}
		break;
	case PARSER_SLOT_LIST:
		if (!strcmp(element_name, "slot_list")) {
			parser_pos = PARSER_SIGNAL;
		}
		break;
	}
}

void profile_t::free_signals()
{
	FOREACH(signals_t, i, m_signals)
		delete *i;
	m_signals.clear();
}

void profile_t::load_from_xml(const char *xml)
{
	static GMarkupParser parser = {
		start_element,
		end_element,
		NULL
	};
	GMarkupParseContext *ctx;
	GError *error = NULL;

	free_signals();
	parser_pos = PARSER_OUT;
	ctx = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, this, NULL);
	if (!g_markup_parse_context_parse(ctx, xml, -1, &error)) {
		print_debug("%s", error->message);
	}
	g_free(ctx);
}

void profile_t::create_profile(const char *full_name, const struct stat *fs)
{
	const char *short_name = strrchr(full_name, '/') + 1;
	if (!ends_with(short_name, ".xml"))
		return;
	if (S_ISREG(fs->st_mode) && access(full_name, R_OK) == 0) {
		GError *error = NULL;
		char *contents;

		g_file_get_contents(full_name, &contents, NULL, &error);
		if (error != NULL) {
			print_debug("Unable to load profile: %s", error->message);
			g_error_free(error);
			return;
		}

		profile_t *profile = new profile_t(string(short_name, strlen(short_name) - 4));
		profile->load_from_xml(contents);
		g_free(contents);
	}
}

void profile_t::load()
{
	foreach_file(PROFILEDIR, create_profile);
}

void profile_t::activate_default()
{
	profile_t *p = find("default");
	if (!p) {
		print_debug("no default profile!!!");
		exit(0);
	}
	p->activate();
}

void profile_t::activate()
{
	print_debug("activated profile %s", name().c_str());
	in_activation = true;
	FOREACH(signals_t, i, m_signals)
		(*i)->activate();
	in_activation = false;
}

void profile_t::deactivate()
{
	print_debug("deactivated profile %s", name().c_str());
	in_activation = true;
	FOREACH(signals_t, i, m_signals)
		(*i)->deactivate();
	in_activation = false;
}

void profile_t::change_profile(const std::string &name)
{
	profile_t *p = find(name);
	if (!p) {
		print_debug("no profile `%s\'", name.c_str());
		return;
	}
	change_profile(p);
}

gboolean profile_t::change_profile_callback(gpointer data)
{
	cp_source_tag = -1;
	change_profile(profile2activate);
        return FALSE;
}

void profile_t::change_profile(profile_t *p)
{
	if (in_activation) {
		profile2activate = p;
		if (cp_source_tag < 0) {
			cp_source_tag = g_timeout_add_seconds
				(1, change_profile_callback, NULL);
		}
		return;
	}
	if (p == m_active_profile)
		return;

	profile2activate = NULL;
	if (m_active_profile)
		m_active_profile->deactivate();
	if (!strcmp(p->name().c_str(), "default")) {
		m_active_profile = NULL;
		return;
	}
	m_active_profile = p;
	p->activate();
}

void profile_t::reload(const char *name, const char *xml)
{
	string filename = string(PROFILEDIR "/") + name + ".xml";
	if (!xml[0]) {
		remove(filename.c_str());
	} else {
		GError *error = NULL;
		if (!g_file_set_contents(filename.c_str(), xml, -1, &error))
			print_debug("%s", error->message);
	}
	profile_t *profile = find(name);
	if (profile) {
		if (!xml[0]) {
			delete profile;
		} else {
			profile->load_from_xml(xml);
			if (!strcmp(name, "default") ||
			    profile == m_active_profile) {
				profile->activate();
			}
		}
	} else {
		profile = new profile_t(name);
		print_debug("created profile %s", name);
		profile->load_from_xml(xml);
	}
}
