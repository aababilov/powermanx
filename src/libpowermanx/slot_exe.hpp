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
#ifndef PMX_SLOT_EXE_HPP
#define PMX_SLOT_EXE_HPP

#include <libpowermanx/signal_slot.hpp>

class slot_exe_t: public slot_plugin_t {
public:
	slot_exe_t(const std::string &name, const std::string &file_name);
	virtual void activate(const char *param);

	static bool init();
private:
	std::string m_file_name;
	static int ask(const char *fname, std::string &result, const char *arg);
	int ask(std::string &result, const char *arg) {
		return ask(m_file_name.c_str(), result, arg);
	}
	int spawn(const char *arg0, const char *arg1);
	int ret_code;

	static void load_plugin(const char *full_name, const struct stat *fs);
	static void sigchld_handler(int num);
	static int set_nonblock_flag(int desc, int value);
};

#endif
