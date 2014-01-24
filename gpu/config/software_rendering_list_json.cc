// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Determines whether certain gpu-related features are blacklisted or not.
// The format of a valid software_rendering_list.json file is defined in
// <gpu/config/gpu_control_list_format.txt>.
// The supported "features" can be found in <gpu/config/gpu_blacklist.cc>.

#include "gpu/config/gpu_control_list_jsons.h"

#define LONG_STRING_CONST(...) #__VA_ARGS__

namespace gpu {

const char kSoftwareRenderingListJson[] = LONG_STRING_CONST(

{
  "name": "software rendering list",
  // Please update the version number whenever you change this file.
  "version": "6.13",
  "entries": [
    {
      "id": 1,
      "description": "ATI Radeon X1900 is not compatible with WebGL on the Mac.",
      "webkit_bugs": [47028],
      "os": {
        "type": "macosx"
      },
      "vendor_id": "0x1002",
      "device_id": ["0x7249"],
      "features": [
        "webgl",
        "flash_3d",
        "flash_stage3d"
      ]
    },
    {
      "id": 3,
      "description": "GL driver is software rendered. Accelerated compositing is disabled.",
      "cr_bugs": [59302],
      "os": {
        "type": "linux"
      },
      "gl_renderer": {
        "op": "contains",
        "value": "software"
      },
      "features": [
        "accelerated_compositing"
      ]
    },
    {
      "id": 4,
      "description": "The Intel Mobile 945 Express family of chipsets is not compatible with WebGL.",
      "cr_bugs": [232035],
      "os": {
        "type": "any"
      },
      "vendor_id": "0x8086",
      "device_id": ["0x27AE", "0x27A2"],
      "features": [
        "webgl",
        "flash_3d",
        "flash_stage3d",
        "accelerated_2d_canvas"
      ]
    },
    {
      "id": 5,
      "description": "ATI/AMD cards with older or third-party drivers in Linux are crash-prone.",
      "cr_bugs": [71381, 76428, 73910, 101225, 136240],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x1002",
      "exceptions": [
        {
          "driver_vendor": {
            "op": "contains",
            "value": "AMD"
          },
          "driver_version": {
            "op": ">=",
            "style": "lexical",
            "value": "8.98"
          }
        }
      ],
      "features": [
        "all"
      ]
    },
    {
      "id": 8,
      "description": "NVIDIA GeForce FX Go5200 is assumed to be buggy.",
      "cr_bugs": [72938],
      "os": {
        "type": "any"
      },
      "vendor_id": "0x10de",
      "device_id": ["0x0324"],
      "features": [
        "all"
      ]
    },
    {
      "id": 10,
      "description": "NVIDIA GeForce 7300 GT on Mac does not support WebGL.",
      "cr_bugs": [73794],
      "os": {
        "type": "macosx"
      },
      "vendor_id": "0x10de",
      "device_id": ["0x0393"],
      "features": [
        "webgl",
        "flash_3d",
        "flash_stage3d"
      ]
    },
    {
      "id": 12,
      "description": "Drivers older than 2009-01 on Windows are possibly unreliable.",
      "cr_bugs": [72979, 89802],
      "os": {
        "type": "win"
      },
      "driver_date": {
        "op": "<",
        "value": "2009.1"
      },
      "exceptions": [
        {
          "vendor_id": "0x8086",
          "device_id": ["0x29a2"],
          "driver_version": {
            "op": ">=",
            "value": "7.15.10.1624"
          }
        },
        {
          "driver_vendor": {
            "op": "=",
            "value": "osmesa"
          }
        }
      ],
      "features": [
        "accelerated_2d_canvas",
        "accelerated_video",
        "accelerated_video_decode",
        "3d_css",
        "multisampling",
        "flash_3d",
        "flash_stage3d",
        "force_compositing_mode"
      ]
    },
    {
      "id": 13,
      "description": "ATI drivers older than 10.6 on Windows XP are possibly unreliable.",
      "cr_bugs": [74212],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "vendor_id": "0x1002",
      "driver_version": {
        "op": "<",
        "value": "8.741"
      },
      "features": [
        "accelerated_video",
        "accelerated_video_decode",
        "3d_css",
        "multisampling",
        "flash_3d",
        "flash_stage3d",
        "force_compositing_mode"
      ]
    },
    {
      "id": 14,
      "description": "NVIDIA drivers older than 257.21 on Windows XP are possibly unreliable.",
      "cr_bugs": [74212],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "vendor_id": "0x10de",
      "driver_version": {
        "op": "<",
        "value": "6.14.12.5721"
      },
      "features": [
        "accelerated_video",
        "accelerated_video_decode",
        "3d_css",
        "multisampling",
        "flash_3d",
        "flash_stage3d",
        "force_compositing_mode"
      ]
    },
    {
      "id": 15,
      "description": "Intel drivers older than 14.42.7.5294 on Windows XP are possibly unreliable.",
      "cr_bugs": [74212],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "vendor_id": "0x8086",
      "driver_version": {
        "op": "<",
        "value": "6.14.10.5294"
      },
      "features": [
        "accelerated_video",
        "accelerated_video_decode",
        "3d_css",
        "multisampling",
        "flash_3d",
        "flash_stage3d",
        "force_compositing_mode"
      ]
    },
    {
      "id": 16,
      "description": "Multisampling is buggy in ATI cards on older MacOSX.",
      "cr_bugs": [67752, 83153],
      "os": {
        "type": "macosx",
        "version": {
          "op": "<",
          "value": "10.7.2"
        }
      },
      "vendor_id": "0x1002",
      "features": [
        "multisampling"
      ]
    },
    {
      "id": 17,
      "description": "Intel mesa drivers are crash-prone.",
      "cr_bugs": [76703, 164555, 225200],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x8086",
      "exceptions": [
        {
          "device_id": ["0x0102", "0x0106", "0x0112", "0x0116", "0x0122", "0x0126", "0x010a", "0x0152", "0x0156", "0x015a", "0x0162", "0x0166"],
          "driver_version": {
            "op": ">=",
            "value": "8.0"
          }
        },
        {
          "device_id": ["0xa001", "0xa002", "0xa011", "0xa012", "0x29a2", "0x2992", "0x2982", "0x2972", "0x2a12", "0x2a42", "0x2e02", "0x2e12", "0x2e22", "0x2e32", "0x2e42", "0x2e92"],
          "driver_version": {
            "op": ">",
            "value": "8.0.2"
          }
        },
        {
          "device_id": ["0x0042", "0x0046"],
          "driver_version": {
            "op": ">",
            "value": "8.0.4"
          }
        },
        {
          "device_id": ["0x2a02"],
          "driver_version": {
            "op": ">=",
            "value": "9.1"
          }
        }
      ],
      "features": [
        "all"
      ]
    },
    {
      "id": 18,
      "description": "NVIDIA Quadro FX 1500 is buggy.",
      "cr_bugs": [84701],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x10de",
      "device_id": ["0x029e"],
      "features": [
        "all"
      ]
    },
    {
      "id": 19,
      "description": "GPU acceleration is no longer supported in Leopard.",
      "cr_bugs": [87157, 130495],
      "os": {
        "type": "macosx",
        "version": {
          "op": "=",
          "value": "10.5"
        }
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 23,
      "description": "Mesa drivers in linux older than 7.11 are assumed to be buggy.",
      "os": {
        "type": "linux"
      },
      "driver_vendor": {
        "op": "=",
        "value": "Mesa"
      },
      "driver_version": {
        "op": "<",
        "value": "7.11"
      },
      "exceptions": [
        {
          "driver_vendor": {
            "op": "=",
            "value": "osmesa"
          }
        }
      ],
      "features": [
        "all"
      ]
    },
    {
      "id": 24,
      "description": "Accelerated 2d canvas is unstable in Linux at the moment.",
      "os": {
        "type": "linux"
      },
      "features": [
        "accelerated_2d_canvas"
      ]
    },
    {
      "id": 27,
      "description": "ATI/AMD cards with older drivers in Linux are crash-prone.",
      "cr_bugs": [95934, 94973, 136240],
      "os": {
        "type": "linux"
      },
      "gl_vendor": {
        "op": "beginwith",
        "value": "ATI"
      },
      "exceptions": [
        {
          "driver_vendor": {
            "op": "contains",
            "value": "AMD"
          },
          "driver_version": {
            "op": ">=",
            "style": "lexical",
            "value": "8.98"
          }
        }
      ],
      "features": [
        "all"
      ]
    },
    {
      "id": 28,
      "description": "ATI/AMD cards with third-party drivers in Linux are crash-prone.",
      "cr_bugs": [95934, 94973],
      "os": {
        "type": "linux"
      },
      "gl_vendor": {
        "op": "beginwith",
        "value": "X.Org"
      },
      "gl_renderer": {
        "op": "contains",
        "value": "AMD"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 29,
      "description": "ATI/AMD cards with third-party drivers in Linux are crash-prone.",
      "cr_bugs": [95934, 94973],
      "os": {
        "type": "linux"
      },
      "gl_vendor": {
        "op": "beginwith",
        "value": "X.Org"
      },
      "gl_renderer": {
        "op": "contains",
        "value": "ATI"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 30,
      "description": "NVIDIA cards with nouveau drivers in Linux are crash-prone.",
      "cr_bugs": [94103],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x10de",
      "gl_vendor": {
        "op": "beginwith",
        "value": "nouveau"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 32,
      "description": "Accelerated 2d canvas is disabled on Windows systems with low perf stats.",
      "cr_bugs": [116350, 151500],
      "os": {
        "type": "win"
      },
      "perf_overall": {
        "op": "<",
        "value": "3.5"
      },
      "exceptions": [
        {
          "perf_gaming": {
            "op": ">",
            "value": "3.5"
          }
        },
        {
          "cpu_info": {
            "op": "contains",
            "value": "Atom"
          }
        }
      ],
      "features": [
        "accelerated_2d_canvas"
      ]
    },
    {
      "id": 33,
      "description": "Multisampling is buggy in Intel IvyBridge.",
      "cr_bugs": [116370],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x8086",
      "device_id": ["0x0152", "0x0156", "0x015a", "0x0162", "0x0166"],
      "features": [
          "multisampling"
      ]
    },
    {
      "id": 34,
      "description": "S3 Trio (used in Virtual PC) is not compatible.",
      "cr_bugs": [119948],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x5333",
      "device_id": ["0x8811"],
      "features": [
        "all"
      ]
    },
    {
      "id": 35,
      "description": "Stage3D is not supported on Linux.",
      "cr_bugs": [129848],
      "os": {
        "type": "linux"
      },
      "features": [
        "flash_stage3d"
      ]
    },
    {
      "id": 37,
      "description": "Drivers are unreliable for Optimus on Linux.",
      "cr_bugs": [131308],
      "os": {
        "type": "linux"
      },
      "multi_gpu_style": "optimus",
      "features": [
        "all"
      ]
    },
    {
      "id": 38,
      "description": "Accelerated 2D canvas is unstable for NVidia GeForce 9400M on Lion.",
      "cr_bugs": [130495],
      "os": {
        "type": "macosx",
        "version": {
          "op": "=",
          "value": "10.7"
        }
      },
      "vendor_id": "0x10de",
      "device_id": ["0x0863"],
      "features": [
        "accelerated_2d_canvas"
      ]
    },
    {
      "id": 41,
      "description": "Disable 3D (but not Stage3D) in Flash on XP",
      "cr_bugs": [134885],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "features": [
        "flash_3d"
      ]
    },
    {
      "id": 42,
      "description": "AMD Radeon HD 6490M and 6970M on Snow Leopard are buggy.",
      "cr_bugs": [137307, 285350],
      "os": {
        "type": "macosx",
        "version": {
          "op": "=",
          "value": "10.6"
        }
      },
      "vendor_id": "0x1002",
      "device_id": ["0x6760", "0x6720"],
      "features": [
        "webgl"
      ]
    },
    {
      "id": 43,
      "description": "Intel driver version 8.15.10.1749 has problems sharing textures.",
      "cr_bugs": [133924],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x8086",
      "driver_version": {
        "op": "=",
        "value": "8.15.10.1749"
      },
      "features": [
        "texture_sharing"
      ]
    },
    {
      "id": 44,
      "description": "Intel HD 4000 causes kernel panic on Lion.",
      "cr_bugs": [134015],
      "os": {
        "type": "macosx",
        "version": {
          "op": "between",
          "value": "10.7.0",
          "value2": "10.7.4"
        }
      },
      "vendor_id": "0x8086",
      "device_id": ["0x0166"],
      "multi_gpu_category": "any",
      "features": [
        "all"
      ]
    },
    {
      "id": 45,
      "description": "Parallels drivers older than 7 are buggy.",
      "cr_bugs": [138105],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x1ab8",
      "driver_version": {
        "op": "<",
        "value": "7"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 46,
      "description": "ATI FireMV 2400 cards on Windows are buggy.",
      "cr_bugs": [124152],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x1002",
      "device_id": ["0x3151"],
      "features": [
        "all"
      ]
    },
    {
      "id": 47,
      "description": "NVIDIA linux drivers older than 295.* are assumed to be buggy.",
      "cr_bugs": [78497],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x10de",
      "driver_vendor": {
        "op": "=",
        "value": "NVIDIA"
      },
      "driver_version": {
        "op": "<",
        "value": "295"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 48,
      "description": "Accelerated video decode is unavailable on Mac and Linux.",
      "cr_bugs": [137247, 133828],
      "exceptions": [
        {
          "os": {
            "type": "chromeos"
          }
        },
        {
          "os": {
            "type": "win"
          }
        },
        {
          "os": {
            "type": "android"
          }
        }
      ],
      "features": [
        "accelerated_video_decode"
      ]
    },
    {
      "id": 49,
      "description": "NVidia GeForce GT 650M can cause the system to hang with flash 3D.",
      "cr_bugs": [140175],
      "os": {
        "type": "macosx",
        "version": {
          "op": "between",
          "value": "10.8.0",
          "value2": "10.8.1"
        }
      },
      "multi_gpu_style": "optimus",
      "vendor_id": "0x10de",
      "device_id": ["0x0fd5"],
      "features": [
        "flash_3d",
        "flash_stage3d"
      ]
    },
    {
      "id": 50,
      "description": "Disable VMware software renderer.",
      "cr_bugs": [145531],
      "os": {
        "type": "linux"
      },
      "gl_vendor": {
        "op": "beginwith",
        "value": "VMware"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 51,
      "description": "NVIDIA drivers 6.14.11.9621 is buggy on Windows XP.",
      "cr_bugs": [152096],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "vendor_id": "0x10de",
      "driver_version": {
        "op": "=",
        "value": "6.14.11.9621"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 52,
      "description": "NVIDIA drivers 6.14.11.8267 is buggy on Windows XP.",
      "cr_bugs": [152096],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "vendor_id": "0x10de",
      "driver_version": {
        "op": "=",
        "value": "6.14.11.8267"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 53,
      "description": "The Intel GMA500 is too slow for Stage3D.",
      "cr_bugs": [152096],
      "vendor_id": "0x8086",
      "device_id": ["0x8108", "0x8109"],
      "features": [
        "flash_stage3d"
      ]
    },
    {
      "id": 55,
      "description": "Drivers older than 2007-01 on Windows are assumed to be buggy.",
      "cr_bugs": [72979, 89802],
      "os": {
        "type": "win"
      },
      "driver_date": {
        "op": "<",
        "value": "2007.1"
      },
      "exceptions": [
        {
          "vendor_id": "0x8086",
          "device_id": ["0x29a2"],
          "driver_version": {
            "op": ">=",
            "value": "7.15.10.1624"
          }
        },
        {
          "driver_vendor": {
            "op": "=",
            "value": "osmesa"
          }
        }
      ],
      "features": [
        "all"
      ]
    },
    {
      "id": 56,
      "description": "NVIDIA linux drivers are unstable when using multiple Open GL contexts and with low memory.",
      "cr_bugs": [145600],
      "os": {
        "type": "linux"
      },
      "vendor_id": "0x10de",
      "driver_vendor": {
        "op": "=",
        "value": "NVIDIA"
      },
      "features": [
        "accelerated_video",
        "accelerated_video_decode",
        "flash_3d",
        "flash_stage3d"
      ]
    },
    {
      // Panel fitting is only used with OS_CHROMEOS. To avoid displaying an
      // error in chrome:gpu on every other platform, this blacklist entry needs
      // to only match on chromeos. The drawback is that panel_fitting will not
      // appear to be blacklisted if accidentally queried on non-chromeos.
      "id": 57,
      "description": "Chrome OS panel fitting is only supported for Intel IVB and SNB Graphics Controllers.",
      "os": {
        "type": "chromeos"
      },
      "exceptions": [
        {
          "vendor_id": "0x8086",
          "device_id": ["0x0106", "0x0116", "0x0166"]
        }
      ],
      "features": [
        "panel_fitting"
      ]
    },
    {
      "id": 59,
      "description": "NVidia driver 8.15.11.8593 is crashy on Windows.",
      "cr_bugs": [155749],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x10de",
      "driver_version": {
        "op": "=",
        "value": "8.15.11.8593"
      },
      "features": [
        "accelerated_video_decode"
      ]
    },
    {
      "id": 60,
      "description": "Multisampling is buggy on Mac with NVIDIA gpu prior to 10.8.3.",
      "cr_bugs": [137303],
      "os": {
        "type": "macosx",
        "version": {
          "op": "<",
          "value": "10.8.3"
        }
      },
      "vendor_id": "0x10de",
      "features": [
        "multisampling"
      ]
    },
    {
      "id": 61,
      "description": "Multisampling is buggy on Mac with Intel gpu prior to 10.8.3.",
      "cr_bugs": [137303],
      "os": {
        "type": "macosx",
        "version": {
          "op": "<",
          "value": "10.8.3"
        }
      },
      "vendor_id": "0x8086",
      "features": [
        "multisampling"
      ]
    },
    {
      "id": 62,
      "description": "Accelerated 2D canvas buggy on old Qualcomm Adreno.",
      "cr_bugs": [161575],
      "os": {
        "type": "android"
      },
      "gl_renderer": {
        "op": "contains",
        "value": "Adreno"
      },
      "driver_version": {
        "op": "<",
        "value": "4.1"
      },
      "features": [
        "accelerated_2d_canvas"
      ]
    },
    {
      "id": 63,
      "description": "Multisampling is buggy on Mac with AMD gpu prior to 10.8.3.",
      "cr_bugs": [162466],
      "os": {
        "type": "macosx",
        "version": {
          "op": "<",
          "value": "10.8.3"
        }
      },
      "vendor_id": "0x1002",
      "features": [
        "multisampling"
      ]
    },
    {
      "id": 64,
      "description": "Hardware video decode is only supported in win7+.",
      "cr_bugs": [159458],
      "os": {
        "type": "win",
        "version": {
          "op": "<",
          "value": "6.1"
        }
      },
      "features": [
        "accelerated_video_decode"
      ]
    },
    {
      "id": 66,
      "description": "Force compositing mode is unstable in MacOSX earlier than 10.8.",
      "cr_bugs": [174101],
      "os": {
        "type": "macosx",
        "version": {
          "op": "<",
          "value": "10.8"
        }
      },
      "features": [
        "force_compositing_mode"
      ]
    },
    {
      "id": 67,
      "description": "Accelerated 2D Canvas is not supported on WinXP.",
      "cr_bugs": [175149],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "features": [
        "accelerated_2d_canvas"
      ]
    },
    {
      "id": 68,
      "description": "VMware Fusion 4 has corrupt rendering with Win Vista+.",
      "cr_bugs": [169470],
      "os": {
        "type": "win",
        "version": {
          "op": ">=",
          "value": "6.0"
        }
      },
      "vendor_id": "0x15ad",
      "driver_version": {
        "op": "<=",
        "value": "7.14.1.1134"
      },
      "features": [
        "all"
      ]
    },
    {
      "id": 69,
      "description": "NVIDIA driver 8.17.11.9621 is buggy with Stage3D baseline mode.",
      "cr_bugs": [172771],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x10de",
      "driver_version": {
        "op": "=",
        "value": "8.17.11.9621"
      },
      "features": [
        "flash_stage3d_baseline"
      ]
    },
    {
      "id": 70,
      "description": "NVIDIA driver 8.17.11.8267 is buggy with Stage3D baseline mode.",
      "cr_bugs": [172771],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x10de",
      "driver_version": {
        "op": "=",
        "value": "8.17.11.8267"
      },
      "features": [
        "flash_stage3d_baseline"
      ]
    },
    {
      "id": 71,
      "description": "All Intel drivers before 8.15.10.2021 are buggy with Stage3D baseline mode.",
      "cr_bugs": [172771],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x8086",
      "driver_version": {
        "op": "<",
        "value": "8.15.10.2021"
      },
      "features": [
        "flash_stage3d_baseline"
      ]
    },
    {
      "id": 72,
      "description": "NVIDIA GeForce 6200 LE is buggy with WebGL.",
      "cr_bugs": [232529],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x10de",
      "device_id": ["0x0163"],
      "features": [
        "webgl"
      ]
    },
    {
      "id": 73,
      "description": "WebGL is buggy with the NVIDIA GeForce GT 330M, 9400, and 9400M on MacOSX earlier than 10.8",
      "cr_bugs": [233523],
      "os": {
        "type": "macosx",
        "version": {
          "op": "<",
          "value": "10.8"
        }
      },
      "vendor_id": "0x10de",
      "device_id": ["0x0a29", "0x0861", "0x0863"],
      "features": [
        "webgl"
      ]
    },
    {
      "id": 74,
      "description": "GPU access is blocked if users don't have proper graphics driver installed after Windows installation",
      "cr_bugs": [248178],
      "os": {
        "type": "win"
      },
      "driver_vendor": {
        "op": "=",
        "value": "Microsoft"
      },
      "features": [
        "all"
      ]
    },
)  // String split to avoid MSVC char limit.
LONG_STRING_CONST(
    {
      "id": 75,
      "description": "Texture sharing not supported on AMD Switchable GPUs due to driver issues",
      "cr_bugs": [117371],
      "os": {
        "type": "win"
      },
      "multi_gpu_style": "amd_switchable",
      "features": [
        "texture_sharing"
      ]
    },
    {
      "id": 76,
      "description": "WebGL is disabled on Android unless GPU reset notification is supported",
      "os": {
        "type": "android"
      },
      "exceptions": [
        {
          "gl_reset_notification_strategy": {
            "op": "=",
            "value": "33362"
          }
        }
      ],
      "features": [
        "webgl"
      ]
    },
    {
      "id": 77,
      "description": "Multisampling is reportedly very slow on Quadro NVS 135M/GeForce 8400M GS",
      "cr_bugs": [279446],
      "os": {
        "type": "win",
        "version": {
          "op": "=",
          "value": "5"
        }
      },
      "vendor_id": "0x10de",
      "device_id": ["0x0429", "0x042b"],
      "features": [
        "multisampling"
      ]
    },
    {
      "id": 78,
      "description": "Accelerated video decode interferes with GPU blacklist on older Intel drivers",
      "cr_bugs": [180695],
      "os": {
        "type": "win"
      },
      "vendor_id": "0x8086",
      "driver_version": {
        "op": "between",
        "value": "8.15.10.1883",
        "value2": "8.15.10.2702"
      },
      "features": [
        "accelerated_video_decode"
      ]
    },
    {
      "id": 79,
      "description": "Disable force compositing mode on all Windows versions prior to and including Vista.",
      "cr_bugs": [273920, 170421],
      "os": {
        "type": "win",
        "version": {
          "op": "<=",
          "value": "6.0"
        }
      },
      "features": [
        "flash_3d",
        "flash_stage3d",
        "force_compositing_mode"
      ]
    },
    {
      "id": 80,
      "description": "Texture sharing should be disabled on all Windows machines",
      "cr_bugs": [304369],
      "os": {
        "type": "win"
      },
      "features": [
        "texture_sharing"
      ]
    },
    {
      "id": 82,
      "description": "MediaCodec is still too buggy to use for encoding (b/11536167).",
      "os": {
        "type": "android"
      },
      "features": [
        "accelerated_video_encode"
      ]
    }
  ]
}

);  // LONG_STRING_CONST macro

}  // namespace gpu
