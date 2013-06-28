#include "ui/base/efl/ewk_view_wrapper.h"

namespace ui {

EwkViewWrapper::EwkViewWrapper()
    : raw_object_(NULL) {
}

EwkViewWrapper::~EwkViewWrapper() {
  // TODO(rakuco): If we add raw_object_ to another Evas_Object*, its lifetime
  // is controlled by it. Not sure if we really need to do this here.
  if (raw_object_) {
    evas_object_del(raw_object_);
    raw_object_ = NULL;
  }
}

void EwkViewWrapper::Init(Evas_Object* parent) {
  raw_object_ = evas_object_rectangle_add(evas_object_evas_get(parent));
  evas_object_color_set(raw_object_, 0, 255, 0, 255);
  evas_object_size_hint_weight_set(raw_object_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
}

} // namespace ui
