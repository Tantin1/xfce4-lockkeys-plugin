/*
 * xfce4-lockkeys-plugin – Lock Keys State Plugin for the Xfce Panel
 *
 * Shows the state of Caps Lock and Num Lock with icons from the GTK icon theme.
 * Similar to KDE Plasma's "Lock Keys State" applet.
 *
 * Copyright (C) 2024
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */



#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>
#include <libnotify/notify.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "lockkeys.h"

/* ── Constants ──────────────────────────────────────────────────────────────── */

#define DEFAULT_SHOW_CAPS    TRUE
#define DEFAULT_SHOW_NUM     TRUE
#define DEFAULT_ICON_SIZE    22
#define DEFAULT_HIDE_INACTIVE FALSE
#define INDICATOR_WIDTH      28

#define ICON_CAPS      "input-caps-on"
#define ICON_NUM       "input-num-on"
#define ICON_FALLBACK  "input-keyboard-symbolic"

#define OPACITY_ON   1.00
#define OPACITY_OFF  0.30

/* ── Forward declarations ───────────────────────────────────────────────────── */

static void     lockkeys_construct           (XfcePanelPlugin *plugin);
static void     lockkeys_free                (XfcePanelPlugin *plugin, LockKeysPlugin *lk);
static gboolean lockkeys_size_changed        (XfcePanelPlugin *plugin, gint size, LockKeysPlugin *lk);
static void     lockkeys_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, LockKeysPlugin *lk);
static void     lockkeys_configure_plugin    (XfcePanelPlugin *plugin, LockKeysPlugin *lk);
static void     lockkeys_save                (XfcePanelPlugin *plugin, LockKeysPlugin *lk);

XFCE_PANEL_PLUGIN_REGISTER (lockkeys_construct);


/* ── XKB state ──────────────────────────────────────────────────────────────── */

static gboolean
lockkeys_get_state (gboolean *caps_on, gboolean *num_on)
{
    Display     *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    XkbStateRec  state;

    if (!dpy || XkbGetState (dpy, XkbUseCoreKbd, &state) != Success)
        return FALSE;

    if (caps_on) *caps_on = (state.locked_mods & LockMask) != 0;
    if (num_on)  *num_on  = (state.locked_mods & Mod2Mask) != 0;
    return TRUE;
}


/* ── Icon loading ───────────────────────────────────────────────────────────── */

static void
lockkeys_set_icon (GtkWidget   *image,
                   const gchar *name,
                   gint         size)
{
    GtkIconTheme *theme  = gtk_icon_theme_get_default ();
    GdkPixbuf    *pixbuf = NULL;
    GError       *err    = NULL;

    if (gtk_icon_theme_has_icon (theme, name))
        pixbuf = gtk_icon_theme_load_icon (theme, name, size,
                                           GTK_ICON_LOOKUP_FORCE_SIZE, &err);
    if (!pixbuf)
    {
        g_clear_error (&err);
        pixbuf = gtk_icon_theme_load_icon (theme, ICON_FALLBACK, size,
                                           GTK_ICON_LOOKUP_FORCE_SIZE, &err);
    }

    if (pixbuf)
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
        g_object_unref (pixbuf);
    }
    else
    {
        g_clear_error (&err);
        gtk_image_set_from_icon_name (GTK_IMAGE (image), ICON_FALLBACK,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);
    }
}


/* ── Notificaciones ─────────────────────────────────────────────────────────── */

static void
lockkeys_notify (const gchar *summary, const gchar *icon_name)
{
    if (!notify_is_initted ())
        notify_init ("xfce4-lockkeys-plugin");

    NotifyNotification *n = notify_notification_new (summary, NULL, icon_name);
    notify_notification_set_timeout (n, 2000);
    notify_notification_show (n, NULL);
    g_object_unref (n);
}

/* ── UI update ──────────────────────────────────────────────────────────────── */

