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

#include <set>

#include <gtk/gtk.h>

#include <libpowermanx/libpowermanx.hpp>

#include "profile_edit.hpp"
#include "status_icon.hpp"
#include "app_utils.hpp"


using namespace std;

#define bool_signal (static_cast<signal_record_bool_t*>(curr_signal))
#define int_signal (static_cast<signal_record_int_t*>(curr_signal))
#define hotkey_signal (static_cast<signal_record_hotkey_t*>(curr_signal))

pmx_profile_edit_t *pr_edit;

/* constants */
enum {COLUMN_PTR, COLUMN_TITLE, COLUMN_ENABLED, COLUMN_PARAMETER,};

/* variables */
set<string> mod_profiles;

GtkWidget *profile_edit_win;
GtkWidget *view_profile, *view_signal, *view_slot, *lbl;
GtkListStore *store_profile, *store_signal,
	*store_slot, *store_slot_param,
	*store_hotkey;
GtkCellRendererCombo *ren_slot_param;
GtkWidget *btn_profile_add, *btn_profile_remove,
	*btn_signal_add, *btn_signal_remove,
	*btn_slot_add, *btn_slot_remove,
	*btn_edit_ok, *btn_edit_apply, *btn_edit_close;
GtkWidget *box_int, *box_bool, *box_signal_sets,
	*box_timeout, *box_hotkey,
	*box_signal_ar_btns, *box_slot_ar_btns;
GtkWidget *rb_bool_changed, *rb_bool_value, *val_bool_value,
	*val_timeout,
	*rb_int_changed, *rb_int_value, *val_int_value,
	*rb_int_interval, *chb_int_from, *chb_int_to,
	*val_int_from, *val_int_to,
	*val_hotkey;

GtkDialog *dlg_add_record;
GtkWidget *cbx_plugin_name;
GtkListStore *store_plugin_name;

GtkDialog *dlg_add_profile;
GtkWidget *new_profile_name;

slot_record_t *curr_slot;
signal_record_t *curr_signal;
profile_t *curr_profile;

GtkTreeIter curr_signal_iter;
bool curr_signal_iter_valid = false;

/* functions */
void store_curr_signal();
void menu_append_params(GtkWidget *param_menu, slot_plugin_t *plugin);

void set_edited()
{
	curr_profile->edited() = true;
}

void gui_set_signal(GtkTreeIter iter, signal_record_t *signal)
{
	gtk_list_store_set(store_signal, &iter,
			   COLUMN_PTR, signal,
			   COLUMN_TITLE, signal->description().c_str(),
			   COLUMN_ENABLED, signal->enabled(),
			   -1);
}

void gui_append_signal(signal_record_t *signal)
{
	GtkTreeIter iter;
	gtk_list_store_append(store_signal, &iter);
	gui_set_signal(iter, signal);
}

void gui_append_slot(slot_record_t *slot)
{
	gtk_list_store_insert_with_values(
		store_slot, NULL, -1,
		COLUMN_PTR, slot,
		COLUMN_TITLE, slot->name().c_str(),
		COLUMN_ENABLED, slot->enabled(),
		COLUMN_PARAMETER, slot->has_param()
		? slot->param().c_str() : "",
		-1);
}

void
gui_append_profile(profile_t *profile)
{
	gtk_list_store_insert_with_values(
		store_profile, NULL, -1,
		COLUMN_PTR, profile,
		COLUMN_TITLE, profile->name().c_str(),
		-1);
}

void
on_profile_edit_win_destroy(GtkWidget *widget,
				 gpointer data)
{
	gtk_main_quit();
}

