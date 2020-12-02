// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdio.h>

#include <btstack.h>
#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include "../../drv/bluetooth/bluetooth_btstack_run_loop_contiki.h"
#include "../../drv/core.h"

void pbdrv_bluetooth_init() {
    btstack_run_loop_init(pbdrv_bluetooth_btstack_run_loop_contiki_get_instance());
}

static void handle_timer_timeout(btstack_timer_source_t *ts) {
    uint32_t *callback_count = ts->context;
    (*callback_count)++;
}

PT_THREAD(test_btstack_run_loop_contiki_timer(struct pt *pt)) {
    static btstack_timer_source_t timer_source, timer_source_2, timer_source_3;
    static uint32_t callback_count, callback_count_2, callback_count_3;

    PT_BEGIN(pt);

    // common btstack timer init
    btstack_run_loop_set_timer_handler(&timer_source, handle_timer_timeout);
    btstack_run_loop_set_timer_handler(&timer_source_2, handle_timer_timeout);
    btstack_run_loop_set_timer_handler(&timer_source_3, handle_timer_timeout);
    btstack_run_loop_set_timer_context(&timer_source, &callback_count);
    btstack_run_loop_set_timer_context(&timer_source_2, &callback_count_2);
    btstack_run_loop_set_timer_context(&timer_source_3, &callback_count_3);

    // -- test single timer callback --

    // init and schedule
    callback_count = 0;
    btstack_run_loop_set_timer(&timer_source, 10);
    btstack_run_loop_add_timer(&timer_source);

    // should not expire early
    clock_tick(9);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    // now it should be done
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);


    // -- timers scheduled out of order should fire in order --

    callback_count = callback_count_2 = callback_count_3 = 0;
    btstack_run_loop_set_timer(&timer_source, 10);
    btstack_run_loop_set_timer(&timer_source_2, 5);
    btstack_run_loop_set_timer(&timer_source_3, 15);
    btstack_run_loop_add_timer(&timer_source);
    btstack_run_loop_add_timer(&timer_source_2);
    btstack_run_loop_add_timer(&timer_source_3);

    // should not expire early
    clock_tick(4);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    tt_want_uint_op(callback_count_2, ==, 0);
    tt_want_uint_op(callback_count_3, ==, 0);

    // only timer 2 should be called back after 5 ms
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 0);

    // then timer 1 after 10 ms
    clock_tick(5);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 0);

    // and finally timer 3
    clock_tick(5);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 1);


    // -- timers with same timeout should all call back at the same time

    callback_count = callback_count_2 = callback_count_3 = 0;
    btstack_run_loop_set_timer(&timer_source, 15);
    btstack_run_loop_add_timer(&timer_source);

    clock_tick(5);
    PT_YIELD(pt);

    btstack_run_loop_set_timer(&timer_source_2, 10);
    btstack_run_loop_add_timer(&timer_source_2);

    clock_tick(5);
    PT_YIELD(pt);

    btstack_run_loop_set_timer(&timer_source_3, 5);
    btstack_run_loop_add_timer(&timer_source_3);

    // none should have timeout out yet
    clock_tick(4);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    tt_want_uint_op(callback_count_2, ==, 0);
    tt_want_uint_op(callback_count_3, ==, 0);

    // then all at the same time
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 1);


    // -- should be able to cancel a timer --

    // init and schedule
    callback_count = 0;
    btstack_run_loop_set_timer_handler(&timer_source, handle_timer_timeout);
    btstack_run_loop_set_timer_context(&timer_source, &callback_count);
    btstack_run_loop_set_timer(&timer_source, 10);
    btstack_run_loop_add_timer(&timer_source);

    // should not expire early
    clock_tick(9);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    btstack_run_loop_remove_timer(&timer_source);

    // it should have been canceled
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    PT_END(pt);
}

static void handle_data_source(btstack_data_source_t *ds,  btstack_data_source_callback_type_t callback_type) {
    uint32_t *callback_count = ds->source.handle;

    switch (callback_type) {
        case DATA_SOURCE_CALLBACK_POLL:
            (*callback_count)++;
            break;
        default:
            break;
    }
}

PT_THREAD(test_btstack_run_loop_contiki_poll(struct pt *pt)) {
    static btstack_data_source_t data_source;
    static uint32_t callback_count;

    PT_BEGIN(pt);

    callback_count = 0;
    btstack_run_loop_set_data_source_handle(&data_source, &callback_count);
    btstack_run_loop_set_data_source_handler(&data_source, &handle_data_source);
    btstack_run_loop_enable_data_source_callbacks(&data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&data_source);

    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);

    callback_count = 0;
    btstack_run_loop_disable_data_source_callbacks(&data_source, DATA_SOURCE_CALLBACK_POLL);
    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    callback_count = 0;
    btstack_run_loop_enable_data_source_callbacks(&data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_remove_data_source(&data_source);
    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    PT_END(pt);
}
