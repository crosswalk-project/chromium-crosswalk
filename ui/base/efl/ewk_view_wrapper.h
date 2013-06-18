#ifndef UI_BASE_EFL_EWK_VIEW_WRAPPER_H_
#define UI_BASE_EFL_EWK_VIEW_WRAPPER_H_

#include "base/basictypes.h"

typedef struct _Evas_Object Evas_Object;

namespace ui {

class EwkViewWrapper {
 public:
  EwkViewWrapper();
  virtual ~EwkViewWrapper();

  void Init(Evas_Object* parent);

  Evas_Object* get() { return raw_object_; }

 private:
  Evas_Object* raw_object_;

  DISALLOW_COPY_AND_ASSIGN(EwkViewWrapper);
};

} // namespace ui

#endif // UI_BASE_EFL_EWK_VIEW_WRAPPER_H_
