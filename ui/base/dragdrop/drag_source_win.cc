// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/dragdrop/drag_source_win.h"

namespace ui {

DragSourceWin::DragSourceWin()
    : cancel_drag_(false),
      event_source_(DragDropTypes::DRAG_EVENT_SOURCE_MOUSE) {
}

DragSourceWin::DragSourceWin(DragDropTypes::DragEventSource event_source)
    : cancel_drag_(false),
      event_source_(event_source) {
}

HRESULT DragSourceWin::QueryContinueDrag(BOOL escape_pressed, DWORD key_state) {
  if (cancel_drag_)
    return DRAGDROP_S_CANCEL;

  if (escape_pressed) {
    OnDragSourceCancel();
    return DRAGDROP_S_CANCEL;
  }

  // On Windows, the touch-initiated drag-drop is driven by mouse right down
  // event programmatically.
  if ((event_source_ == DragDropTypes::DRAG_EVENT_SOURCE_MOUSE &&
      !(key_state & MK_LBUTTON)) ||
      (event_source_ == DragDropTypes::DRAG_EVENT_SOURCE_TOUCH &&
      !(key_state && MK_RBUTTON))) {
    OnDragSourceDrop();
    return DRAGDROP_S_DROP;
  }

  OnDragSourceMove();
  return S_OK;
}

HRESULT DragSourceWin::GiveFeedback(DWORD effect) {
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT DragSourceWin::QueryInterface(const IID& iid, void** object) {
  *object = NULL;
  if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropSource)) {
    *object = this;
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

ULONG DragSourceWin::AddRef() {
  base::RefCountedThreadSafe<DragSourceWin>::AddRef();
  return 0;
}

ULONG DragSourceWin::Release() {
  base::RefCountedThreadSafe<DragSourceWin>::Release();
  return 0;
}

}  // namespace ui
