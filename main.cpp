extern "C" {
    #include <nfc/nfc-types.h>
    #include <nfc/nfc.h>
}

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "headers/application.h"
#include "headers/device_nfc.h"
#include "headers/tools.h"

enum Mode { fast, full, GPO, UNKNOWN };

void brute_device_records(DeviceNFC &device, Application &app, Mode mode) {
    device.select_application(app);
    std::cerr << std::endl;

    size_t to_record = (mode == Mode::fast ? 16:255);

    for (size_t sfi = 1; sfi <= 31; ++sfi) {
        bool file_exist = false;
        for (size_t record = 1; record <= to_record; ++record) {
            APDU res = device.read_record(sfi, record);

            byte_t sw1 = res.data[res.size-2];
            byte_t sw2 = res.data[res.size-1];

            if (sw1 == 0x90 && sw2 == 0x00) {
                file_exist = true;
            } else if (sw1 == 0x6A && sw2 == 0x82) {
                break; // FileNotExist at this SFI
            }
        }
        if (file_exist) {
            std::cerr << std::endl;
        }
    }
}

void walk_through_gpo_files(DeviceNFC &device, Application &app) {
    device.select_application(app);
    APDU gpo = device.get_processing_options(app);

    // Find AFL tag to locate files on card
    size_t i = 0;
    while (i < gpo.size && gpo.data[i] != 0x94) {
        i++;
    }

    if (i >= gpo.size) {
        std::cerr << "Can't get AFL" << std::endl;
        return;
    }

    APDU afl = {0, {0}};
    afl.size = parse_TLV(afl.data, gpo.data, i);

    std::cerr << std::endl;
    for (size_t j = 0; j < afl.size; j+=4) {
        byte_t sfi = afl.data[j] >> 3;
        byte_t from_record = afl.data[j+1];
        byte_t to_record = afl.data[j+2];

        for (size_t record = from_record; record <= to_record; ++record) {
            device.read_record(sfi, record);
        }
        std::cerr << std::endl;
    }
    byte_t const debug_tx[] = {
        0x40, 0x01, // Pn532 InDataExchange
        0xba, 0x0b, 0xba, 0xba, 0x20, 0x00, 0x02, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xca, 0xca, 0xfe, 0xfa, 0xce, 0x14, 0x88, 0x00,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0xba, 0x0b, 0xba, 0xba, 0x20, 0x00, 0x02, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xca, 0xca, 0xfe, 0xfa, 0xce, 0x14, 0x88, 0x00,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0xba, 0x0b, 0xba, 0xba, 0x20, 0x00, 0x02, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xca, 0xca, 0xfe, 0xfa, 0xce, 0x14, 0x88, 0x00,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0xba, 0x0b, 0xba, 0xba, 0x20, 0x00, 0x02, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xca, 0xca, 0xfe, 0xfa, 0xce, 0x14, 0x88, 0x00,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0xba, 0x0b, 0xba, 0xba, 0x20, 0x00, 0x02, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xca, 0xca, 0xfe, 0xfa, 0xce, 0x14, 0x88, 0x00,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0xba, 0x0b, 0xba, 0xba, 0x20, 0x00, 0x02, 0x28, 0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xca, 0xca, 0xfe, 0xfa, 0xce, 0x14, 0x88, 0x00,
    };

    byte_t const debug_rx[] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x14, 0x88, 0x02, 0x28, 0x00, 0x00,
        0xca, 0xca, 0x00, 0xc0, 0xc0, 0x00, 0xde, 0xad, 0xbe, 0xef, 0xce, 0xee, 0xec, 0xca, 0xfe, 0xba, 0xba, 0xb0, 0xb0, 0xac, 0xdc, 0x11,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x14, 0x88, 0x02, 0x28, 0x00, 0x00,
        0xca, 0xca, 0x00, 0xc0, 0xc0, 0x00, 0xde, 0xad, 0xbe, 0xef, 0xce, 0xee, 0xec, 0xca, 0xfe, 0xba, 0xba, 0xb0, 0xb0, 0xac, 0xdc, 0x11,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x14, 0x88, 0x02, 0x28, 0x00, 0x00,
        0xca, 0xca, 0x00, 0xc0, 0xc0, 0x00, 0xde, 0xad, 0xbe, 0xef, 0xce, 0xee, 0xec, 0xca, 0xfe, 0xba, 0xba, 0xb0, 0xb0, 0xac, 0xdc, 0x11,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x14, 0x88, 0x02, 0x28, 0x00, 0x00,
        0xca, 0xca, 0x00, 0xc0, 0xc0, 0x00, 0xde, 0xad, 0xbe, 0xef, 0xce, 0xee, 0xec, 0xca, 0xfe, 0xba, 0xba, 0xb0, 0xb0, 0xac, 0xdc, 0x11,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x14, 0x88, 0x02, 0x28, 0x00, 0x00,
        0xca, 0xca, 0x00, 0xc0, 0xc0, 0x00, 0xde, 0xad, 0xbe, 0xef, 0xce, 0xee, 0xec, 0xca, 0xfe, 0xba, 0xba, 0xb0, 0xb0, 0xac, 0xdc, 0x11,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x14, 0x88, 0x02, 0x28, 0x00, 0x00
    };

    // Log string to be printed
    APDU res = device.execute_command(debug_tx, sizeof(debug_tx), "READ debug buffer");
    for(uint16_t i = 0; i < res.size - 1; i++) {
        if(res.data[i + 1] != debug_rx[i]) {
            printf("Failed in %d byte. REceived: %02X ; OUR: %02X\r\n", i, res.data[i], debug_rx[i]);
        }
    }
    if(!memcmp(&res.data[1], debug_rx, sizeof(debug_rx))) {
        std::cout << "\nTest Data OK!\n";
    } else {
        std::cout << "\nTest Data FAILED!\n";
    }
}

int main(int argc, char *argv[]) {
    Mode mode = Mode::UNKNOWN;

    if (argc == 1) {
        std::cerr << GREEN("[Info]") << " Use mode 'fast', 'full' or 'GPO'" << std::endl;
        return 0;
    }

    std::string mode_str(argv[1]);
    if (mode_str == "fast") {
        mode = Mode::fast;
    } else if (mode_str == "full") {
        mode = Mode::full;
    } else if (mode_str == "GPO") {
        mode = Mode::GPO;
    } else {
        std::cerr << RED("[Error]") << " Unknown mode" << std::endl;
        return 0;
    }

    try {
        DeviceNFC device;
        std::cerr << GREEN("[Info]") << " NFC reader: " << device.get_name() << " opened.\n";

        while (!device.pool_target()) {
            std::cerr << GREEN("[Info]") << " Searching card..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cerr << std::endl;

        std::vector<Application> list = device.load_applications_list();

        if (mode == Mode::fast || mode == Mode::full) {
            for (Application &app : list) {
                brute_device_records(device, app, mode);
            }
        } else if (mode == Mode::GPO) {
            for (Application &app : list) {
                walk_through_gpo_files(device, app);
            }
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