static void
lockkeys_update (LockKeysPlugin *lk)
{
    gboolean caps_on = FALSE, num_on = FALSE;

    if (!lockkeys_get_state (&caps_on, &num_on))
        return;

    /* Skip redraw if nothing changed.
     * -1 means uninitialized — always draw in that case even if
     * the key state happens to be 0, so icons are never left empty. */
    if (lk->caps_state != -1 && lk->num_state != -1 &&
        (gint) caps_on == lk->caps_state && (gint) num_on == lk->num_state)
        return;

    if (lk->notifications && lk->caps_state != -1 && (gint) caps_on != lk->caps_state)
        lockkeys_notify (caps_on ? _("Caps Lock: ON") : _("Caps Lock: OFF"),
                         ICON_CAPS);

    if (lk->notifications && lk->num_state != -1 && (gint) num_on != lk->num_state)
        lockkeys_notify (num_on ? _("Num Lock: ON") : _("Num Lock: OFF"),
                         ICON_NUM);

    lk->caps_state = (gint) caps_on;
    lk->num_state  = (gint) num_on;

    /* ── Caps Lock indicator ── */
    if (lk->show_caps && lk->caps_box)
    {
        if (lk->hide_inactive && !caps_on)
        {
            gtk_widget_hide (lk->caps_box);
        }
        else
        {
            lockkeys_set_icon (lk->caps_image, ICON_CAPS, lk->icon_size);
            gtk_widget_set_opacity (lk->caps_image,
                                    caps_on ? OPACITY_ON : OPACITY_OFF);
            gtk_widget_set_tooltip_text (lk->caps_box,
                                         caps_on ? _("Caps Lock: ON")
                                                 : _("Caps Lock: OFF"));
            gtk_widget_show (lk->caps_box);
        }
    }

    /* ── Num Lock indicator ── */
    if (lk->show_num && lk->num_box)
    {
        if (lk->hide_inactive && !num_on)
        {
            gtk_widget_hide (lk->num_box);
        }
        else
        {
            lockkeys_set_icon (lk->num_image, ICON_NUM, lk->icon_size);
            gtk_widget_set_opacity (lk->num_image,
                                    num_on ? OPACITY_ON : OPACITY_OFF);
            gtk_widget_set_tooltip_text (lk->num_box,
                                         num_on ? _("Num Lock: ON")
                                                : _("Num Lock: OFF"));
            gtk_widget_show (lk->num_box);
        }
    }

    /*
     * If hide_inactive is on and both keys are off, hide the main box
     * entirely so the plugin takes zero space in the panel.
     * Otherwise make sure the box is visible.
     */
    if (lk->hide_inactive)
    {
        gboolean any_visible =
            (lk->show_caps && caps_on) ||
            (lk->show_num  && num_on);

        if (any_visible)
            gtk_widget_show (lk->box);
        else
            gtk_widget_hide (lk->box);
    }
    else
    {
        gtk_widget_show (lk->box);
    }
}

/* ── Filtro de eventos XKB ──────────────────────────────────────────────────── */

static GdkFilterReturn
lockkeys_xkb_filter (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    LockKeysPlugin *lk  = (LockKeysPlugin *) data;
    XkbStateNotifyEvent *xev = (XkbStateNotifyEvent *) xevent;

    
    if (xev->xkb_type == XkbStateNotify &&
        (xev->changed & XkbModifierLockMask))
        {
            lockkeys_update (lk);
        }
    return GDK_FILTER_CONTINUE;
}


/* ── Build / rebuild widget tree ────────────────────────────────────────────── */