void
pmx_profile_edit_t::save_profiles()
{
	DBusMessage *message, *result;
	DBusError error;

	vector<string> names, xmls;
	const char **str_arr;
	print_debug("%s\n", __func__);
	store_curr_signal();

	FOREACH (profile_t::map_t, i, profile_t::obj_map()) {
		profile_t *profile = i->second;
		if (profile->edited()) {
			print_debug("edited %s\n", profile->name().c_str());
			mod_profiles.erase(profile->name());
			names.push_back(profile->name());
			xmls.push_back(profile->to_xml());
		}
	}
	FOREACH(set<string>, profile_name, mod_profiles) {
		names.push_back(*profile_name);
		xmls.push_back("");
	}
	int n_profiles = names.size();
	if (!n_profiles) {
		print_debug("exxxxxx\n");
		return;
	}
	message = dbus_message_new_method_call(
			POWERMAN_SERVICE, POWERMAN_PATH,
			POWERMAN_INTERFACE, "ReloadProfiles");
	str_arr = new const char*[n_profiles];
	for (int i = 0; i < n_profiles; ++i)
		str_arr[i] = names[i].c_str();
	dbus_message_append_args(
		message,
		DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &str_arr, n_profiles,
		DBUS_TYPE_INVALID);
	for (int i = 0; i < n_profiles; ++i)
		str_arr[i] = xmls[i].c_str();
	dbus_message_append_args(
		message,
		DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &str_arr, n_profiles,
		DBUS_TYPE_INVALID);
	delete []str_arr;

	dbus_error_init(&error);
	result = dbus_connection_send_with_reply_and_block(
		glob_dbus_conn, message, -1, &error);
	if (dbus_error_is_set(&error)) {
		print_debug("Couldn't send dbus message: %s\n",
			    error.message);
		dbus_error_free(&error);
	} else {
		dbus_message_unref(result);
		FOREACH (profile_t::map_t, i, profile_t::obj_map())
			i->second->edited() = false;
		mod_profiles.clear();
	}
	dbus_message_unref(message);

	status_icon->profiles_changed();
}

void on_btn_edit_clicked(GtkButton *button,
			 gpointer   user_data)
{
	if (button == (GtkButton*)btn_edit_ok ||
	    button == (GtkButton*)btn_edit_apply) {
		pr_edit->save_profiles();
	}
	if (button == (GtkButton*)btn_edit_ok ||
	    button == (GtkButton*)btn_edit_close) {
		gtk_widget_hide(profile_edit_win);
	}
}

void on_btn_profile_add_clicked(GtkButton *button,
			gpointer   user_data)
{
	gint result = gtk_dialog_run(dlg_add_profile);
	gtk_widget_hide(GTK_WIDGET(dlg_add_profile));
	if (result != GTK_RESPONSE_OK)
		return;
	const char *name = gtk_entry_get_text(GTK_ENTRY(new_profile_name));
	if (name[0] == '\0' || profile_t::find(name))
		return;
	profile_t *profile = new profile_t(name);
	profile->edited() = true;
	gui_append_profile(profile);
}

void on_btn_profile_remove_clicked(GtkButton *button,
				   gpointer   user_data)
{
	FOREACH(profile_t::map_t, i, profile_t::obj_map())
		if (i->second == curr_profile) {
			GtkTreeIter iter;

			profile_t::obj_map().erase(i);
			mod_profiles.insert(curr_profile->name());
			delete curr_profile;
			curr_profile = NULL;
			gtk_tree_selection_get_selected(
				gtk_tree_view_get_selection(GTK_TREE_VIEW(view_profile)),
				NULL, &iter);
			gtk_list_store_remove(store_profile, &iter);
			break;
		}
}

void on_btn_rec_add_clicked(GtkButton *button,
			    gpointer   user_data)
{
	GtkTreeIter iter;
	bool add_signal = btn_signal_add == (void*)button;

	gtk_window_set_title(GTK_WINDOW(dlg_add_record),
			     add_signal ? "New signal": "New slot");
	gtk_list_store_clear(store_plugin_name);
	if (add_signal) {
		FOREACH(signal_plugin_t::map_t, i, signal_plugin_t::obj_map()) {
			gtk_list_store_insert_with_values(
				store_plugin_name, NULL, -1,
				0, i->second,
				1, i->first,
				-1);
		}
	} else {
		FOREACH(slot_plugin_t::map_t, i, slot_plugin_t::obj_map()) {
			gtk_list_store_insert_with_values(
				store_plugin_name, NULL, -1,
				0, i->second,
				1, i->first,
				-1);
		}
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(cbx_plugin_name), 0);
	gint result = gtk_dialog_run(dlg_add_record);
	gtk_widget_hide(GTK_WIDGET(dlg_add_record));
	if (result != GTK_RESPONSE_OK)
		return;

	void *plugin;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cbx_plugin_name), &iter)) {
		gtk_tree_model_get(
			GTK_TREE_MODEL(store_plugin_name), &iter,
			0, &plugin,
			-1);
	} else {
		return;
	}
	if (add_signal) {
		signal_record_t *rec = static_cast<signal_plugin_t*>(plugin)->create_record();
		curr_profile->signals().push_back(rec);
		gui_append_signal(rec);
	} else {
		slot_record_t *rec = static_cast<slot_plugin_t*>(plugin)->create_record();
		curr_signal->slots().push_back(rec);
		gui_append_slot(rec);
	}
	set_edited();
}

