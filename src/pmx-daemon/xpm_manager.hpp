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
#include <glib.h>
#include <glib-object.h>

#define XPM_TYPE_MANAGER                 (xpm_manager_get_type())
#define XPM_MANAGER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), XPM_TYPE_MANAGER, XpmManager))
#define XPM_MANAGER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), XPM_TYPE_MANAGER, XpmManagerClass))
#define XPM_MANAGER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), XPM_TYPE_MANAGER, XpmManagerClass))


typedef struct _XpmManager XpmManager;
typedef struct _XpmManagerClass XpmManagerClass;
//const char *name;

struct _XpmManager {
	GObject parent;
//	int v;
	/* instance members */
};

struct _XpmManagerClass {
	GObjectClass parent;
	/* class members */
};


GType xpm_manager_get_type (void);

void xpm_manager_install_info(void);
