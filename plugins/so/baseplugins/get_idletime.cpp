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
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <cassert>
#include <string>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <utmpx.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/input.h>

#include <glib.h>

#include <libpowermanx/utils.hpp>

using namespace std;

/*
  based on Condor source
 */

/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define dprintf print_debug
//#define NEED_DEBUG

/* define some static functions */
typedef struct {
	unsigned long num_key_intr;
	unsigned long num_mouse_intr;
	time_t timepoint;
} idle_t;

static time_t last_input_event;

static time_t utmp_pty_idle_time( time_t now );
static time_t all_pty_idle_time( time_t now );
static time_t dev_idle_time( const char *path, time_t now );
static time_t calc_idle_time();

static time_t km_idle_time(const time_t now);
static int get_keyboard_info(idle_t *fill_me);
static int get_mouse_info(idle_t *fill_me);
static int get_keyboard_mouse_info(idle_t *fill_me);
static int is_number(const char *str);

/* we must now use the kstat interface to get this information under later
	releases of Solaris */

/* the local function that does the main work */


/* delimiting characters between the numbers in the /proc/interrupts file */
#define DELIMS " "
#define BUFFER_SIZE (1024*10)

/* calc_idle_time fills in user_idle and console_idle with the number
 * of seconds since there has been activity detected on any tty or on
 * just the console, respectively.  therefore, console_idle will always
 * be equal to or greater than user_idle.  think about it.  also, note
 * that user_idle and console_idle are passed by reference.  Also,
 * on some platforms console_idle is always -1 because it cannot reliably
 * be determined.
 */
// Unix
time_t
calc_idle_time()
{
	time_t tty_idle, m_idle, m_console_idle;
	time_t now = time( 0 );
	/* what is the idle time using the /proc/interrupts method? */
	time_t m_interrupt_idle;
	const char *console_devices[] = {"console", "mouse", NULL};

	// Find idle time from ptys/ttys.  See if we should trust
	// utmp.  If so, only stat the devices that utmp says are
	// associated with active logins.  If not, stat /dev/tty* and
	// /dev/pty* to make sure we get it right.
	static int bad_utmp = 3;
	if (bad_utmp == 3)
		bad_utmp = getenv("BAD_UTMP") != NULL;
	if (bad_utmp)
		m_idle = all_pty_idle_time( now );
	else
		m_idle = utmp_pty_idle_time( now );
	print_debug("pty idle = %d\n", (int)m_idle);
	// Now, if CONSOLE_DEVICES is defined in the config file, stat
	// those devices and report console_idle.  If it's not there,
	// console_idle is -1.
	m_console_idle = -1;  // initialize
	{
		const char **tmp;
		for (tmp = console_devices; *tmp; ++tmp) {
			tty_idle = dev_idle_time( *tmp, now );
			m_idle = MIN( tty_idle, m_idle );
			if( m_console_idle == -1 ) {
				m_console_idle = tty_idle;
			} else {
				m_console_idle = MIN( tty_idle, m_console_idle );
			}
		}
	}
	print_debug("console idle = %d\n", (int)m_console_idle);
	/* If we still don't have console idle info, (e.g. atime is not updated
	   on device files in Linux 2.6 kernel), get keyboard and mouse idle
	   time via /proc/interrupts.  Update user_idle appropriately too.
	*/
	m_interrupt_idle = km_idle_time(now);
	print_debug("interrupt idle = %d", (int)m_interrupt_idle);
	/* If m_console_idle is still -1, MIN will always return -1
	   So we need to check whether m_console_idle is -1
	   - jaeyoung 2007/04/10
	*/
	if( m_console_idle != -1 ) {
		m_console_idle = MIN(m_interrupt_idle, m_console_idle);
	}else {
		m_console_idle = m_interrupt_idle;
	}

	if( m_console_idle != -1 ) {
		m_idle = MIN(m_console_idle, m_idle);
	}
	print_debug("/dev/input idle = %ld",
		    now - last_input_event);
	m_idle = MIN(m_idle, now - last_input_event);
	return m_idle;
}

