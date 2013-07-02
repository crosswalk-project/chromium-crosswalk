// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Elementary.h>

#include "efl_webview/lib/process_main.h"
#include "efl_webview/lib/webview.h"

static const char APP_NAME[] = "EFL WebView Example";

static int window_width = 800;
static int window_height = 600;

static void
on_back_button_clicked(void *user_data, Evas_Object *back_button, void *event_info)
{
  xwalk::WebView* webview = static_cast<xwalk::WebView*>(user_data);
  webview->Back();
}

static void
on_forward_button_clicked(void *user_data, Evas_Object *forward_button, void *event_info)
{
  xwalk::WebView* webview = static_cast<xwalk::WebView*>(user_data);
  webview->Forward();
}

static void window_create()
{
  /* Create window */
  Evas_Object* elm_window = elm_win_util_standard_add("efl-webview-window", APP_NAME);
  elm_win_autodel_set(elm_window, EINA_TRUE);

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

  /* Create WebView */
  xwalk::WebView* webview_object = xwalk::WebView::Create(elm_window);
  Evas_Object* webview = webview_object->EvasObject();
  evas_object_size_hint_weight_set(webview, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(webview, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_box_pack_end(vertical_layout, webview);
  evas_object_focus_set(webview, EINA_TRUE);
  evas_object_show(webview);

  evas_object_smart_callback_add(back_button, "clicked",
                                 on_back_button_clicked, webview_object);
  evas_object_smart_callback_add(forward_button, "clicked",
                                 on_forward_button_clicked, webview_object);

  evas_object_resize(elm_window, window_width, window_height);
  evas_object_show(elm_window);
}

int main(int argc, char *argv[])
{
  // FIXME: this function will be removed after implementing
  // sub process launcher.
  if (int exit_code = xwalk::ProcessMain(argc, argv) <= 0)
    return exit_code;

  elm_init(argc, argv);

  elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

  window_create();

  elm_run();
  elm_shutdown();

  return 0;
}