static void
lockkeys_rebuild_ui (LockKeysPlugin *lk)
{
    GList *children = gtk_container_get_children (GTK_CONTAINER (lk->box));
    for (GList *l = children; l; l = l->next)
        gtk_container_remove (GTK_CONTAINER (lk->box), GTK_WIDGET (l->data));
    g_list_free (children);

    lk->caps_image = NULL;
    lk->num_image  = NULL;

    if (lk->show_caps)
    {
        lk->caps_box   = gtk_event_box_new ();
        lk->caps_image = gtk_image_new ();
        gtk_widget_set_size_request (lk->caps_box, INDICATOR_WIDTH, -1);
        gtk_widget_set_halign (lk->caps_image, GTK_ALIGN_CENTER);
        gtk_widget_set_valign (lk->caps_image, GTK_ALIGN_CENTER);
        gtk_container_add  (GTK_CONTAINER (lk->caps_box), lk->caps_image);
        gtk_box_pack_start (GTK_BOX (lk->box), lk->caps_box, FALSE, FALSE, 0);
        /* Start hidden — lockkeys_update will show with correct icon/opacity */
        gtk_widget_show (lk->caps_image);
        gtk_widget_hide (lk->caps_box);
    }

    if (lk->show_num)
    {
        lk->num_box   = gtk_event_box_new ();
        lk->num_image = gtk_image_new ();
        gtk_widget_set_size_request (lk->num_box, INDICATOR_WIDTH, -1);
        gtk_widget_set_halign (lk->num_image, GTK_ALIGN_CENTER);
        gtk_widget_set_valign (lk->num_image, GTK_ALIGN_CENTER);
        gtk_container_add  (GTK_CONTAINER (lk->num_box), lk->num_image);
        gtk_box_pack_start (GTK_BOX (lk->box), lk->num_box, FALSE, FALSE, 0);
        /* Start hidden — lockkeys_update will show with correct icon/opacity */
        gtk_widget_show (lk->num_image);
        gtk_widget_hide (lk->num_box);
    }

    gtk_widget_show (lk->box);

    /* Force full redraw on next poll */
    lk->caps_state = -1;
    lk->num_state  = -1;
    lockkeys_update (lk);
}


/* ── Settings ───────────────────────────────────────────────────────────────── */

static void
lockkeys_read_settings (LockKeysPlugin *lk)
{
    gchar  *path = xfce_panel_plugin_save_location (lk->plugin, FALSE);
    XfceRc *rc;

    if (!path) goto defaults;
    rc = xfce_rc_simple_open (path, TRUE);
    g_free (path);
    if (!rc) goto defaults;

    lk->show_caps     = xfce_rc_read_bool_entry (rc, "show_caps",     DEFAULT_SHOW_CAPS);
    lk->show_num      = xfce_rc_read_bool_entry (rc, "show_num",      DEFAULT_SHOW_NUM);
    lk->hide_inactive    = xfce_rc_read_bool_entry (rc, "hide_inactive",    DEFAULT_HIDE_INACTIVE);
    lk->notifications    = xfce_rc_read_bool_entry (rc, "notifications",    TRUE);
    lk->manual_icon_size = xfce_rc_read_bool_entry (rc, "manual_icon_size", FALSE);
    lk->icon_size        = xfce_rc_read_int_entry  (rc, "icon_size",        DEFAULT_ICON_SIZE);
    if (lk->icon_size < 8 || lk->icon_size > 128)
        lk->icon_size = DEFAULT_ICON_SIZE;
    xfce_rc_close (rc);
    return;

defaults:
    lk->show_caps        = DEFAULT_SHOW_CAPS;
    lk->show_num         = DEFAULT_SHOW_NUM;
    lk->hide_inactive    = DEFAULT_HIDE_INACTIVE;
    lk->notifications    = TRUE;
    lk->manual_icon_size = FALSE;
    lk->icon_size        = DEFAULT_ICON_SIZE;
}

static void
lockkeys_save (XfcePanelPlugin *plugin,
               LockKeysPlugin  *lk)
{
    gchar  *path = xfce_panel_plugin_save_location (plugin, TRUE);
    XfceRc *rc;

    if (!path) return;
    rc = xfce_rc_simple_open (path, FALSE);
    g_free (path);
    if (!rc) return;

    xfce_rc_write_bool_entry (rc, "show_caps",     lk->show_caps);
    xfce_rc_write_bool_entry (rc, "show_num",      lk->show_num);
    xfce_rc_write_bool_entry (rc, "hide_inactive",    lk->hide_inactive);
    xfce_rc_write_bool_entry (rc, "notifications",    lk->notifications);
    xfce_rc_write_bool_entry (rc, "manual_icon_size", lk->manual_icon_size);
    xfce_rc_write_int_entry  (rc, "icon_size",        lk->icon_size);
    xfce_rc_close (rc);
}