// Unix
time_t
utmp_pty_idle_time( time_t now )
{
	time_t tty_idle;
	time_t answer = (time_t)INT_MAX;
	static time_t saved_now;
	static time_t saved_idle_answer = -1;
	struct utmpx *utmp_info;

	while((utmp_info = getutxent()) != NULL) {
		if (utmp_info->ut_type != USER_PROCESS)
			continue;
		tty_idle = dev_idle_time(utmp_info->ut_line, now);

		answer = MIN(tty_idle, answer);
	}
	endutxent();

	/* Here we check to see if we are about to return INT_MAX.  If so,
	 * we recompute via the last pty access we knew about.  -Todd, 2/97 */
	if ( (answer == INT_MAX) && ( saved_idle_answer != -1 ) ) {
		answer = (now - saved_now) + saved_idle_answer;
		if ( answer < 0 )
			answer = 0; /* someone messed with the system date */
	} else {
		if ( answer != INT_MAX ) {
			/* here we are returning an answer we discovered; save it */
			saved_idle_answer = answer;
			saved_now = now;
		}
	}

	return answer;
}

// Unix
time_t
all_pty_idle_time( time_t now )
{
	time_t	idle_time;
	time_t	answer = (time_t)INT_MAX;

	struct dirent *dent;
	DIR *dir;

	dir = opendir("/dev");
	if (dir) {
		while ((dent = readdir(dir)) != NULL) {
			if (strncmp("tty", dent->d_name, 3) == 0 ||
			    strncmp("pty", dent->d_name, 3) == 0) {
				idle_time = dev_idle_time(dent->d_name, now );
				if (idle_time < answer) {
					answer = idle_time;
				}
			}
		}
		closedir(dir);
	}
	// Now, if there's a /dev/pts, search all the devices in there.
	dir = opendir("/dev/pts");
	if (dir) {
		char pathname[100];
		while ((dent = readdir(dir)) != NULL) {
			if (dent->d_name[0] == '.')
				continue;
			sprintf(pathname, "pts/%s", dent->d_name);
			idle_time = dev_idle_time(pathname, now);
			if( idle_time < answer ) {
				answer = idle_time;
			}
		}
		closedir(dir);
	}

	return answer;
}

time_t
dev_idle_time( const char *path, time_t now )
{
	struct stat	buf;
	time_t answer;
	static char pathname[100] = "/dev/";
	static int null_major_device = -1;

	if ( !path || path[0]=='\0' ||
	     strncmp(path,"unix:",5) == 0 ) {
		// we don't have a valid path, or it is
		// a nonuseful/fake path setup by the X server
		return now;
	}
	strcpy( &pathname[5], path );
	if ( null_major_device == -1 ) {
		// get the major device number of /dev/null so
		// we can ignore any device that shares that
		// major device number (things like /dev/null,
		// /dev/kmem, etc).
		null_major_device = -2;	// so we don't try again
		if ( stat("/dev/null",&buf) < 0 ) {
			print_debug("Cannot stat /dev/null\n");
		} else {
			// we were able to stat /dev/null, stash dev num
			if ( !S_ISREG(buf.st_mode) && !S_ISDIR(buf.st_mode) &&
			     !S_ISLNK(buf.st_mode) ) {
				null_major_device = major(buf.st_rdev);
				print_debug("/dev/null major dev num is %d\n",
				       null_major_device);
			}
		}
	}

	/* ok, just check the device idle time for normal devices using stat() */
	if (stat(pathname,&buf) < 0) {
		if( errno != ENOENT ) {
			print_debug("Error on stat(%s,%p), errno = %d(%s)\n",
				pathname, &buf, errno, strerror(errno) );
		}
		buf.st_atime = 0;
	}

	/* XXX The signedness problem in this comparison is hard to fix properly */
	/*
	  The first argument is there in case buf is uninitialized here.
	  In this case, buf.st_atime would already be set to 0 above.
	*/
	if ( buf.st_atime != 0 && null_major_device > -1 &&
	     null_major_device == (int)major(buf.st_rdev) ) {
		// this device is related to /dev/null, it should not count
		buf.st_atime = 0;
	}

	answer = now - buf.st_atime;
	if (answer < 0) {
		/* device's atime is in future - cannot rely on it,
		   assuming infinite idle */
		answer = (time_t)INT_MAX;
	}

#ifdef NEED_DEBUG
	dprintf( "%s: %d secs; %s\n", pathname, (int)answer);
#endif

	return answer;
}

