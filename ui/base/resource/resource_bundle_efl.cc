// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/resource_bundle.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/synchronization/lock.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_handle.h"
#include "ui/base/ui_base_paths.h"

namespace ui {

namespace {

base::FilePath GetResourcesPakFilePath(const std::string& pak_name) {
  base::FilePath path;
  if (PathService::Get(base::DIR_MODULE, &path))
    return path.AppendASCII(pak_name.c_str());

  // Return just the name of the pack file.
  return base::FilePath(pak_name.c_str());
}

}  // namespace

void ResourceBundle::LoadCommonResources() {
  AddDataPackFromPath(GetResourcesPakFilePath("chrome.pak"),
                      SCALE_FACTOR_NONE);
  AddDataPackFromPath(GetResourcesPakFilePath(
                      "chrome_100_percent.pak"),
                      SCALE_FACTOR_100P);
}

gfx::Image& ResourceBundle::GetNativeImageNamed(int resource_id, ImageRTL rtl) {
  // Use the negative |resource_id| for the key for BIDI-aware images.
  int key = rtl == RTL_ENABLED ? -resource_id : resource_id;

  // Check to see if the image is already in the cache.
  {
    base::AutoLock lock_scope(*images_and_fonts_lock_);
    if (images_.count(key))
      return images_[key];
  }

  gfx::Image image;
  if (delegate_)
    image = delegate_->GetNativeImageNamed(resource_id, rtl);

//  if (image.IsEmpty()) {
//    scoped_refptr<base::RefCountedStaticMemory> data(
//        LoadDataResourceBytesForScale(resource_id, SCALE_FACTOR_100P));
//    GdkPixbuf* pixbuf = LoadPixbuf(data.get(), rtl == RTL_ENABLED);

//    if (!pixbuf) {
//      LOG(WARNING) << "Unable to load pixbuf with id " << resource_id;
//      NOTREACHED();  // Want to assert in debug mode.
//      return GetEmptyImage();
//    }

//    image = gfx::Image(pixbuf);  // Takes ownership.
//  }

  base::AutoLock lock_scope(*images_and_fonts_lock_);

  // Another thread raced the load and has already cached the image.
  if (images_.count(key))
    return images_[key];

  images_[key] = image;
  return images_[key];
}

}  // namespace ui
