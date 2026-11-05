/* SPDX-FileCopyrightText: 2026 Core Devices LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#include <pbl/drivers/rtc.h>

#include <applib/ui/layer.h>
#include <applib/ui/recognizer/recognizer_manager.h>
#include <applib/ui/window.h>
#include <clar.h>
#include <shell/normal/watchface.h>

// Fakes
#include <fake_rtc.h>
#include <stubs_app_install_manager.h>

// Stubs
#include <pbl/util/units.h>

// ---------------------------------------------------------------------------------------------
// Collaborators. The watchface manager has no window: only its task-global swipe list runs.

RecognizerList *window_get_recognizer_list(Window *window) {
  return nullptr;
}

RecognizerManager *window_get_recognizer_manager(Window *window) {
  return nullptr;
}

struct Layer *window_get_root_layer(const Window *window) {
  if (!window) {
    return nullptr;
  }
  return &((Window *)window)->layer;
}

Layer *layer_find_layer_containing_point(const Layer *node, const GPoint *point) {
  return nullptr;
}

static bool s_animating;
static int s_launch_count;

bool compositor_is_animating(void) {
  return s_animating;
}

void watchface_swipe_open_launcher(void) {
  s_launch_count++;
}

// ---------------------------------------------------------------------------------------------
// Helpers

static void prv_event(TouchEventType type, int16_t x, int16_t y, bool gated) {
  const TouchEvent e = {.type = type, .x = x, .y = y, .non_navigational = gated};
  watchface_handle_touch_event(&e);
}

static void prv_advance_ms(uint32_t ms) {
  fake_rtc_increment_ticks((RtcTicks)ms * RTC_TICKS_HZ / PBL_MSEC_PER_SEC);
}

// A fast, straight horizontal flick; the gate latch rides on every event as the kernel stamps it.
static void prv_swipe_gated(int16_t sx, int16_t ex, int16_t y, bool gated) {
  prv_event(TouchEvent_Touchdown, sx, y, gated);
  prv_advance_ms(20);
  prv_event(TouchEvent_PositionUpdate, (int16_t)((sx + ex) / 2), y, gated);
  prv_advance_ms(20);
  prv_event(TouchEvent_PositionUpdate, ex, y, gated);
  prv_advance_ms(20);
  prv_event(TouchEvent_Liftoff, ex, y, gated);
  prv_advance_ms(100);
}

static void prv_swipe(int16_t sx, int16_t ex, int16_t y) {
  prv_swipe_gated(sx, ex, y, false);
}

// ---------------------------------------------------------------------------------------------

void test_watchface_touch__initialize(void) {
  fake_rtc_init(0, 0);
  s_animating = false;
  s_launch_count = 0;
  watchface_touch_reset();
}

void test_watchface_touch__cleanup(void) {
}

void test_watchface_touch__right_to_left_opens_launcher(void) {
  prv_swipe(150, 50, 100);
  cl_assert_equal_i(s_launch_count, 1);
}

void test_watchface_touch__left_to_right_ignored(void) {
  prv_swipe(50, 150, 100);
  cl_assert_equal_i(s_launch_count, 0);
}

void test_watchface_touch__vertical_swipes_ignored(void) {
  prv_event(TouchEvent_Touchdown, 100, 150, false);
  prv_advance_ms(20);
  prv_event(TouchEvent_PositionUpdate, 100, 100, false);
  prv_advance_ms(20);
  prv_event(TouchEvent_PositionUpdate, 100, 50, false);
  prv_advance_ms(20);
  prv_event(TouchEvent_Liftoff, 100, 50, false);
  cl_assert_equal_i(s_launch_count, 0);
}

void test_watchface_touch__tap_ignored(void) {
  prv_event(TouchEvent_Touchdown, 100, 100, false);
  prv_advance_ms(50);
  prv_event(TouchEvent_Liftoff, 100, 100, false);
  cl_assert_equal_i(s_launch_count, 0);
}

void test_watchface_touch__unarmed_swipe_ignored(void) {
  // Before a wake gesture the kernel latches the whole contact as non-navigational.
  prv_swipe_gated(150, 50, 100, true);
  cl_assert_equal_i(s_launch_count, 0);
}

void test_watchface_touch__swipe_after_unarmed_contact(void) {
  // The double tap that wakes the watch is itself gated contact; the next swipe must still work.
  prv_event(TouchEvent_Touchdown, 100, 100, true);
  prv_event(TouchEvent_Liftoff, 100, 100, true);
  prv_advance_ms(80);
  prv_event(TouchEvent_Touchdown, 100, 100, true);
  prv_event(TouchEvent_Liftoff, 100, 100, true);
  prv_advance_ms(200);
  prv_swipe(150, 50, 100);
  cl_assert_equal_i(s_launch_count, 1);
}

void test_watchface_touch__slow_drag_ignored(void) {
  prv_event(TouchEvent_Touchdown, 150, 100, false);
  prv_advance_ms(200);
  prv_event(TouchEvent_PositionUpdate, 100, 100, false);
  prv_advance_ms(200);
  prv_event(TouchEvent_PositionUpdate, 50, 100, false);
  prv_event(TouchEvent_Liftoff, 50, 100, false);
  cl_assert_equal_i(s_launch_count, 0);
}

void test_watchface_touch__dropped_while_animating(void) {
  s_animating = true;
  prv_swipe(150, 50, 100);
  cl_assert_equal_i(s_launch_count, 0);
}

void test_watchface_touch__repeatable(void) {
  prv_swipe(150, 50, 100);
  prv_swipe(150, 50, 100);
  cl_assert_equal_i(s_launch_count, 2);
}

void test_watchface_touch__reset_mid_gesture(void) {
  // A modal grabbing focus resets the watchface click manager (and this gesture) mid-swipe.
  prv_event(TouchEvent_Touchdown, 150, 100, false);
  prv_advance_ms(20);
  prv_event(TouchEvent_PositionUpdate, 100, 100, false);
  watchface_touch_reset();
  prv_event(TouchEvent_PositionUpdate, 50, 100, false);
  prv_event(TouchEvent_Liftoff, 50, 100, false);
  cl_assert_equal_i(s_launch_count, 0);
  // And the next swipe still works.
  prv_advance_ms(100);
  prv_swipe(150, 50, 100);
  cl_assert_equal_i(s_launch_count, 1);
}
