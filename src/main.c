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
#include <string.h>
#include "os/mynewt.h"
#include "bsp/bsp.h"
#include "console/console.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"

/* Mandatory services. */
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

/* Application-specified header. */
#include "blepadv.h"

#define BLEPADV_MAIN_TASK_PRIO 0xf0

static int blepadv_gap_event(struct ble_gap_event *event, void *arg);

/* Define stack, object and semaphore for central main task. */
#define BLEPADV_MAIN_TASK_STACK_SIZE (128)
static struct os_task blepadv_main_task;
static os_stack_t blepadv_main_task_stack[BLEPADV_MAIN_TASK_STACK_SIZE];
static void blepadv_main_task_fn(void *arg);
static struct os_sem blepadv_main_sem;

static void
blepadv_start_periodic(void)
{
    int rc;
    uint8_t instance = 0;
    struct ble_gap_periodic_adv_params pparams;
    struct ble_gap_ext_adv_params params;
    struct os_mbuf *data;
    struct ble_hs_adv_fields adv_fields;

    /*
     * Extended advertising.
     */

    /* For periodic we use instance with non-connectable advertising */
    memset(&params, 0, sizeof(params));

    /* advertise using public addr at 1 sec interval */
    params.own_addr_type = BLE_OWN_ADDR_PUBLIC;
    params.itvl_min = 1600;
    params.itvl_max = 1600;
    params.primary_phy = BLE_HCI_LE_PHY_1M;
    params.secondary_phy = BLE_HCI_LE_PHY_1M;
    params.tx_power = 0;
    params.sid = 0;

    /* configure extended advertising (required by periodic advertising) */
    rc = ble_gap_ext_adv_configure(instance, &params, NULL, blepadv_gap_event,
                                   NULL);
    assert(rc == 0);

    /* Use defaults for non-set fields */
    memset(&adv_fields, 0, sizeof adv_fields);
    
    /* General Discoverable and BrEdrNotSupported */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Default to legacy PDUs size, mbuf chain will be increased if needed */
    data = os_msys_get_pkthdr(BLE_HCI_MAX_ADV_DATA_LEN, 0);
    assert(data);

    rc = ble_hs_adv_set_fields_mbuf(&adv_fields, data);
    assert(rc == 0);

    /* Set adv. data (just the flags AD type) */
    rc = ble_gap_ext_adv_set_data(instance, data);
    assert(rc == 0);

    /* configure periodic advertising with 100 ms adv. interval */
    memset(&pparams, 0, sizeof(pparams));
    pparams.include_tx_power = 0;
    pparams.itvl_min = 80;
    pparams.itvl_max = 80;

    /*
     * Periodic advertising. No payload.
     */
    rc = ble_gap_periodic_adv_configure(instance, &pparams);
    assert(rc == 0);

    /* start periodic advertising */
    rc = ble_gap_periodic_adv_start(instance);
    assert (rc == 0);

    /* start advertising */
    rc = ble_gap_ext_adv_start(instance, 0, 0);
    assert (rc == 0);

    console_printf("Instance %u started (periodic)\n", instance);
}

static int
blepadv_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_ADV_COMPLETE:
        console_printf("Adv. complete, instance %d reason %d,\n",
                       event->adv_complete.instance,
                       event->adv_complete.reason);
        return 0;
    default:
        console_printf("Event %d not handled\n", event->type);
        return 0;
    }
}

static void
blepadv_on_reset(int reason)
{
    console_printf("Resetting state; reason=%d\n", reason);
}

static void
blepadv_on_sync(void)
{
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Create the BLE Central main task */
    rc = os_task_init(&blepadv_main_task, "blepadv_main_task",
                      blepadv_main_task_fn, NULL, BLEPADV_MAIN_TASK_PRIO,
                      OS_WAIT_FOREVER, blepadv_main_task_stack,
                      BLEPADV_MAIN_TASK_STACK_SIZE);

    assert(rc == 0);
}

static void
blepadv_main_task_fn(void *arg)
{
    int rc;

    console_printf("BLE Periodic Advertiser main task welcomes you on-board\n");
    
    /* Init semaphore with 0 tokens. */
    rc = os_sem_init(&blepadv_main_sem, 0);
    assert(rc == 0);

    /* Configure and start periodic advertising */
    blepadv_start_periodic();

    /* Just a test... */
    os_sem_pend(&blepadv_main_sem, OS_TICKS_PER_SEC/2);

    console_printf("Entering infinite loop\n");

    /* Task should never return */
    while (1) {
        /* Delay used only to prevent watchdog to reset the device. */
        os_time_delay(os_time_ms_to_ticks32(2000));
    }
}

/**
 * main
 *
 * All application logic and NimBLE host work is performed in default task.
 *
 * @return int NOTE: this function should never return!
 */
static int
main_fn(int argc, char **argv)
{
    /* Initialize OS */
    sysinit();

    console_printf("Hello, BLE periodic advertiser!\n");

    /* Configure the host. */
    ble_hs_cfg.reset_cb = blepadv_on_reset;
    ble_hs_cfg.sync_cb = blepadv_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* os start should never return. If it does, this should be an error */
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }

    return 0;
}

int
main(int argc, char **argv)
{
#if BABBLESIM
    extern void bsim_init(int argc, char** argv, void *main_fn);
    bsim_init(argc, argv, main_fn);
#else
    main_fn(argc, argv);
#endif

    return 0;
}
