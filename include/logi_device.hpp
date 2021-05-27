/*
 * This file is part of Crypt Underworld.
 *
 * Crypt Underworld is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later
 * version.
 *
 * Crypt Underworld is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Crypt Underworld. If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Copyright (c) 2021 Zoë Sparks <zoe@milky.flowers>
 */

#ifndef xe47e6d7980140aa9940894be9435fc2
#define xe47e6d7980140aa9940894be9435fc2

#include <vulkan/vulkan.h>

#include "phys_device.hpp"

namespace cu {

class Instance;

class LogiDevice {
public:
    LogiDevice(PhysDevice&, Instance&);

    LogiDevice(LogiDevice&&) = delete;
    LogiDevice(const LogiDevice&) = delete;
    LogiDevice& operator=(const LogiDevice&) = delete;

    ~LogiDevice();

    VkDevice inner() { return dev; }

    PFN_vkVoidFunction get_proc_addr(const char* name);

private:
    VkDevice dev;
    VkQueue graphics;
    VkQueue present;

    PFN_vkCreateDevice create_dev;
    PFN_vkGetDeviceQueue get_dev_queue;
    PFN_vkGetDeviceProcAddr get_dev_proc_addr;
    PFN_vkDestroyDevice destroy_dev;
};

} // namespace cu

#endif