void on_btn_signal_remove_clicked(GtkButton *button,
				  gpointer   user_data)
{
	FOREACH(profile_t::signals_t, signal, curr_profile->signals())
		if (*signal == curr_signal) {
			GtkTreeIter iter;

			set_edited();
			curr_profile->signals().erase(signal);
			delete curr_signal;
			curr_signal = NULL;
			gtk_tree_selection_get_selected(
				gtk_tree_view_get_selection(
					GTK_TREE_VIEW(view_signal)), NULL, &iter);
			gtk_list_store_remove(store_signal, &iter);
			break;
		}
}

void on_btn_slot_remove_clicked(GtkButton *button,
				gpointer   user_data)
{
	FOREACH(signal_record_t::slots_t, slot, curr_signal->slots())
		if (*slot == curr_slot) {
			GtkTreeIter iter;

			set_edited();
			curr_signal->slots().erase(slot);
			delete curr_slot;
			curr_slot = NULL;
			gtk_tree_selection_get_selected(
				gtk_tree_view_get_selection(
					GTK_TREE_VIEW(view_slot)), NULL, &iter);
			gtk_list_store_remove(store_slot, &iter);
			break;
		}
}

void on_signal_enabled_toggled(GtkCellRendererToggle *cell_renderer,
			      gchar                 *path,
			      gpointer               user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(store_signal);

	gtk_tree_model_get_iter_from_string(model, &iter, path);

	gtk_tree_model_get(model, &iter, COLUMN_PTR, &curr_signal, -1);
	curr_signal->enabled() = !curr_signal->enabled();
	gtk_list_store_set(store_signal, &iter,
			   COLUMN_ENABLED, (gboolean)curr_signal->enabled(), -1);
	set_edited();
}

void on_slot_enabled_toggled(GtkCellRendererToggle *cell_renderer,
			    gchar                 *path,
			    gpointer               user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(store_slot);

	gtk_tree_model_get_iter_from_string(model, &iter, path);
	gtk_tree_model_get(model, &iter, COLUMN_PTR, &curr_slot, -1);
	curr_slot->enabled() = !curr_slot->enabled();
	gtk_list_store_set(store_slot, &iter,
			   COLUMN_ENABLED, (gboolean)curr_slot->enabled(), -1);
	set_edited();
}

void on_slot_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		curr_slot = NULL;
		gtk_widget_set_sensitive(btn_slot_remove, FALSE);
		return;
	}
	gtk_widget_set_sensitive(btn_slot_remove, TRUE);

	gtk_tree_model_get(model, &iter, COLUMN_PTR, &curr_slot, -1);
	printf("selected: %s\n", curr_slot->name().c_str());
	g_object_set(ren_slot_param, "editable",
		     curr_slot->plugin()->has_param(), NULL);
	if (curr_slot->plugin()->has_param()) {
		g_object_set(G_OBJECT(ren_slot_param),
			     "has_entry", (gboolean)curr_slot->plugin()->any_param(),
			     NULL);
	} else {
		return;
	}

	gtk_list_store_clear(store_slot_param);

	curr_slot->plugin()->reload_params();
	FOREACH_CONST(vector<string>, i, curr_slot->plugin()->avail_params()) {
		gtk_list_store_insert_with_values(
			store_slot_param, NULL, -1,
			0, i->c_str(),
			-1);
		//printf("append: %s\n", i->c_str());
	}
}

void on_slot_param_edited(GtkCellRendererText *renderer,
			  gchar               *path,
			  gchar               *new_text,
			  gpointer             user_data)
{
	GtkTreeIter iter;
	printf("edited: %s, %s\n", path, new_text);
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store_slot), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(store_slot), &iter, COLUMN_PTR, &curr_slot, -1);
	gtk_list_store_set(store_slot, &iter, COLUMN_PARAMETER, new_text, -1);
	curr_slot->set_param(new_text);
	set_edited();
}

typedef struct {
	gboolean found;
	GtkTreeIter iter;
	gchar *text;
} SearchData;

