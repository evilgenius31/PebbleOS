/* SPDX-FileCopyrightText: 2024 Google LLC */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <pbl/services/compositor/compositor.h>

#include <kernel/events.h>
#include <process_management/app_install_manager.h>

void watchface_init(void);

void watchface_handle_button_event(PebbleEvent *e);

//! Feed a kernel touch event while the watchface is in front. A right-to-left swipe opens the
//! launcher. Gated touches (unarmed session) are dropped.
void watchface_handle_touch_event(const TouchEvent *touch_event);

//! Drop any in-flight watchface gesture.
void watchface_touch_reset(void);

//! Open the launcher as a SELECT click would, unless the watchface is ignoring input.
void watchface_swipe_open_launcher(void);

void watchface_set_default_install_id(AppInstallId id);

AppInstallId watchface_get_default_install_id(void);

void watchface_launch_default(const CompositorTransition *animation);

void watchface_start_low_power(void);

void watchface_reset_click_manager(void);