/* This block of code should only get compiled for linux machines which have
	/proc/interrupts */

/* Returns true if the string contains only digits (and is not empty) */
int
is_number(const char *str)
{
	int result = TRUE;
	int i;

	if (str == NULL)
	    return FALSE;

	for (i = 0; str[i] != '\0'; i++) {
		if (!isdigit(str[i])) {
			result = FALSE;
			break;
		}
	}

	return result;
}

/* Sets fill_me with info about keyboard idleness */
int
get_keyboard_info(idle_t *fill_me)
{
	FILE *intr_fs;
	int result = FALSE;
	char buf[BUFFER_SIZE], *tok, *tok_loc;

	/* Search /proc/interrupts for either:
	   1) the first occurrance of "i8042" or
	   2) "keyboard".
	   Generally, the keyboard will be IRQ 1.

	   The format of /proc/interrupts is:
	   [Header line]
	   [IRQ #]:  [# of interrupts at CPU 1] ... [CPU N] [dev type] [dev name]
	*/

	if ((intr_fs = fopen("/proc/interrupts", "r")) == NULL) {
		print_debug("Failed to open /proc/interrupts\n");
		return FALSE;
	}

	fgets(buf, BUFFER_SIZE, intr_fs);  /* Ignore header line */
	while (!result && (fgets(buf, BUFFER_SIZE, intr_fs) != NULL)) {
		if (strstr(buf, "i8042") != NULL || strstr(buf, "keyboard") != NULL){
#ifdef NEED_DEBUG
			print_debug("Keyboard IRQ: %d\n", atoi(buf) );
#endif
			tok = strtok_r(buf, DELIMS, &tok_loc);  /* Ignore [IRQ #]: */
			do {
				tok = strtok_r(NULL, DELIMS, &tok_loc);
				if (is_number(tok)) {
					/* It is ok if this overflows */
					fill_me->num_key_intr += strtoul(tok, NULL, 10);
#ifdef NEED_DEBUG
						dprintf(
							 "Add %lu keyboard interrupts.  Total: %lu\n",
							 strtoul(tok, NULL, 10), fill_me->num_key_intr );
#endif
				} else {
					break;  /* device type column */
				}
			} while (tok != NULL);
			result = TRUE;
		}
	}

	fclose(intr_fs);
	return result;
}

/* Sets fill_me with info about the mouse idleness */
int
get_mouse_info(idle_t *fill_me)
{
	FILE *intr_fs;
	int result = FALSE, first_i8042 = FALSE;
	char buf[BUFFER_SIZE], *tok, *tok_loc;

	/* Search /proc/interrupts for:
	   1) the second occurrance of "i8042", as we assume the first to be
	   the keyboard, or
	   2) "Mouse" or
	   3) "mouse"
	   Generally, the mouse will be IRQ 12.

	   The format of /proc/interrupts is:
	   [Header line]
	   [IRQ #]:  [# of interrupts at CPU 1] ... [CPU N] [dev type] [dev name]
	*/
	if ((intr_fs = fopen("/proc/interrupts", "r")) == NULL) {
		print_debug(
			"get_mouse_info(): Failed to open /proc/interrupts\n");
		return FALSE;
	}

	fgets(buf, BUFFER_SIZE, intr_fs);  /* Ignore header line */
	while (!result && (fgets(buf, BUFFER_SIZE, intr_fs) != NULL)) {
		if (strstr(buf, "i8042") && !first_i8042) {
			first_i8042 = TRUE;
		}
		else if ((strstr(buf, "i8042") != NULL && first_i8042) ||
			 strstr(buf, "Mouse") != NULL || strstr(buf, "mouse") != NULL)
		{

#ifdef NEED_DEBUG
			print_debug("Mouse IRQ: %d\n", atoi(buf));
#endif
			tok = strtok_r(buf, DELIMS, &tok_loc);  /* Ignore [IRQ #]: */
			do {
				tok = strtok_r(NULL, DELIMS, &tok_loc);
				if (is_number(tok)) {
					/* It is ok if this overflows */
					fill_me->num_mouse_intr += strtoul(tok, NULL, 10);

#ifdef NEED_DEBUG
					print_debug(
						"Add %lu mouse interrupts.  Total: %lu\n",
						strtoul(tok, NULL, 10), fill_me->num_mouse_intr);
#endif
				} else {
					break;  /* device type column */
				}
			} while (tok != NULL);
			result = TRUE;

		}
	}

	fclose(intr_fs);
	return result;
}