static gboolean
find_text(GtkTreeModel *model,
	  GtkTreePath  *path,
	  GtkTreeIter  *iter,
	  gpointer      data)
{
	SearchData *search_data = (SearchData *)data;
	gchar *text;

	gtk_tree_model_get(model, iter, 0, &text, -1);
	if (text && !strcmp(text, search_data->text)) {
		search_data->iter = *iter;
		search_data->found = TRUE;
	}
	g_free(text);
	return search_data->found;
}

void show_curr_signal()
{
	gtk_widget_set_sensitive(box_timeout, curr_signal->can_timeout());
	if (curr_signal->can_timeout()) {
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(val_timeout),
			curr_signal->timeout());
	}

	gtk_widget_hide(box_bool);
	gtk_widget_hide(box_int);
	gtk_widget_hide(box_hotkey);

	switch (curr_signal->settings_type()) {
		signal_record_ib_t::react_t react;
	case SETTINGS_BOOL:
		gtk_widget_show(box_bool);

		react = bool_signal->react();
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(
				react == signal_record_ib_t::ON_CHANGED ?
				rb_bool_changed : rb_bool_value),
			TRUE);
		if (react == signal_record_ib_t::ON_SIG_VALUE) {
			gtk_combo_box_set_active(
				GTK_COMBO_BOX(val_bool_value),
				bool_signal->sig_value());
		}
		break;
	case SETTINGS_INT:
		gtk_widget_show(box_int);

		react = int_signal->react();
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(
				react == signal_record_ib_t::ON_CHANGED ?
				rb_int_changed :
				react == signal_record_ib_t::ON_SIG_VALUE ?
				rb_int_value : rb_int_interval),
			TRUE);
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(val_int_value),
			int_signal->sig_value());
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(chb_int_from),
			int_signal->on_interval_from());
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(chb_int_to),
			int_signal->on_interval_to());
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(val_int_from),
			int_signal->interval_from());
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(val_int_to),
			int_signal->interval_to());
		break;
	case SETTINGS_HOTKEY:
		gtk_widget_show(box_hotkey);

		SearchData search_data;
		printf("txt: %s\n", hotkey_signal->key_name().c_str());
		search_data.found = FALSE;
		search_data.text = &hotkey_signal->key_name()[0];
		gtk_tree_model_foreach(
			GTK_TREE_MODEL(store_hotkey),
			find_text, &search_data);
		if (search_data.found) {
			printf("set active\n");
			gtk_combo_box_set_active_iter(
				GTK_COMBO_BOX(val_hotkey),
				&(search_data.iter));
			printf("found\n");
		} else {
			gtk_entry_set_text(
				GTK_ENTRY(gtk_bin_get_child(GTK_BIN(val_hotkey))),
				hotkey_signal->key_name().c_str());
			printf("not found\n");
		}
		break;
	case SETTINGS_VOID:
		break;
	case SETTINGS_OTHER:
		break;
	}
}

#define SET_PROP(a, b) do { const __typeof(a) t = b; if (a != t) { set_edited(); a = t; } } while (0)