/* ── Configuration dialog ────────────────────────────────────────────────────── */

static void on_show_caps_toggled (GtkToggleButton *b, LockKeysPlugin *lk)
{
    lk->show_caps = gtk_toggle_button_get_active (b);
    lockkeys_rebuild_ui (lk);
}

static void on_show_num_toggled (GtkToggleButton *b, LockKeysPlugin *lk)
{
    lk->show_num = gtk_toggle_button_get_active (b);
    lockkeys_rebuild_ui (lk);
}

static void on_notifications_toggled (GtkToggleButton *b, LockKeysPlugin *lk)
{
    lk->notifications = gtk_toggle_button_get_active (b);
}

static void on_hide_inactive_toggled (GtkToggleButton *b, LockKeysPlugin *lk)
{
    lk->hide_inactive = gtk_toggle_button_get_active (b);
    /* Force redraw to apply immediately */
    lk->caps_state = -1;
    lk->num_state  = -1;
    lockkeys_update (lk);
}

static void on_manual_icon_size_toggled (GtkToggleButton *b, LockKeysPlugin *lk)
{
    lk->manual_icon_size = gtk_toggle_button_get_active (b);
    if (!lk->manual_icon_size)
    {
        lk->icon_size  = xfce_panel_plugin_get_icon_size (lk->plugin);
        lk->caps_state = -1;
        lk->num_state  = -1;
        lockkeys_update (lk);
    }
}

static void on_icon_size_changed (GtkSpinButton *spin, LockKeysPlugin *lk)
{
    lk->icon_size  = (gint) gtk_spin_button_get_value (spin);
    lk->caps_state = -1;
    lk->num_state  = -1;
    lockkeys_update (lk);
}

