// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef xwalk_view_h
#define xwalk_view_h

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

EAPI Evas_Object *xwalk_view_add(Evas_Object *root_window);

EAPI Eina_Bool xwalk_view_reload(Evas_Object *obj);

EAPI Eina_Bool xwalk_view_back(Evas_Object *obj);

EAPI Eina_Bool xwalk_view_forward(Evas_Object *obj);

#ifdef __cplusplus
}
#endif
#endif // xwalk_view_h
