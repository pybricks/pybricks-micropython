// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Contiki run loop integration for BlueKitchen BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <btstack.h>
#include <contiki.h>

PROCESS(btstack_run_loop_contiki_process, "btstack");
static struct etimer timer;
static btstack_linked_list_t timers;
static btstack_linked_list_t data_sources;

/**
 * Schedules event timer for the earliest timeout, if any.
 */
static void schedule_timer(void) {
    btstack_timer_source_t *first_timer = (void *)timers;

    PROCESS_CONTEXT_BEGIN(&btstack_run_loop_contiki_process);

    if (first_timer == NULL) {
        etimer_stop(&timer);
    } else {
        etimer_set(&timer, clock_from_msec(first_timer->timeout - clock_to_msec(clock_time())));
    }

    PROCESS_CONTEXT_END();
}

static void btstack_run_loop_contiki_init(void) {
    process_start(&btstack_run_loop_contiki_process, NULL);
}

static void btstack_run_loop_contiki_add_data_source(btstack_data_source_t *ds) {
    btstack_linked_list_add(&data_sources, &ds->item);
}

static bool btstack_run_loop_contiki_remove_data_source(btstack_data_source_t *ds) {
    return btstack_linked_list_remove(&data_sources, &ds->item);
}

static void btstack_run_loop_contiki_enable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callback_types) {
    ds->flags |= callback_types;
}

static void btstack_run_loop_contiki_disable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callback_types) {
    ds->flags &= ~callback_types;
}

static void btstack_run_loop_contiki_set_timer(btstack_timer_source_t *ts, uint32_t timeout_in_ms) {
    ts->timeout = clock_to_msec(clock_time()) + timeout_in_ms;
}

static void btstack_run_loop_contiki_add_timer(btstack_timer_source_t *ts) {
    btstack_linked_item_t *it;
    for (it = (void *)&timers; it->next; it = it->next) {
        // don't add timer that's already in there
        btstack_timer_source_t *next = (void *)it->next;
        if (next == ts) {
            // timer was already in the list!
            assert(0);
            return;
        }
        // exit if new timeout before list timeout
        int32_t delta = btstack_time_delta(ts->timeout, next->timeout);
        if (delta < 0) {
            break;
        }
    }

    ts->item.next = it->next;
    it->next = &ts->item;

    schedule_timer();
}

static bool btstack_run_loop_contiki_remove_timer(btstack_timer_source_t *ts) {
    if (btstack_linked_list_remove(&timers, &ts->item)) {
        schedule_timer();
        return true;
    }
    return false;
}

static void btstack_run_loop_contiki_execute(void) {
    // not used
}

static void btstack_run_loop_contiki_dump_timer(void) {
    // not used
}

static uint32_t btstack_run_loop_contiki_get_time_ms(void) {
    return clock_to_msec(clock_time());
}

static const btstack_run_loop_t btstack_run_loop_contiki = {
    .init = btstack_run_loop_contiki_init,
    .add_data_source = btstack_run_loop_contiki_add_data_source,
    .remove_data_source = btstack_run_loop_contiki_remove_data_source,
    .enable_data_source_callbacks = btstack_run_loop_contiki_enable_data_source_callbacks,
    .disable_data_source_callbacks = btstack_run_loop_contiki_disable_data_source_callbacks,
    .set_timer = btstack_run_loop_contiki_set_timer,
    .add_timer = btstack_run_loop_contiki_add_timer,
    .remove_timer = btstack_run_loop_contiki_remove_timer,
    .execute = btstack_run_loop_contiki_execute,
    .dump_timer = btstack_run_loop_contiki_dump_timer,
    .get_time_ms = btstack_run_loop_contiki_get_time_ms,
};

/**
 * Gets the BTStack Contiki run loop.
 */
const btstack_run_loop_t *pbdrv_bluetooth_btstack_run_loop_contiki_get_instance(void) {
    return &btstack_run_loop_contiki;
}

/**
 * Polls the BTStack Contiki process.
 */
void pbdrv_bluetooth_btstack_run_loop_contiki_trigger(void) {
    process_poll(&btstack_run_loop_contiki_process);
}

static void btstack_run_loop_contiki_poll_handler(void) {
    btstack_data_source_t *ds, *next;
    for (ds = (void *)data_sources; ds != NULL; ds = next) {
        // cache pointer to next data_source to allow data source to remove itself
        next = (void *)ds->item.next;
        if (ds->flags & DATA_SOURCE_CALLBACK_POLL) {
            ds->process(ds, DATA_SOURCE_CALLBACK_POLL);
        }
    }
}

PROCESS_THREAD(btstack_run_loop_contiki_process, ev, data) {
    PROCESS_POLLHANDLER(btstack_run_loop_contiki_poll_handler());

    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        // process all BTStack timers in list that have expired
        while (timers) {
            btstack_timer_source_t *ts = (void *)timers;
            int32_t delta = btstack_time_delta(ts->timeout, clock_to_msec(clock_time()));
            if (delta > 0) {
                // we have reached unexpired timers
                break;
            }

            // note: calling this function also reschedules contiki etimer
            btstack_run_loop_contiki_remove_timer(ts);
            ts->process(ts);
        }
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
