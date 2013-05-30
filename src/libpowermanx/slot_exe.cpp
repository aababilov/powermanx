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
#include "slot_exe.hpp"

#include <cstdio>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

using std::string;

void slot_exe_t::load_plugin(const char *full_name, const struct stat *fs)
{
	const char *short_name = strrchr(full_name, '/') + 1;
	if (S_ISREG(fs->st_mode) && access(full_name, X_OK) == 0) {
		new slot_exe_t(short_name, full_name);
	}
}

bool slot_exe_t::init()
{
	signal(SIGCHLD, sigchld_handler);
	foreach_file(PLUGINEXECDIR, load_plugin);
	return true;
}

int slot_exe_t::set_nonblock_flag(int desc, int value)
{
	int oldflags = fcntl(desc, F_GETFL, 0);
	/* If reading the flags failed, return error indication now. */
	if (oldflags == -1)
		return -1;
	/* Set just the flag we want to set. */
	if (value != 0)
		oldflags |= O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;
	/* Store modified flag word in the descriptor. */
	return fcntl(desc, F_SETFL, oldflags);
}

void slot_exe_t::sigchld_handler(int num)
{
	sigset_t set, oldset;
	pid_t pid;
	int status, exitstatus;

	/* block other incoming SIGCHLD signals */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, &oldset);

	/* wait for child */
	while((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0)	{
		if(WIFEXITED(status)) {
			exitstatus = WEXITSTATUS(status);
/*			fprintf(stderr, "Parent: child exited, pid = %d, exit status = %d\n",
			(int)pid, exitstatus);
*/
		} else if(WIFSIGNALED(status)) {
			exitstatus = WTERMSIG(status);

			/*fprintf(stderr,
			  "Parent: child terminated by signal %d, pid = %d\n",
			  exitstatus, (int)pid);
			*/
		} else if(WIFSTOPPED(status)) {
			exitstatus = WSTOPSIG(status);

			/*fprintf(stderr,
			  "Parent: child stopped by signal %d, pid = %d\n",
			  exitstatus, (int)pid);
			*/
		} else {
			/*fprintf(stderr,
			  "Parent: child exited magically, pid = %d\n",
			  (int)pid);
			*/
		}
	}

	/* re-install the signal handler (some systems need this) */
	signal(SIGCHLD, sigchld_handler);

	/* and unblock it */
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &set, &oldset);
}

slot_exe_t::slot_exe_t(const std::string &name,
		       const std::string &file_name)
	: slot_plugin_t(name)
{
	m_file_name = file_name;
	string ans;
	ask(ans, "has_param");
	set_has_param(strstr(ans.c_str(), "yes") != NULL);
	if (has_param()) {
		string avail;
		ask(avail, "avail_params");
		char *s = &avail[0], *token;
		while ((token = strtok(s, "\n"))) {
			m_avail_params.push_back(token);
			s = NULL;
		}
		ask(avail, "any_param");
		set_any_param(strstr(avail.c_str(), "yes") != NULL);
	}
}

int slot_exe_t::ask(const char *fname, std::string &result, const char *arg)
{
	pid_t pid;
	int mypipe[2];
	result.clear();

	if (pipe(mypipe)) {
		fprintf(stderr, "Pipe failed.\n");
		return EXIT_FAILURE;
	}

	pid = fork();
	if (pid == (pid_t)0) {
		close(mypipe[0]);
		dup2(mypipe[1], STDOUT_FILENO);
		execl(fname, fname, arg, NULL);
		return -1;
	}
	int file = mypipe[0], rret;
	close(mypipe[1]);
	static char buf[1024];
	set_nonblock_flag(file, 1);
	while ((rret = read(file, buf, sizeof(buf))) != 0) {
		if (rret == -1) {
			if (errno == EAGAIN)
				continue;
			else
				return -1;
		}
		result += string(buf, rret);
	}
	return 0;
}

int slot_exe_t::spawn(const char *arg0, const char *arg1)
{
	pid_t pid = fork();
	if (pid == (pid_t)0) {
		execl(m_file_name.c_str(), m_file_name.c_str(),
		      arg0, arg1, NULL);
		_exit(1);
	}
	ret_code = 0;
	return 0;
}

void slot_exe_t::activate(const char *param)
{
	if (has_param()) {
		spawn("set", param);
	} else
		spawn(NULL, NULL);
}
