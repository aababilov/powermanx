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
#ifndef PMX_UTILS_HPP
#define PMX_UTILS_HPP

#include <string>
#include <map>
#include <set>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <libupower-glib/upower.h>


// foreach
#define FOREACH(type, i, cont)					\
	for (type::iterator i = cont.begin(), __e = cont.end();	\
	     i != __e; ++i)
#define FOREACH_CONST(type, i, cont)					\
	for (type::const_iterator i = cont.begin(), __e = cont.end();	\
	     i != __e; ++i)

//properties
#define PROPERTY_GETTER(type, name) type name() const { return m_##name; }
#define PROPERTY_SETTER(type, name) type &name() { return m_##name; }

#define DEFINE_PROPERTY_R_SIMPLE(type, name)	\
	public: PROPERTY_GETTER(type, name)	\
	private: type m_##name
#define DEFINE_PROPERTY_RW_SIMPLE(type, name)	\
	public: PROPERTY_GETTER(type, name)	\
	PROPERTY_SETTER(type, name)		\
	private: type m_##name

#define DEFINE_PROPERTY_R_COMPLEX(type, name)	\
	public: PROPERTY_GETTER(const type&, name)	\
	private: type m_##name
#define DEFINE_PROPERTY_RW_COMPLEX(type, name)		\
	public: PROPERTY_GETTER(const type&, name)	\
	PROPERTY_SETTER(type, name)			\
	private: type m_##name

#define POWERMAN_SERVICE "org.freedesktop.PowerManX"
#define POWERMAN_PATH "/org/freedesktop/PowerManX"
#define POWERMAN_INTERFACE "org.freedesktop.PowerManX"

void print_debug(const char *fmt, ...)
#ifdef __GNUC__
        __attribute__ ((format (printf, 1, 2)));
#endif
;

int foreach_file(const char *dir,
		 void (*f)(const char *full_name,
			   const struct stat *fs));

int ends_with(const char *s, const char *e);

std::string to_string(bool b);
std::string to_string(int i);

namespace std {
#define LESS_OF_CHAR(_Tp)						\
	template<>							\
	struct less<_Tp> :						\
		public binary_function<_Tp, _Tp, bool>			\
	{								\
		bool							\
		operator()(const char* __x, const char* __y) const	\
			{ return std::strcmp(__x, __y) < 0; }		\
	};
	LESS_OF_CHAR(char*)
	LESS_OF_CHAR(const char*)
#undef LESS_OF_CHAR
}

template<class T>
class named_object_t {
public:
	typedef std::map<const char *, T*> map_t;

	named_object_t(const std::string &name) {
		reg(name);
	}

	virtual ~named_object_t() {
		unreg();
	}

	void set_name(const std::string &name) {
		unreg();
		reg(name);
	}

	static T *find(const char *name) {
		typename map_t::iterator i = m_obj_map.find(name);
		return i != m_obj_map.end() ? i->second : NULL;
	}

	static T *find(const std::string &name) { return find(name.c_str()); }
	static map_t &obj_map() { return m_obj_map; }
	DEFINE_PROPERTY_R_COMPLEX(std::string, name);
private:
	static map_t m_obj_map;
	void reg(const std::string &name) {
		T *found;
		m_name = name;
		for (int i = 0; ; ++i) {
			found = find(m_name);
			if (!found)
				break;
			m_name = name + to_string(i);
			++i;
		}
		m_obj_map[m_name.c_str()] = static_cast<T*>(this);
	}
protected:
	void unreg() {
		m_obj_map.erase(m_name.c_str());
	}
};

#define DEFINE_OBJ_MAP(type) template<>		\
	named_object_t<type>::map_t		\
	named_object_t<type>::m_obj_map =	\
		named_object_t<type>::map_t()


#define glob_dbus_conn (dbus_provider_t::dbus_conn())
#define glob_up_client (dbus_provider_t::up_client())

class dbus_provider_t {
public:
	static DBusConnection *dbus_conn() { return m_dbus_conn; }
	static UpClient *up_client() { return m_up_client; }

	static void init();
	static void fini();
private:
	static DBusConnection *m_dbus_conn;
	static UpClient *m_up_client;
};


#endif