/* Returns true if info about the idleness of the keyboard or mouse (or both)
   has been obtained. */
int
get_keyboard_mouse_info(idle_t *fill_me)
{
    int r1, r2;

    r1 = get_keyboard_info(fill_me);
    r2 = get_mouse_info(fill_me);

    return r1 || r2;
}

/* Calculate # of seconds since there has been activity detected on
   the keyboard and/or mouse.

   In order to determine "activity since", we need to measure from a known
   point in time.  However, when this function is first called, we lack this.
   Thus, when first called this function assumes that the keyboard/mouse
   were active immediately prior, ie. returns 0 (seconds since last activity).
*/
time_t
km_idle_time(const time_t now)
{
	/* we want certain debugging messages to only happen rarely since otherwise
		they fill the logs with useless garbage. */
	static int timer_initialized = FALSE;
	static struct timeval msg_delay;
	static struct timeval msg_now;
	time_t msg_timeout = 60 * 60; /* 1 hour seems good */
	static int msg_emit_immediately = TRUE;

	/* initialize the message timer to force certain messages to print out
		much less frequently */
	if (timer_initialized == FALSE) {
		gettimeofday(&msg_delay, NULL);
		timer_initialized = TRUE;
	}
	gettimeofday(&msg_now, NULL);

	/* We need to store information about the state of the keyboard
	   and mouse the last time we saw activity on either of them.  Thus,
	   last_km_activity is a static variable that is initialized when
	   this function is first called.  And "once" is a switch that tells
	   us if last_km_activity has been initialized or not.
	*/
	static int once = FALSE;
	static idle_t last_km_activity;

	idle_t current = {0, 0, 0};

	/* Initialize, if necessary.  last_km_activity holds information about
	   the most recently detected activity on the keyboard or mouse. */
	if (once == FALSE) {
		last_km_activity.num_key_intr = 0;
		last_km_activity.num_mouse_intr = 0;
		last_km_activity.timepoint = now;

		if (!get_keyboard_mouse_info(&last_km_activity)) {

			/* emit the error on msg delay boundaries */
			if (msg_emit_immediately == TRUE ||
				(msg_now.tv_sec - msg_delay.tv_sec) > msg_timeout)
			{
				dprintf(
					"Unable to calculate keyboard/mouse idle time due to them "
					"both being USB or not present, assuming infinite "
					"idle time for these devices.\n");

				msg_delay = msg_now;
				msg_emit_immediately = FALSE;
			}

			/* Here we'll try to initialize it again hoping whatever
			   the problem was was transient and went away.

			   What do we return in this case?
			   Report infinite idle time 'a la' utmp_pty_idle_time */
			return (time_t)INT_MAX;
		}
		dprintf( "Initialized last_km_activity\n");
		once = TRUE;
	}

	/* Take current measurement */
	if (!get_keyboard_mouse_info(&current)) {
		if ((msg_now.tv_sec - msg_delay.tv_sec) > msg_timeout) {
			/* This is kind of a rare error, it would mean someone unplugged
				the keyboard and mouse after Condor has already recognized
				them. */
			dprintf(
				"Condor had been able to determine keybaord and idle times, "
				"but something has changed about the hardware and Condor is now"
				"unable to calculate keyboard/mouse idle time due to them "
				"both being USB or not present, assuming infinite "
				"idle time for these devices.\n");

			msg_delay = msg_now;
		}
		/* Report latest idle time we know about */
		return now - last_km_activity.timepoint;
	}

	/* Activity is revealed by higher interrupt sums. In the case of
	   overflow of an interrupt counter on a single CPU or their sum, I
	   really only care that at least one interrupt occurred.  Thus, if the
	   new sum and old sum are different in any way, activity occurred. */
	if ((current.num_key_intr != last_km_activity.num_key_intr) ||
	    (current.num_mouse_intr != last_km_activity.num_mouse_intr)) {

		/*  Save info about most recent activity */
		last_km_activity.num_key_intr = current.num_key_intr;
		last_km_activity.num_mouse_intr = current.num_mouse_intr;
		last_km_activity.timepoint = now;
	}

	return now - last_km_activity.timepoint;
}

