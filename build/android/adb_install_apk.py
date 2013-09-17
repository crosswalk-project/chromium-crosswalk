#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import multiprocessing
import optparse
import os
import sys
import time

from pylib import android_commands
from pylib import constants
from pylib.utils import apk_helper
from pylib.utils import test_options_parser


def _InstallApk(args):
  apk_path, apk_package, keep_data, device = args
  result = android_commands.AndroidCommands(device=device).ManagedInstall(
      apk_path, keep_data, apk_package)
  print '-----  Installed on %s  -----' % device
  print result


def main(argv):
  parser = optparse.OptionParser()
  test_options_parser.AddInstallAPKOption(parser)
  options, args = parser.parse_args(argv)
  test_options_parser.ValidateInstallAPKOption(parser, options)
  if len(args) > 1:
    raise Exception('Error: Unknown argument:', args[1:])

  retry_times = 5
  retry_interval = 15
  while retry_times > 0:
    devices = android_commands.GetAttachedDevices()
    if not devices:
      print 'No connected devices found, '\
            'kill adb server and retry in %d seconds...' % retry_interval
      android_commands.AndroidCommands().KillAdbServer()
      time.sleep(retry_interval)
      retry_interval *= 2
      retry_times -= 1
    else:
      break

  if not devices:
    raise Exception('Error: no connected devices')

  if not options.apk_package:
    options.apk_package = apk_helper.GetPackageName(options.apk)

  pool = multiprocessing.Pool(len(devices))
  # Send a tuple (apk_path, apk_package, device) per device.
  pool.map(_InstallApk, zip([options.apk] * len(devices),
                            [options.apk_package] * len(devices),
                            [options.keep_data] * len(devices),
                            devices))


if __name__ == '__main__':
  sys.exit(main(sys.argv))