static void
lockkeys_dialog_response (GtkDialog       *dialog,
                          gint             response,
                          XfcePanelPlugin *plugin)
{
    if (response == GTK_RESPONSE_HELP)
    {
        GtkWidget *about = gtk_about_dialog_new ();
        gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (about), "xfce4-lockkeys-plugin");
        gtk_about_dialog_set_version      (GTK_ABOUT_DIALOG (about), "1.0.0");
        gtk_about_dialog_set_comments     (GTK_ABOUT_DIALOG (about),
            "Shows the state of Caps Lock and Num Lock in the Xfce panel.\n"
            "Similar to KDE Plasma's Lock Keys State applet.");
        gtk_about_dialog_set_website      (GTK_ABOUT_DIALOG (about),
            "https://github.com/Tantin1/xfce4-lockkeys-plugin");
        gtk_about_dialog_set_logo_icon_name (GTK_ABOUT_DIALOG (about),
            "preferences-desktop-keyboard");
        gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW (dialog));
        gtk_dialog_run (GTK_DIALOG (about));
        gtk_widget_destroy (about);
        return;
    }
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
lockkeys_configure_plugin (XfcePanelPlugin *plugin,
                            LockKeysPlugin  *lk)
{
    GtkWidget *dialog, *content, *vbox;
    GtkWidget *label, *caps_chk, *num_chk, *hide_chk;
    GtkWidget *size_box, *size_label, *size_spin;

    xfce_panel_plugin_block_menu (plugin);

    dialog = gtk_dialog_new_with_buttons (
        _("Lock Keys Plugin"),
        GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        _("About"), GTK_RESPONSE_HELP,
        _("Close"), GTK_RESPONSE_OK,
        NULL);

    gtk_window_set_position  (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "preferences-desktop-keyboard");
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
    gtk_box_pack_start (GTK_BOX (content), vbox, TRUE, TRUE, 0);

    /* ── Indicators section ── */
    label = gtk_label_new (_("Indicators to display:"));
    gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    caps_chk = gtk_check_button_new_with_mnemonic (_("Show _Caps Lock indicator"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (caps_chk), lk->show_caps);
    g_signal_connect (caps_chk, "toggled", G_CALLBACK (on_show_caps_toggled), lk);
    gtk_box_pack_start (GTK_BOX (vbox), caps_chk, FALSE, FALSE, 0);

    num_chk = gtk_check_button_new_with_mnemonic (_("Show _Num Lock indicator"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (num_chk), lk->show_num);
    g_signal_connect (num_chk, "toggled", G_CALLBACK (on_show_num_toggled), lk);
    gtk_box_pack_start (GTK_BOX (vbox), num_chk, FALSE, FALSE, 0);

    /* ── Separator ── */
    gtk_box_pack_start (GTK_BOX (vbox),
                        gtk_separator_new (GTK_ORIENTATION_HORIZONTAL),
                        FALSE, FALSE, 2);

    /* ── Behaviour section ── */
    label = gtk_label_new (_("Behaviour:"));
    gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    hide_chk = gtk_check_button_new_with_mnemonic (
        _("_Hide icon when lock key is inactive"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (hide_chk), lk->hide_inactive);
    gtk_widget_set_tooltip_text (hide_chk,
        _("When enabled, each icon is hidden while the corresponding lock key is off. "
          "If both are off, the plugin becomes invisible."));
    g_signal_connect (hide_chk, "toggled", G_CALLBACK (on_hide_inactive_toggled), lk);
    gtk_box_pack_start (GTK_BOX (vbox), hide_chk, FALSE, FALSE, 0);

    GtkWidget *notif_chk;
    notif_chk = gtk_check_button_new_with_mnemonic (_("Show _notifications on lock key change"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (notif_chk), lk->notifications);
    g_signal_connect (notif_chk, "toggled", G_CALLBACK (on_notifications_toggled), lk);
    gtk_box_pack_start (GTK_BOX (vbox), notif_chk, FALSE, FALSE, 0);

    /* ── Separator ── */
    gtk_box_pack_start (GTK_BOX (vbox),
                        gtk_separator_new (GTK_ORIENTATION_HORIZONTAL),
                        FALSE, FALSE, 2);

    /* ── Icon size section ── */
    GtkWidget *manual_chk;
    manual_chk = gtk_check_button_new_with_mnemonic (_("_Manual icon size (px):"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (manual_chk), lk->manual_icon_size);
    gtk_widget_set_tooltip_text (manual_chk,
        _("When disabled, the icon size follows the panel configuration."));
    g_signal_connect (manual_chk, "toggled",
                      G_CALLBACK (on_manual_icon_size_toggled), lk);
    gtk_box_pack_start (GTK_BOX (vbox), manual_chk, FALSE, FALSE, 0);

    size_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start (GTK_BOX (vbox), size_box, FALSE, FALSE, 0);

    size_label = gtk_label_new (_("    Size (px):"));
    gtk_label_set_xalign (GTK_LABEL (size_label), 0.0f);
    gtk_box_pack_start (GTK_BOX (size_box), size_label, TRUE, TRUE, 0);

    size_spin = gtk_spin_button_new_with_range (8, 128, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (size_spin), lk->icon_size);
    gtk_widget_set_sensitive (size_spin, lk->manual_icon_size);
    gtk_widget_set_tooltip_text (size_spin,
        _("Icon size in pixels. Only active when manual size is enabled."));
    g_signal_connect (size_spin, "value-changed",
                      G_CALLBACK (on_icon_size_changed), lk);
    gtk_box_pack_start (GTK_BOX (size_box), size_spin, FALSE, FALSE, 0);

    /* Vincular sensibilidad del spinner al checkbox */
    g_object_bind_property (manual_chk, "active", size_spin, "sensitive",
                            G_BINDING_SYNC_CREATE);

    /* ── Close / About ── */
    g_signal_connect (dialog, "response",
                      G_CALLBACK (lockkeys_dialog_response), plugin);
    g_signal_connect_swapped (dialog, "destroy",
                               G_CALLBACK (xfce_panel_plugin_unblock_menu), plugin);

    gtk_widget_show_all (dialog);
}


/* ── Panel callbacks ────────────────────────────────────────────────────────── */

static void
lockkeys_free (XfcePanelPlugin *plugin,
               LockKeysPlugin  *lk)
{
    gdk_window_remove_filter (NULL, lockkeys_xkb_filter, lk);

    if (lk->poll_id)
        g_source_remove (lk->poll_id);

    gtk_widget_destroy (lk->box);
    g_slice_free (LockKeysPlugin, lk);
}

static gboolean
lockkeys_size_changed (XfcePanelPlugin *plugin,
                       gint             size,
                       LockKeysPlugin  *lk)
{
    (void) size;
    if (!lk->manual_icon_size)
    {
        lk->icon_size  = xfce_panel_plugin_get_icon_size (plugin);
        lk->caps_state = -1;
        lk->num_state  = -1;
        lockkeys_update (lk);
    }
    return TRUE;
}
static void
lockkeys_orientation_changed (XfcePanelPlugin *plugin,
                               GtkOrientation   orientation,
                               LockKeysPlugin  *lk)
{
    gtk_orientable_set_orientation (GTK_ORIENTABLE (lk->box), orientation);
}


/* ── Startup redraw workaround ─────────────────────────────────────────────── */

static gboolean
lockkeys_force_redraw (LockKeysPlugin *lk)
{
    /* Toggle hide_inactive twice to force a full redraw cycle */
    lk->hide_inactive = !lk->hide_inactive;
    lk->caps_state = -1;
    lk->num_state  = -1;
    lockkeys_update (lk);

    lk->hide_inactive = !lk->hide_inactive;
    lk->caps_state = -1;
    lk->num_state  = -1;
    lockkeys_update (lk);

    return G_SOURCE_REMOVE;  /* one-shot */
}


/* ── Plugin entry point ─────────────────────────────────────────────────────── */

static void
lockkeys_construct (XfcePanelPlugin *plugin)
{
    LockKeysPlugin *lk;
    Display        *dpy;
    int             op, er, maj = XkbMajorVersion, min = XkbMinorVersion;

    dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    if (!XkbQueryExtension (dpy, &op, NULL, &er, &maj, &min))
    {
        g_warning ("xfce4-lockkeys-plugin: XKB extension not available");
        return;
    }

    /* GDK ya suscribe eventos XKB internamente */

    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    lk = g_slice_new0 (LockKeysPlugin);
    lk->plugin     = plugin;
    lk->caps_state = -1;
    lk->num_state  = -1;

    lockkeys_read_settings (lk);

    lk->box = gtk_box_new (xfce_panel_plugin_get_orientation (plugin), 0);
    gtk_container_add (GTK_CONTAINER (plugin), lk->box);

    lockkeys_rebuild_ui (lk);

    /* Workaround: toggle hide_inactive twice after a short delay to
     * force a full redraw on startup. Without this, the icons may
     * appear empty when both lock keys are off at boot time. */
    
    gdk_window_add_filter (NULL, lockkeys_xkb_filter, lk);
    g_timeout_add (150, (GSourceFunc) lockkeys_force_redraw, lk);
    lk->poll_id = 0;

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (lockkeys_free), lk);
    g_signal_connect (plugin, "save",
                      G_CALLBACK (lockkeys_save), lk);
    g_signal_connect (plugin, "size-changed",
                      G_CALLBACK (lockkeys_size_changed), lk);
    g_signal_connect (plugin, "orientation-changed",
                      G_CALLBACK (lockkeys_orientation_changed), lk);
    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (lockkeys_configure_plugin), lk);

    xfce_panel_plugin_menu_show_configure (plugin);

    gtk_widget_show_all (GTK_WIDGET (plugin));
}