typedef struct _InputData InputData;
struct _InputData
{
	struct input_event event;
	gsize offset;
	gboolean button_has_state;
	gboolean button_state;
};

static void
destroy_data(InputData *data)
{
	g_free(data);
}

gboolean
event_callback(GIOChannel *channel, GIOCondition condition, gpointer data)
{
	InputData  *input_data = (InputData*)data;
	gsize read_bytes;

	if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
		return FALSE;
	}

	/** tbh, we can probably assume every time we read we have a whole
	 * event availiable, but hey..*/
	while (g_io_channel_read_chars (
		       channel,
		       ((gchar*)&input_data->event) + input_data->offset,
		       sizeof(struct input_event) - input_data->offset,
		       &read_bytes, NULL) == G_IO_STATUS_NORMAL) {
		if (input_data->offset + read_bytes < sizeof (struct input_event)) {
			input_data->offset = input_data->offset + read_bytes;
			return TRUE;
		} else {
			input_data->offset = 0;
		}
	}

	last_input_event = time(NULL);
	return TRUE;
}

GIOChannel* add_listener(const char *udi)
{
	GIOChannel *chan;
        InputData *data;
        const char* device_file;

	if ((device_file = libhal_device_get_property_string(
				glob_hal_ctx,
				udi,
				"input.device", NULL)) == NULL) {
		return NULL;
        }

	chan = g_io_channel_new_file(device_file, "r", NULL);
	if (!chan) {
		print_debug("not opened %s\n", device_file);
		return NULL;
	}
	print_debug("opened %s\n", device_file);

	data = (InputData*)g_malloc(sizeof (InputData));
	memset(data, 0, sizeof (InputData));
	g_io_add_watch_full(
		chan,
		G_PRIORITY_DEFAULT,
		(GIOCondition)(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
		event_callback, data,
		(GDestroyNotify)destroy_data);
	return chan;
}


class input_dev_notifier_t : public hal_listener_t {
public:
	input_dev_notifier_t();
	~input_dev_notifier_t() {}

	virtual void on_hal_device_presence(
		const char *udi,
		bool present);
	virtual void on_hal_device_capability(
		const char *udi,
		const char *capability,
		bool present);
	virtual void on_hal_device_property_modified(
		const char *udi,
		const char *key,
		dbus_bool_t is_removed,
		dbus_bool_t is_added);
	virtual void on_hal_device_condition(
		const char *udi,
		const char *condition_name,
		const char *condition_detail);

	void add(const char *udi);
	void remove(const char *udi);

	map<string, void *> devs;
	string capability;
};

input_dev_notifier_t::input_dev_notifier_t()
{
	register_event_list(
		(1 << EVENT_HAL_DEVICE_PRESENCE));
	capability = "input";

	int num_devs;
	char **devs_list = libhal_find_device_by_capability(
		glob_hal_ctx,
		"input",
		&num_devs,
		NULL);
	if (devs_list) {
		for (int i = 0; i < num_devs; ++i)
			add(devs_list[i]);
		libhal_free_string_array(devs_list);
	}
}

void
input_dev_notifier_t::add(const char *udi)
{
	GIOChannel *chan = add_listener(udi);
	if (chan)
		devs[udi] = chan;
}

void
input_dev_notifier_t::remove(const char *udi)
{
	map<string, void*>::iterator i = devs.find(udi);
	if (i != devs.end()) {
		GIOChannel *channel = (GIOChannel *)i->second;
		devs.erase(i);
		g_io_channel_shutdown(channel, FALSE, NULL);
                g_io_channel_unref (channel);

	}
}

void
input_dev_notifier_t::on_hal_device_presence(
	const char *udi,
	bool present)
{
	if (present) {
		DBusError error;
		dbus_error_init(&error);
		bool is_dev = libhal_device_query_capability(
			    glob_hal_ctx,
			    udi,
			    capability.c_str(),
			    &error);
		if (dbus_error_is_set(&error)) {
			dbus_error_free(&error);
		} else if (is_dev) {
			add(udi);
		}
	} else {
		remove(udi);
	}
}

void
input_dev_notifier_t::on_hal_device_capability(
		const char *udi,
		const char *capability,
		bool present)
{
}
void
input_dev_notifier_t::on_hal_device_property_modified(
		const char *udi,
		const char *key,
		dbus_bool_t is_removed,
		dbus_bool_t is_added)
{
}
void
input_dev_notifier_t::on_hal_device_condition(
		const char *udi,
		const char *condition_name,
		const char *condition_detail)
{
}

#if 0

int get_xidletime()
{
	XScreenSaverInfo ssi;
	int event_basep, error_basep;
	int ret;

	Display *dpy = find_x_display();
	if (dpy == NULL) {
		print_debug("couldn't open display");
		return -1;
	}

	if (!XScreenSaverQueryExtension(dpy, &event_basep, &error_basep)) {
		print_debug("screen saver extension not supported");
		ret = -1;
		goto close_disp;
	}

	if (!XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), &ssi)) {
		print_debug("couldn't query screen saver info");
		ret = -1;
		goto close_disp;
	}
	print_debug("ssi.idle=%lu", ssi.idle);
	ret = workaroundCreepyXServer(dpy, ssi.idle) / 1000;

