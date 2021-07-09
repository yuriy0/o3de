# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# To Do: Add a cli and extend with configurable screenshot name

import azlmbr.bus
import azlmbr.atom

azlmbr.atom.FrameCaptureRequestBus(\
    azlmbr.bus.Broadcast,
    "CapturePassAttachment",
    ["Root", "MainPipeline_0", "MainPipeline", "PostProcessPass", "LightAdaptation", "DisplayMapperPass", "DisplayMapperPassthrough"], "Output",\
    "screenshot_display_mapper_passthrough.tif")
