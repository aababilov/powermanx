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
#ifndef PROFILE_HPP
#define PROFILE_HPP

#include <string>
#include <list>

#include <libpowermanx/signal_slot.hpp>

class profile_t : public named_object_t<profile_t> {
public:
	typedef std::list<signal_record_t *> signals_t;

	DEFINE_PROPERTY_RW_SIMPLE(bool, edited);
	DEFINE_PROPERTY_RW_SIMPLE(bool, enabled);
	DEFINE_PROPERTY_RW_COMPLEX(signals_t, signals);
public:
	profile_t(const std::string &name);
	~profile_t();

	std::string to_xml() const;
	void load_from_xml(const char *xml);
	void free_signals();

	bool is_default() const;

	static void change_profile(const std::string &name);
	static void change_profile(profile_t *p);
	static profile_t* active_profile() { return m_active_profile; }
	static void activate_default();
	static void load();
	static void reload(const char *name, const char *xml);
protected:
	void activate();
	void deactivate();

	static gboolean change_profile_callback(gpointer data);
	static void create_profile(const char *full_name, const struct stat *fs);
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

	enum parser_pos_t {
		PARSER_OUT, PARSER_PROFILE, PARSER_SIGNAL,
		PARSER_CONDITION_LIST, PARSER_SLOT_LIST,};

	static profile_t *m_active_profile;
	static profile_t *profile2activate;
	static bool in_activation;
	static parser_pos_t parser_pos;
	static signal_record_t *cur_signal;
	static int cp_source_tag;
};

#endif /*!defined SIGNAL_PLUGIN_HPP*/