close_disp:
	XCloseDisplay(dpy);
	return ret;
}

/*!
 * This function works around an XServer idleTime bug in the
 * XScreenSaverExtension if dpms is running. In this case the current
 * dpms-state time is always subtracted from the current idletime.
 * This means: XScreenSaverInfo->idle is not the time since the last
 * user activity, as descriped in the header file of the extension.
 * This result in SUSE bug # and sf.net bug #. The bug in the XServer itself
 * is reported at https://bugs.freedesktop.org/buglist.cgi?quicksearch=6439.
 *
 * Workaround: Check if if XServer is in a dpms state, check the
 *             current timeout for this state and add this value to
 *             the current idle time and return.
 *
 * \param _idleTime a unsigned long value with the current idletime from
 *                  XScreenSaverInfo->idle
 * \return a unsigned long with the corrected idletime
 */
unsigned long workaroundCreepyXServer(Display *dpy, unsigned long _idleTime) {
	int dummy;
	CARD16 standby, suspend, off;
	CARD16 state;
	BOOL onoff;

	if (DPMSQueryExtension(dpy, &dummy, &dummy) && DPMSCapable(dpy)) {
		DPMSGetTimeouts(dpy, &standby, &suspend, &off);
		DPMSInfo(dpy, &state, &onoff);
		if (!onoff)
			goto end;

		switch (state) {
		case DPMSModeStandby:
			/* this check is a littlebit paranoid, but be sure */
			if (_idleTime < (unsigned) (standby * 1000))
				_idleTime += (standby * 1000);
			break;
		case DPMSModeSuspend:
			if (_idleTime < (unsigned) ((suspend + standby) * 1000))
				_idleTime += ((suspend + standby) * 1000);
			break;
		case DPMSModeOff:
			if (_idleTime < (unsigned) ((off + suspend + standby) * 1000))
				_idleTime += ((off + suspend + standby) * 1000);
			break;
		case DPMSModeOn:
		default:
			break;
		}

	}
end:
	return _idleTime;
}
#endif

int get_idletime()
{
	return calc_idle_time();
}

/*
  km_idle_time is a cool function, but it considers that user
  is active only in moment when the function is called
  So, if we have these events:
  00 sec - km_idle_time (1st call)
  03 sec - user activity
  30 sec - km_idle_time (2nd call)
  km_idle_time will return 0 secs, not 27
  The following function is called each 4 seconds,
  so km_idle_time will be more precise
 */
static gboolean km_timer_callback(gpointer data)
{
	km_idle_time(time(NULL));
	return TRUE;
}

void init_get_idletime()
{
	static bool inited = false;
	if (inited)
		return;
	inited = true;

	g_timeout_add_seconds(4, km_timer_callback, NULL);
	new input_dev_notifier_t();
}
