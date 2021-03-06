/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
    Gpredict: Real-time satellite tracking and orbit prediction program

    Copyright (C)  2001-2013  Alexandru Csete, OZ9AEC.

    Authors: Alexandru Csete <oz9aec@gmail.com>

    Comments, questions and bugreports should be submitted via
    http://sourceforge.net/projects/gpredict/
    More details can be found at the project home page:

            http://gpredict.oz9aec.net/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, visit http://www.fsf.org/
*/
/** \brief Pop-up menu used by GtkSatList, GtkSatMap, etc.
 */
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <goocanvas.h>
#include "sgpsdp/sgp4sdp4.h"
#include "sat-log.h"
#include "config-keys.h"
#include "sat-cfg.h"
#include "mod-cfg-get-param.h"
#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include "gtk-polar-view.h"
#include "time-tools.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-pass-dialogs.h"
#include "sat-info.h"
#include "gtk-polar-view-popup.h"
#include "gtk-sat-popup-common.h"

static void     track_toggled(GtkCheckMenuItem * item, gpointer data);

/* static void target_toggled (GtkCheckMenuItem *item, gpointer data); */


/** \brief Show satellite popup menu.
 *  \param sat Pointer to the satellite data.
 *  \param qth The current location.
 *  \param pview The GtkPolarView widget.
 *  \param event The mouse-click related event info
 *  \param toplevel The toplevel window or NULL.
 *
 */
void
gtk_polar_view_popup_exec(sat_t * sat,
                          qth_t * qth,
                          GtkPolarView * pview,
                          GdkEventButton * event, GtkWidget * toplevel)
{
    GtkWidget      *menu;
    GtkWidget      *menuitem;
    GtkWidget      *label;
    GtkWidget      *image;
    gchar          *buff;
    sat_obj_t      *obj = NULL;
    gint           *catnum;



    menu = gtk_menu_new();

    /* first menu item is the satellite name, centered */
    menuitem = gtk_image_menu_item_new();
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    buff = g_markup_printf_escaped("<b>%s</b>", sat->nickname);
    gtk_label_set_markup(GTK_LABEL(label), buff);
    g_free(buff);
    gtk_container_add(GTK_CONTAINER(menuitem), label);
    image = gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);

    /* attach data to menuitem and connect callback */
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_sat_info_menu_cb),
                     toplevel);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* add the menu items for current,next, and future passes. */
    add_pass_menu_items(menu, sat, qth, &pview->tstamp, GTK_WIDGET(pview));

    /* separator */
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    /* get sat obj since we'll need it for the remaining items */
    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;
    obj = SAT_OBJ(g_hash_table_lookup(pview->obj, catnum));
    g_free(catnum);

    /* show track */
    menuitem = gtk_check_menu_item_new_with_label(_("Sky track"));
    g_object_set_data(G_OBJECT(menuitem), "sat", sat);
    g_object_set_data(G_OBJECT(menuitem), "qth", qth);
    g_object_set_data(G_OBJECT(menuitem), "obj", obj);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
                                   obj->showtrack);
    g_signal_connect(menuitem, "activate", G_CALLBACK(track_toggled), pview);

    /* disable menu item if satellite is geostationary */
    if (sat->otype == ORBIT_TYPE_GEO)
        gtk_widget_set_sensitive(menuitem, FALSE);

    /* target */
    /*      menuitem = gtk_check_menu_item_new_with_label (_("Set as target")); */
    /*      g_object_set_data (G_OBJECT (menuitem), "sat", sat); */
    /*      g_object_set_data (G_OBJECT (menuitem), "qth", qth); */
    /*      g_object_set_data (G_OBJECT (menuitem), "obj", obj); */
    /*      gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem); */
    /*      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), obj->istarget); */
    /*      g_signal_connect (menuitem, "activate", G_CALLBACK (target_toggled), pview); */

    gtk_widget_show_all(menu);

    /* Note: event can be NULL here when called from view_onPopupMenu;
     *  gdk_event_get_time() accepts a NULL argument */
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent *) event));


}



/** \brief Manage toggling of Ground Track.
 *  \param item The menu item that was toggled.
 *  \param data Pointer to the GtkPolarView structure.
 *
 */
static void track_toggled(GtkCheckMenuItem * item, gpointer data)
{
    GtkPolarView   *pv = GTK_POLAR_VIEW(data);
    sat_obj_t      *obj = NULL;
    sat_t          *sat;

    /* qth_t              *qth; Unused */
    gint           *catnum;


    /* get satellite object */
    obj = SAT_OBJ(g_object_get_data(G_OBJECT(item), "obj"));
    sat = SAT(g_object_get_data(G_OBJECT(item), "sat"));
    /*qth = (qth_t *)(g_object_get_data (G_OBJECT (item), "qth")); */

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to get satellite object."),
                    __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->showtrack = !obj->showtrack;
    gtk_check_menu_item_set_active(item, obj->showtrack);

    catnum = g_new0(gint, 1);
    *catnum = sat->tle.catnr;

    if (obj->showtrack)
    {
        /* add sky track */

        /* add it to the storage structure */
        g_hash_table_insert(pv->showtracks_on, catnum, NULL);
        /* remove it from the don't show */
        g_hash_table_remove(pv->showtracks_off, catnum);

        gtk_polar_view_create_track(pv, obj, sat);
    }
    else
    {
        /* add it to the hide */
        g_hash_table_insert(pv->showtracks_off, catnum, NULL);
        /* remove it from the show */
        g_hash_table_remove(pv->showtracks_on, catnum);

        /* delete sky track */
        gtk_polar_view_delete_track(pv, obj, sat);
    }

}


#if 0
/** \brief Manage toggling of Set Target.
 *  \param item The menu item that was toggled.
 *  \param data Pointer to the GtkPolarView structure.
 *
 */
static void target_toggled(GtkCheckMenuItem * item, gpointer data)
{
    sat_obj_t      *obj = NULL;



    /* get satellite object */
    obj = SAT_OBJ(g_object_get_data(G_OBJECT(item), "obj"));

    if (obj == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Failed to get satellite object."),
                    __FILE__, __LINE__);
        return;
    }

    /* toggle flag */
    obj->istarget = !obj->istarget;
    gtk_check_menu_item_set_active(item, obj->istarget);
}
#endif