void store_curr_signal()
{
	if (curr_signal == NULL)
		return;
	printf("store: %s\n", curr_signal->name().c_str());
	if (curr_signal->can_timeout()) {
		gtk_spin_button_update (GTK_SPIN_BUTTON(val_timeout));
		SET_PROP(curr_signal->timeout(),
			 gtk_spin_button_get_value_as_int(
				 GTK_SPIN_BUTTON(val_timeout)));
	}
	switch (curr_signal->settings_type()) {
		signal_record_ib_t::react_t react;
	case SETTINGS_BOOL:
		react = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(rb_bool_changed)) ?
			signal_record_ib_t::ON_CHANGED
			: signal_record_ib_t::ON_SIG_VALUE;
		SET_PROP(bool_signal->react(), react);
		if (react == signal_record_ib_t::ON_SIG_VALUE) {
			SET_PROP(bool_signal->sig_value(),
				 gtk_combo_box_get_active(
					 GTK_COMBO_BOX(val_bool_value)));
		}
		break;
	case SETTINGS_INT:
		react = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(rb_int_changed)) ?
			signal_record_ib_t::ON_CHANGED
			: gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(rb_int_value)) ?
			signal_record_ib_t::ON_SIG_VALUE
			: signal_record_ib_t::ON_INTERVAL;
		SET_PROP(int_signal->react(), react);
		if (react == signal_record_ib_t::ON_SIG_VALUE) {
			gtk_spin_button_update(
				GTK_SPIN_BUTTON(val_int_value));
			SET_PROP(int_signal->sig_value(),
				 gtk_spin_button_get_value_as_int(
					 GTK_SPIN_BUTTON(val_int_value)));
		} else if (react == signal_record_ib_t::ON_INTERVAL) {
			SET_PROP(int_signal->on_interval_from(),
				 gtk_toggle_button_get_active(
					 GTK_TOGGLE_BUTTON(chb_int_from)));
			SET_PROP(int_signal->on_interval_to(),
				 gtk_toggle_button_get_active(
					 GTK_TOGGLE_BUTTON(chb_int_to)));
			if (int_signal->on_interval_from()) {
				gtk_spin_button_update(
					GTK_SPIN_BUTTON(val_int_from));
				SET_PROP(int_signal->interval_from(),
					 gtk_spin_button_get_value_as_int(
						 GTK_SPIN_BUTTON(val_int_from)));
			}
			if (int_signal->on_interval_to()) {
				gtk_spin_button_update(
					GTK_SPIN_BUTTON(val_int_to));
				SET_PROP(int_signal->interval_to(),
					 gtk_spin_button_get_value_as_int(
						 GTK_SPIN_BUTTON(val_int_to)));
			}
		}
		break;
	case SETTINGS_HOTKEY:
		const char *key_name;
		key_name = gtk_combo_box_get_active_id(
			GTK_COMBO_BOX(val_hotkey));
		if (!key_name) {
			hotkey_signal->key_name() = "";
			set_edited();
			break;
		}
		if (strcmp(key_name, hotkey_signal->key_name().c_str())) {
			hotkey_signal->key_name() = key_name;
			set_edited();
		}
		break;
	case SETTINGS_VOID:
		break;
	case SETTINGS_OTHER:
		break;
	}

	if (curr_signal_iter_valid)
		gui_set_signal(curr_signal_iter, curr_signal);
}

void on_signal_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	store_curr_signal();
	gtk_list_store_clear(store_slot);
	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		curr_signal = NULL;
		gtk_widget_set_sensitive(btn_signal_remove, FALSE);
		gtk_widget_set_sensitive(btn_slot_add, FALSE);
		return;
	}
	curr_signal_iter = iter;
	curr_signal_iter_valid = true;

	gtk_widget_set_sensitive(btn_signal_remove, TRUE);
	gtk_widget_set_sensitive(btn_slot_add, TRUE);
	printf("sel changed - process\n");

	gtk_tree_model_get(model, &iter, COLUMN_PTR, &curr_signal, -1);
	printf("\tsignal: %s\n", curr_signal->name().c_str());
	curr_slot = NULL;
	FOREACH(signal_record_t::slots_t, slot, curr_signal->slots())
		gui_append_slot(*slot);

	show_curr_signal();
}

void on_profile_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	curr_signal_iter_valid = false;
	gtk_list_store_clear(store_signal);
	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		curr_profile = NULL;
		gtk_widget_set_sensitive(btn_profile_remove, FALSE);
		gtk_widget_set_sensitive(btn_signal_add, FALSE);
		return;
	}
	gtk_widget_set_sensitive(btn_profile_remove, TRUE);
	gtk_widget_set_sensitive(btn_signal_add, TRUE);
	gtk_tree_model_get(model, &iter, COLUMN_PTR, &curr_profile, -1);
	printf("profile %s, edited=%d\n", curr_profile->name().c_str(), curr_profile->edited());

	FOREACH(profile_t::signals_t, signal, curr_profile->signals())
		gui_append_signal(*signal);
}

void on_profile_name_edited(GtkCellRendererText *renderer,
			    gchar               *path,
			    gchar               *new_text,
			    gpointer             user_data)
{
	GtkTreeIter iter;
	printf("edited profile name: %s, %s\n", path, new_text);
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store_profile), &iter, path);
	//gtk_tree_model_get(GTK_TREE_MODEL(store_plugin), &iter, COLUMN_PTR, &curr_profile, -1);
	gtk_list_store_set(store_profile, &iter, COLUMN_TITLE, new_text, -1);
	mod_profiles.insert(curr_profile->name());
	curr_profile->set_name(new_text);
	set_edited();
}

