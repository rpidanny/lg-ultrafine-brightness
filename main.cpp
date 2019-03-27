/*
busb_get_device_descriptor* libusb example program to list devices on the bus
* Copyright © 2007 Daniel Drake <dsd@gentoo.org>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <iostream>
#include <string>
// #include <stdio.h>
#include <vector>
#include <libusb.h>
#include <curses.h>

// From the HID spec:

static const int HID_GET_REPORT = 0x01;
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

using std::vector;
const uint16_t vendor_id = 0x43e;
const uint16_t product_id = 0x9a40;
const uint16_t max_brightness = 0xd2f0;
const uint16_t min_brightness = 0x0000;

const std::vector<uint16_t> small_steps = {
    0x0000,
    0x0190, 0x01af, 0x01d2, 0x01f7,
    0x021f, 0x024a, 0x0279, 0x02ac,
    0x02e2, 0x031d, 0x035c, 0x03a1,
    0x03eb, 0x043b, 0x0491, 0x04ee,
    0x0553, 0x05c0, 0x0635, 0x06b3,
    0x073c, 0x07d0, 0x086f, 0x091b,
    0x09d5, 0x0a9d, 0x0b76, 0x0c60,
    0x0d5c, 0x0e6c, 0x0f93, 0x10d0,
    0x1227, 0x1399, 0x1529, 0x16d9,
    0x18aa, 0x1aa2, 0x1cc1, 0x1f0b,
    0x2184, 0x2430, 0x2712, 0x2a2e,
    0x2d8b, 0x312b, 0x3516, 0x3951,
    0x3de2, 0x42cf, 0x4822, 0x4de1,
    0x5415, 0x5ac8, 0x6203, 0x69d2,
    0x7240, 0x7b5a, 0x852d, 0x8fc9,
    0x9b3d, 0xa79b, 0xb4f5, 0xc35f,
    0xd2f0};

const std::vector<uint16_t> big_steps = {
    0x0000,
    0x0190, 0x021f, 0x02e2, 0x03eb,
    0x0553, 0x073c, 0x09d5, 0x0d5c,
    0x1227, 0x18aa, 0x2184, 0x2d8b,
    0x3de2, 0x5415, 0x7240, 0x9b3d,
    0xd2f0};

static libusb_device *get_lg_ultrafine_usb_device(libusb_device **devs)
{
    libusb_device *dev, *lgdev = NULL;
    int i = 0, j = 0;
    uint8_t path[8];

    while ((dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
        {
            printw("failed to get device descriptor");
            return NULL;
        }

        if (desc.idVendor == vendor_id && desc.idProduct == product_id)
        {
            lgdev = dev;
        }

        // r = libusb_get_port_numbers(dev, path, sizeof(path));
        // if (r > 0)
        // {
        //     printw(" path: %d", path[0]);
        //     for (j = 1; j < r; j++)
        //         printw(".%d", path[j]);
        // }
        // printw("\n");
    }

    return lgdev;
}

uint16_t next_step(uint16_t val, const vector<uint16_t> &steps)
{
    auto start = 0;
    auto end = steps.size() - 1;
    while (start + 1 < end)
    {
        auto mid = start + (end - start) / 2;
        if (steps[mid] > val)
        {
            end = mid;
        }
        else
        {
            start = mid;
        }
    }
    return steps[end];
}

uint16_t prev_step(uint16_t val, const vector<uint16_t> &steps)
{
    auto start = 0;
    auto end = steps.size() - 1;
    while (start + 1 < end)
    {
        auto mid = start + (end - start) / 2;
        if (steps[mid] >= val)
        {
            end = mid;
        }
        else
        {
            start = mid;
        }
    }
    return steps[start];
}

uint16_t get_brightness(libusb_device_handle *handle)
{
    u_char data[8] = {0x00};
    // int res = hid_get_feature_report(handle, data, sizeof(data));
    int res = libusb_control_transfer(handle,
                                        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                        HID_GET_REPORT, (HID_REPORT_TYPE_FEATURE<<8)|0, 1, data, sizeof(data), 0);

    if (res < 0)
    {
        printw("Unable to get brightness.\n");
        printw("libusb_control_transfer error: %s (%d)\n", libusb_error_name(res), res);
    } 
    else {
        // for (int i = 0; i < sizeof(data); i++) {
        //     printw("0x%x  ", data[i]);
        // }
        // printw("\n");
    }

    uint16_t val = data[0] + (data[1] << 8);
    // printw("val=%d (0x%x 0x%x 0x%x)\n", val, data[0], data[1], data[2]);

    return int((float(val) / 54000) * 100.0);
}

void set_brightness(libusb_device_handle *handle, uint16_t val)
{
    u_char data[6] = {
        u_char(val & 0x00ff),
        u_char((val >> 8) & 0x00ff), 0x00, 0x00, 0x00, 0x00 };
    // int res = hid_send_feature_report(handle, data, sizeof(data));
    int res = libusb_control_transfer(handle,
                                      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                      HID_SET_REPORT, (HID_REPORT_TYPE_FEATURE<<8)|0, 1, data, sizeof(data), 0);

    if (res < 0)
    {
        printw("Unable to set brightness.\n");
        printw("libusb_control_transfer error: %s\n", libusb_error_name(res));
    } 
    // else {
    //     get_brightness(handle);
    // }
}

void adjust_brighness(libusb_device_handle *handle)
{
    auto brightness = get_brightness(handle);
    printw("Press '-' or '=' to adjust brightness.\n");
    printw("Press '[' or: ']' to fine tune.\n");
    printw("Press 'p' to use the minimum brightness\n");
    printw("Press '\\' to use the maximum brightness\n");
    printw("Press 'q' to quit.\n");

    bool stop = false;
    while (not stop)
    {
        printw("Current brightness = %d%4s\r", int((float(brightness) / 54000) * 100.0), " ");
        int c = getch();

        switch (c)
        {
            case '+':
            case '=':
                brightness = next_step(brightness, big_steps);
                set_brightness(handle, brightness);
                break;
            case '-':
            case '_':
                brightness = prev_step(brightness, big_steps);
                set_brightness(handle, brightness);
                break;
            case ']':
                brightness = next_step(brightness, small_steps);
                set_brightness(handle, brightness);
                break;
            case '[':
                brightness = prev_step(brightness, small_steps);
                set_brightness(handle, brightness);
                break;
            case '\\':
                brightness = max_brightness;
                set_brightness(handle, brightness);
                break;
            case 'p':
                brightness = min_brightness;
                set_brightness(handle, brightness);
                break;
                case 'q':
            case '\n':
                printw("You pressed q.\n");
                stop = true;
                break;
            default:
                break;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc > 2) {
        // print help
        printf("USAGE: lgufb [<brightness>]\n\n");
        printf("EXAMPLE:\n\n");
        printf("  lgufb 80\n");
        printf(" \tSetting display brightness to 80%\n");
        return 1;
    }

    libusb_device **devs, *lgdev;
    int r, openCode, iface = 1;
    ssize_t cnt;
    libusb_device_handle *handle;

    initscr();
    noecho();
    cbreak();

    r = libusb_init(NULL);
    libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);       // LIBUSB_LOG_LEVEL_DEBUG  
    if (r < 0)
    {
        printw("Unable to initialize libusb.\n");
        endwin();
        return r;
    }

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
    {
        printw("Unable to get USB device list (%ld).\n", cnt);
        endwin();
        return (int)cnt;
    }

    lgdev = get_lg_ultrafine_usb_device(devs);
    
    if (lgdev == NULL)
    {
        printw("Failed to get LG screen device.\n");
        endwin();
        return -1;
    }

    openCode = libusb_open(lgdev, &handle);
    if (openCode == 0)
    {
        libusb_set_auto_detach_kernel_driver(handle, 1);
        // r = libusb_detach_kernel_driver(handle, iface);
        // if (r == LIBUSB_SUCCESS) {
            r = libusb_claim_interface(handle, iface);
            if (r == LIBUSB_SUCCESS) {
                    if(argc == 1) {
                        adjust_brighness(handle);
                    } else if(argc == 2)
                    {
                        int percent = std::stoi(argv[1]);
                        if (percent > 100) percent = 100;
                        int brightness = int((float(percent) / 100.0) * 54000);
                        set_brightness(handle, brightness);
                    }
                libusb_release_interface(handle, iface);
                libusb_attach_kernel_driver(handle, iface);
            } else {
                printw("Failed to claim interface %d. Error: %d\n", iface, r);
                printw("Error: %s\n", libusb_error_name(r));
            }

        // } else {
        //     printw("Failed to detach interface %d. Error: %d\n", iface, r);
        //     printw("Error: %s\n", libusb_error_name(r));
        // }
        libusb_close(handle);
    }
    else
    {
        printw("libusb_open failed and returned %d\n", openCode);
    }

    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);

    getch();
    
    endwin();
    
    return 0;
}
