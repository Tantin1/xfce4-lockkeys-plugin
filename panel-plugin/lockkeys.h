/*
 * xfce4-lockkeys-plugin – Lock Keys State Plugin for the Xfce Panel
 * Copyright (C) 2024 – Licensed under GNU LGPL v2.1 or later.
 */

#ifndef __LOCKKEYS_H__
#define __LOCKKEYS_H__

#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _LockKeysPlugin LockKeysPlugin;

struct _LockKeysPlugin
{
    XfcePanelPlugin *plugin;

    GtkWidget *box;

    GtkWidget *caps_box;
    GtkWidget *caps_image;

    GtkWidget *num_box;
    GtkWidget *num_image;

    /* Settings */
    gboolean   show_caps;
    gboolean   show_num;
    gboolean   hide_inactive;      /* hide each icon when its lock key is off */
    gboolean   notifications;      /* TRUE = mostrar notificación al cambiar estado */
    gboolean   manual_icon_size;   /* TRUE = usar icon_size propio; FALSE = seguir al panel */
    gint       icon_size;

    /* State: -1 = uninitialized, 0 = off, 1 = on */
    gint       caps_state;
    gint       num_state;

    guint      poll_id;        /* fallback timer (solo arranque) */
};

G_END_DECLS

#endif /* __LOCKKEYS_H__ */
