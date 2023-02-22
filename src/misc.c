/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "blepadv.h"

/**
 * Utility function to log an array of bytes.
 */
void
print_bytes(const uint8_t *bytes, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        MODLOG_DFLT(DEBUG, "%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
}

void
print_mbuf(const struct os_mbuf *om)
{
    int colon;

    colon = 0;
    while (om != NULL) {
        if (colon) {
            MODLOG_DFLT(DEBUG, ":");
        } else {
            colon = 1;
        }
        print_bytes(om->om_data, om->om_len);
        om = SLIST_NEXT(om, om_next);
    }
}

char *
addr_str(const void *addr)
{
    static char buf[6 * 2 + 5 + 1];
    const uint8_t *u8p;

    u8p = addr;
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);

    return buf;
}
