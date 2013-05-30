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
#ifndef VARIANT_HPP
#define VARIANT_HPP

#include <string>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <dbus/dbus.h>

class variant_t {
	union {
		dbus_int32_t int_val;
		dbus_uint64_t uint64_val;
		bool bool_val;
		double double_val;
	};
	std::string string_val;
	int which_t;
public:
	variant_t() { which_t = DBUS_TYPE_STRING; }
	variant_t(int t) { which_t = t; }
	variant_t(const char *s) { which_t = str2type(s); }

	variant_t &operator=(dbus_int32_t i) { which_t = DBUS_TYPE_INT32; int_val = i; return *this; }
	variant_t &operator=(dbus_uint64_t i) { which_t = DBUS_TYPE_UINT64; int_val = i; return *this; }
	variant_t &operator=(bool i) { which_t = DBUS_TYPE_BOOLEAN; bool_val = i; return *this; }
	variant_t &operator=(double i) { which_t = DBUS_TYPE_DOUBLE; double_val = i; return *this; }
	variant_t &operator=(const std::string &i) { which_t = DBUS_TYPE_STRING; string_val = i; return *this; }
	variant_t &operator=(const char *i) { which_t = DBUS_TYPE_STRING; string_val = i ? i : ""; return *this; }

	bool operator==(dbus_int32_t i) const { return which_t == DBUS_TYPE_INT32 && int_val == i; }
	bool operator==(dbus_uint64_t i) const { return which_t == DBUS_TYPE_UINT64 && uint64_val == i; }
	bool operator==(bool i) const { return which_t == DBUS_TYPE_BOOLEAN && bool_val == i; }
	bool operator==(double i) const { return which_t == DBUS_TYPE_DOUBLE && double_val == i; }
	bool operator==(const std::string &i) const { return which_t == DBUS_TYPE_STRING && string_val == i; }
	bool operator==(const char *i) const { return which_t == DBUS_TYPE_STRING && std::strcmp(string_val.c_str(), i) == 0; }
	bool operator==(char *i) const { return which_t == DBUS_TYPE_STRING && std::strcmp(string_val.c_str(), i) == 0; }

	operator int&() { return int_val; }
	operator bool&() { return bool_val; }
	operator double&() { return double_val; }
	operator std::string&() { return string_val; }

	operator int() const { return int_val; }
	operator bool() const { return bool_val; }
	operator double() const { return double_val; }
	operator const std::string&() const { return string_val; }

	int &which_type() { return which_t; }
	int which_type() const { return which_t; }

	void set_type(const char *s) { which_t = str2type(s); }
	void from_str(const char *s) {
		switch (which_t) {
		case DBUS_TYPE_BOOLEAN:
			if (s) {
				bool_val = s[0] == '1' || s[0] == 'y' ||
					s[0] == 't';
			} else {
				bool_val = 0;
			}
			break;
		case DBUS_TYPE_INT32:
			if (s)
				int_val = std::atoi(s);
			else
				int_val = 0;
			break;
		case DBUS_TYPE_DOUBLE:
			if (s)
				double_val = std::atof(s);
			else
				double_val = 0;
			break;
		case DBUS_TYPE_STRING:
			string_val = s ? s : "";
			break;
		}
	}

	static int str2type(const char *name) {
		if (name == NULL)
			return DBUS_TYPE_STRING;
		char c = std::tolower(name[0]);
		if (std::strchr("ibds", c))
			return c;
		return DBUS_TYPE_STRING;
	}
};

#endif /*!defined VARIANT_HPP*/
