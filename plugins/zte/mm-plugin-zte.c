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
 * Copyright (C) 2009 - 2012 Red Hat, Inc.
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#include <string.h>
#include <gmodule.h>

#include <libmm-common.h>

#include "mm-log.h"
#include "mm-plugin-zte.h"
#include "mm-broadband-modem.h"

G_DEFINE_TYPE (MMPluginZte, mm_plugin_zte, MM_TYPE_PLUGIN)

int mm_plugin_major_version = MM_PLUGIN_MAJOR_VERSION;
int mm_plugin_minor_version = MM_PLUGIN_MINOR_VERSION;

/*****************************************************************************/
/* Custom commands for AT probing */

/* Many ZTE devices will flood the port with "Message waiting" indications
 * and eventually fill up the serial buffer and crash.  We need to turn off
 * that indicator.  See NetworkManager commits
 * 	1235f71b20c92cded4abd976ccc5010649aae1a0 and
 * 	f38ad328acfdc6ce29dd1380602c546b064161ae for more details.
 *
 * We use this command also for checking AT support in the port.
 */
static const MMPortProbeAtCommand custom_at_probe[] = {
    { "ATE0+CPMS?", 3, mm_port_probe_response_processor_is_at },
    { "ATE0+CPMS?", 3, mm_port_probe_response_processor_is_at },
    { "ATE0+CPMS?", 3, mm_port_probe_response_processor_is_at },
    { NULL }
};

/*****************************************************************************/

static MMBaseModem *
create_modem (MMPlugin *self,
              const gchar *sysfs_path,
              const gchar *driver,
              guint16 vendor,
              guint16 product,
              GList *probes,
              GError **error)
{
    return MM_BASE_MODEM (mm_broadband_modem_new (sysfs_path,
                                                  driver,
                                                  mm_plugin_get_name (self),
                                                  vendor,
                                                  product));
}

static gboolean
grab_port (MMPlugin *self,
           MMBaseModem *modem,
           MMPortProbe *probe,
           GError **error)
{
    GUdevDevice *port;
    MMAtPortFlag pflags = MM_AT_PORT_FLAG_NONE;

    port = mm_port_probe_peek_port (probe);

    if (mm_port_probe_is_at (probe)) {
        /* Look for port type hints */
        if (g_udev_device_get_property_as_boolean (port, "ID_MM_ZTE_PORT_TYPE_MODEM"))
            pflags = MM_AT_PORT_FLAG_PRIMARY;
        else if (g_udev_device_get_property_as_boolean (port, "ID_MM_ZTE_PORT_TYPE_AUX"))
            pflags = MM_AT_PORT_FLAG_SECONDARY;
    }

    return mm_base_modem_grab_port (modem,
                                    mm_port_probe_get_port_subsys (probe),
                                    mm_port_probe_get_port_name (probe),
                                    mm_port_probe_get_port_type (probe),
                                    pflags,
                                    error);
}

/*****************************************************************************/

G_MODULE_EXPORT MMPlugin *
mm_plugin_create (void)
{
    static const gchar *subsystems[] = { "tty", "net", NULL };
    static const guint16 vendor_ids[] = { 0x19d2, 0 };

    return MM_PLUGIN (
        g_object_new (MM_TYPE_PLUGIN_ZTE,
                      MM_PLUGIN_NAME,               "ZTE",
                      MM_PLUGIN_ALLOWED_SUBSYSTEMS, subsystems,
                      MM_PLUGIN_ALLOWED_VENDOR_IDS, vendor_ids,
                      MM_PLUGIN_CUSTOM_AT_PROBE,    custom_at_probe,
                      MM_PLUGIN_ALLOWED_AT,         TRUE,
                      MM_PLUGIN_ALLOWED_QCDM,       TRUE,
                      NULL));
}

static void
mm_plugin_zte_init (MMPluginZte *self)
{
}

static void
mm_plugin_zte_class_init (MMPluginZteClass *klass)
{
    MMPluginClass *plugin_class = MM_PLUGIN_CLASS (klass);

    plugin_class->create_modem = create_modem;
    plugin_class->grab_port = grab_port;
}