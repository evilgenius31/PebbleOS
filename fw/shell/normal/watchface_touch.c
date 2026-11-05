/* SPDX-FileCopyrightText: 2026 Core Devices LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#include "watchface.h"

#ifdef CONFIG_TOUCH

#include <stdint.h>

#include <pbl/logging/logging.h>
#include <pbl/services/compositor/compositor.h>

#include <applib/ui/recognizer/recognizer.h>
#include <applib/ui/recognizer/recognizer_list.h>
#include <applib/ui/recognizer/recognizer_manager.h>
#include <applib/ui/recognizer/swipe.h>

// Watchfaces never receive touch, so the kernel owns their gestures as it owns their buttons.
// Only a right-to-left swipe is recognized; taps stay inert so the double tap that woke the
// watch does nothing else.
static RecognizerList s_list;
static RecognizerManager s_manager;
alignas(void *) static uint8_t s_swipe_storage[SWIPE_RECOGNIZER_STATIC_SIZE];
static Recognizer *s_swipe;

static void prv_swipe_event(const Recognizer *recognizer, RecognizerEvent event) {
  if (event != RecognizerEvent_Completed ||
      swipe_recognizer_get_direction((Recognizer *)recognizer) != SwipeDirection_Left) {
    return;
  }
  if (compositor_is_animating()) {
    return;
  }
  PBL_LOG_DBG("Watchface swipe: opening launcher");
  watchface_swipe_open_launcher();
}

static void prv_init(void) {
  recognizer_list_init(&s_list);
  recognizer_manager_init(&s_manager);
  s_manager.global_list = &s_list;
  s_swipe =
      swipe_recognizer_init_static(s_swipe_storage, prv_swipe_event, nullptr, SwipeDirection_Left);
  recognizer_add_to_list(s_swipe, &s_list);
}

void watchface_handle_touch_event(const TouchEvent *touch_event) {
  if (!s_swipe) {
    prv_init();
  }
  if (touch_event->type == TouchEvent_Touchdown && touch_event->non_navigational) {
    // Unarmed contact on the idle watchface: drop the whole gesture.
    recognizer_manager_cancel_and_reset(&s_manager);
    return;
  }
  recognizer_manager_handle_touch_event(touch_event, &s_manager);
}

void watchface_touch_reset(void) {
  if (s_swipe) {
    recognizer_manager_cancel_and_reset(&s_manager);
  }
}

#endif // CONFIG_TOUCH
