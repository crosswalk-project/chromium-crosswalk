// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Elementary.h>

#include "efl_webview/lib/process_main.h"
#include "efl_webview/lib/webview.h"
#include "efl_webview/public/xwalk_main.h"
#include "efl_webview/public/xwalk_view.h"

static const char APP_NAME[] = "EFL WebView Example";

static int window_width = 800;
static int window_height = 600;

static void
on_back_button_clicked(void *user_data, Evas_Object *back_button, void *event_info)
{
  xwalk_view_back((Evas_Object*)user_data);
}

static void
on_forward_button_clicked(void *user_data, Evas_Object *forward_button, void *event_info)
{
  xwalk_view_forward((Evas_Object*)user_data);
}

static void
on_reload_button_clicked(void *user_data,
                         Evas_Object *forward_button, void *event_info)
{
  xwalk_view_reload((Evas_Object*)user_data);
}

static void window_create()
{
  /* Create window */
  Evas_Object* elm_window = elm_win_util_standard_add("efl-webview-window", APP_NAME);
  elm_win_autodel_set(elm_window, EINA_TRUE);
  elm_object_focus_allow_set(elm_window, EINA_TRUE);
  elm_win_focus_highlight_enabled_set(elm_window, EINA_TRUE);

  /* Create vertical layout */
  Evas_Object* vertical_layout = elm_box_add(elm_window);
  elm_box_padding_set(vertical_layout, 0, 2);
  evas_object_size_hint_weight_set(vertical_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_win_resize_object_add(elm_window, vertical_layout);
  evas_object_show(vertical_layout);

  /* Create horizontal layout for top bar */
  Evas_Object* horizontal_layout = elm_box_add(elm_window);
  elm_box_horizontal_set(horizontal_layout, EINA_TRUE);
  evas_object_size_hint_weight_set(horizontal_layout, EVAS_HINT_EXPAND, 0.0);
  evas_object_size_hint_align_set(horizontal_layout, EVAS_HINT_FILL, 0.0);
  elm_box_pack_end(vertical_layout, horizontal_layout);
  evas_object_show(horizontal_layout);

  /* Create Back button */
  Evas_Object* back_button = elm_button_add(elm_window);
  elm_object_text_set(back_button, "BACK");
  evas_object_size_hint_weight_set(back_button, 0.0, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(back_button, 0.0, 0.5);
  elm_box_pack_end(horizontal_layout, back_button);
  evas_object_show(back_button);

  /* Create Forward button */
  Evas_Object* forward_button = elm_button_add(elm_window);
  elm_object_text_set(forward_button, "FORWARD");
  evas_object_size_hint_weight_set(forward_button, 0.0, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(forward_button, 0.0, 0.5);
  elm_box_pack_end(horizontal_layout, forward_button);
  evas_object_show(forward_button);

  /* Create Reload button */
  Evas_Object* reload_button = elm_button_add(elm_window);
  elm_object_text_set(reload_button, "RELOAD");
  evas_object_size_hint_weight_set(reload_button, 0.0, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(reload_button, 0.0, 0.5);
  elm_box_pack_end(horizontal_layout, reload_button);
  evas_object_show(reload_button);

  /* Create WebView */
  Evas_Object* webview = xwalk_view_add(elm_window);
  evas_object_size_hint_weight_set(webview, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(webview, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_box_pack_end(vertical_layout, webview);
  elm_object_focus_set(webview, EINA_TRUE);
  evas_object_show(webview);

  evas_object_smart_callback_add(back_button, "clicked",
                                 on_back_button_clicked, webview);
  evas_object_smart_callback_add(forward_button, "clicked",
                                 on_forward_button_clicked, webview);
  evas_object_smart_callback_add(reload_button, "clicked",
                                 on_reload_button_clicked, webview);

  evas_object_resize(elm_window, window_width, window_height);
  evas_object_show(elm_window);
}

int main(int argc, char *argv[])
{
  // FIXME: Handle chrome command line and url.
  // It is needed only in development stage.
  xwalk_init(argc, argv);

  elm_init(argc, argv);

  elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

  window_create();

  elm_run();
  elm_shutdown();

  return 0;
}
