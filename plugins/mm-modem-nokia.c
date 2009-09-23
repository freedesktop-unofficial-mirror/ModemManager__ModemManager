/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2008 - 2009 Novell, Inc.
 * Copyright (C) 2009 Red Hat, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mm-modem-nokia.h"
#include "mm-serial-parsers.h"

static gpointer mm_modem_nokia_parent_class = NULL;

MMModem *
mm_modem_nokia_new (const char *device,
                    const char *driver,
                    const char *plugin)
{
    g_return_val_if_fail (device != NULL, NULL);
    g_return_val_if_fail (driver != NULL, NULL);
    g_return_val_if_fail (plugin != NULL, NULL);

    return MM_MODEM (g_object_new (MM_TYPE_MODEM_NOKIA,
                                   MM_MODEM_MASTER_DEVICE, device,
                                   MM_MODEM_DRIVER, driver,
                                   MM_MODEM_PLUGIN, plugin,
                                   NULL));
}

static gboolean
grab_port (MMModem *modem,
           const char *subsys,
           const char *name,
           MMPortType suggested_type,
           gpointer user_data,
           GError **error)
{
    MMGenericGsm *gsm = MM_GENERIC_GSM (modem);
    MMPortType ptype = MM_PORT_TYPE_IGNORED;
    MMPort *port = NULL;

    if (suggested_type == MM_PORT_TYPE_UNKNOWN) {
        if (!mm_generic_gsm_get_port (gsm, MM_PORT_TYPE_PRIMARY))
                ptype = MM_PORT_TYPE_PRIMARY;
        else if (!mm_generic_gsm_get_port (gsm, MM_PORT_TYPE_SECONDARY))
            ptype = MM_PORT_TYPE_SECONDARY;
    } else
        ptype = suggested_type;

    port = mm_generic_gsm_grab_port (gsm, subsys, name, ptype, error);
    if (port && MM_IS_SERIAL_PORT (port)) {
        mm_serial_port_set_response_parser (MM_SERIAL_PORT (port),
                                            mm_serial_parser_v1_e1_parse,
                                            mm_serial_parser_v1_e1_new (),
                                            mm_serial_parser_v1_e1_destroy);
    }

    return !!port;
}

/*****************************************************************************/

static void
modem_init (MMModem *modem_class)
{
    modem_class->grab_port = grab_port;
}

static void
mm_modem_nokia_init (MMModemNokia *self)
{
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
    /* Nokia headsets (at least N85) do not support "power on"; they do
     * support "power off" but you proabably do not want to turn off the
     * power on your telephone if something went wrong with connecting
     * process. So, disabling both these operations.  The Nokia GSM/UMTS command
     * reference v1.2 also states that only CFUN=0 (turn off but still charge)
     * and CFUN=1 (full functionality) are supported, and since the phone has
     * to be in CFUN=1 before we'll be able to talk to it in the first place,
     * we shouldn't bother with CFUN at all.
     */
    switch (prop_id) {
    case MM_GENERIC_GSM_PROP_POWER_UP_CMD:
        g_value_set_string (value, "");
        break;
    case MM_GENERIC_GSM_PROP_POWER_DOWN_CMD:
        g_value_set_string (value, "");
        break;
    default:
        break;
    }
}

static void
mm_modem_nokia_class_init (MMModemNokiaClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    mm_modem_nokia_parent_class = g_type_class_peek_parent (klass);

    object_class->get_property = get_property;

    g_object_class_override_property (object_class,
                                      MM_GENERIC_GSM_PROP_POWER_UP_CMD,
                                      MM_GENERIC_GSM_POWER_UP_CMD);

    g_object_class_override_property (object_class,
                                      MM_GENERIC_GSM_PROP_POWER_DOWN_CMD,
                                      MM_GENERIC_GSM_POWER_DOWN_CMD);
}

GType
mm_modem_nokia_get_type (void)
{
    static GType modem_nokia_type = 0;

    if (G_UNLIKELY (modem_nokia_type == 0)) {
        static const GTypeInfo modem_nokia_type_info = {
            sizeof (MMModemNokiaClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) mm_modem_nokia_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,   /* class_data */
            sizeof (MMModemNokia),
            0,      /* n_preallocs */
            (GInstanceInitFunc) mm_modem_nokia_init,
        };

        static const GInterfaceInfo modem_iface_info = { 
            (GInterfaceInitFunc) modem_init
        };

        modem_nokia_type = g_type_register_static (MM_TYPE_GENERIC_GSM, "MMModemNokia", &modem_nokia_type_info, 0);

        g_type_add_interface_static (modem_nokia_type, MM_TYPE_MODEM, &modem_iface_info);
    }

    return modem_nokia_type;
}