pmx_profile_edit_t::pmx_profile_edit_t(
	GtkBuilder *builder,
	pmx_status_icon_t *status_icon)
{
	this->status_icon = status_icon;
	pr_edit = this;

	SET_OBJECT(profile_edit_win);
	SET_OBJECT(view_profile);
	SET_OBJECT(view_signal);
	SET_OBJECT(view_slot);
	SET_OBJECT(box_signal_ar_btns);
	SET_OBJECT(box_slot_ar_btns);
	SET_OBJECT(box_bool);
	SET_OBJECT(box_int);
	SET_OBJECT(box_signal_sets);
	SET_OBJECT(box_timeout);
	SET_OBJECT(btn_edit_ok);
	SET_OBJECT(btn_edit_apply);
	SET_OBJECT(btn_edit_close);
	SET_OBJECT(rb_bool_changed);
	SET_OBJECT(rb_bool_value);
	SET_OBJECT(val_bool_value);
	SET_OBJECT(val_timeout);
	SET_OBJECT(rb_int_changed);
	SET_OBJECT(rb_int_value);
	SET_OBJECT(val_int_value);
	SET_OBJECT(rb_int_interval);
	SET_OBJECT(chb_int_from);
	SET_OBJECT(chb_int_to);
	SET_OBJECT(val_int_from);
	SET_OBJECT(val_int_to);
	SET_OBJECT(box_hotkey);
	SET_OBJECT(val_hotkey);
	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(val_hotkey), 0);

	SET_OBJECT(store_hotkey);
	SET_OBJECT(btn_profile_add);
	SET_OBJECT(btn_profile_remove);
	SET_OBJECT(btn_signal_add);
	SET_OBJECT(btn_signal_remove);
	SET_OBJECT(btn_slot_add);
	SET_OBJECT(btn_slot_remove);

	SET_OBJECT(dlg_add_record);
	SET_OBJECT(store_plugin_name);
	SET_OBJECT(cbx_plugin_name);

	SET_OBJECT(dlg_add_profile);
	SET_OBJECT(new_profile_name);

	CONNECT_SIGNAL(profile_edit_win, destroy);
	g_signal_connect(G_OBJECT(profile_edit_win), "delete-event",
			 G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	CONNECT_SIGNAL(btn_profile_add, clicked);
	CONNECT_SIGNAL(btn_profile_remove, clicked);
	CONNECT_SIGNAL_NAMED(btn_signal_add, btn_rec_add, clicked);
	CONNECT_SIGNAL_NAMED(btn_slot_add, btn_rec_add, clicked);
	CONNECT_SIGNAL(btn_signal_remove, clicked);
	CONNECT_SIGNAL(btn_slot_remove, clicked);

	CONNECT_SIGNAL_NAMED(btn_edit_ok, btn_edit, clicked);
	CONNECT_SIGNAL_NAMED(btn_edit_apply, btn_edit, clicked);
	CONNECT_SIGNAL_NAMED(btn_edit_close, btn_edit, clicked);

#define SETUP_SELECTION(name) do {					\
		GtkTreeSelection    *select;				\
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(view_##name)); \
		gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE); \
		CONNECT_SIGNAL_NAMED(select, name##_selection, changed); \
	}while (0)

	//Setup profile
	SET_OBJECT(store_profile);
	SET_OBJECT(view_profile);
	SET_RENDERER_TEXT_SORTABLE(profile_name, COLUMN_TITLE);
	SETUP_SELECTION(profile);
	FOREACH(profile_t::map_t, i, profile_t::obj_map())
		gui_append_profile(i->second);

	//Setup signal
	SET_OBJECT(store_signal);
	SET_OBJECT(view_signal);
	SETUP_COLUMN_SORTABILITY(signal_name, COLUMN_TITLE);
	SET_RENDERER_BOOL(signal_enabled);
	SETUP_SELECTION(signal);

	//Setup slot
	SET_OBJECT(store_slot);
	SET_OBJECT(view_slot);
	SETUP_COLUMN_SORTABILITY(slot_name, COLUMN_TITLE);
	SET_OBJECT(ren_slot_param);
	SET_RENDERER_TEXT(slot_param);
	SET_RENDERER_BOOL(slot_enabled);
	SETUP_SELECTION(slot);

	SET_OBJECT(store_slot_param);
}

void
pmx_profile_edit_t::show()
{
	gtk_widget_show(profile_edit_win);
}
