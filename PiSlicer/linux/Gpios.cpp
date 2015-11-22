#include "../Gpios.hpp"

#include <stdint.h>
#include <stdio.h>

namespace PiSlicer {

    struct GpiosImpl {
        uint32_t base;

        GpiosImpl()
            : base(0)
        {
        }
    };

    Gpios::Gpios()
        : _impl(new GpiosImpl)
    {
        FILE* rangesFile = fopen("/proc/device-tree/soc/ranges", "rb");
        if (rangesFile != NULL) {
            (void)fseek(rangesFile, 4, SEEK_SET);
            uint8_t bytes[4];
            if (fread(bytes, sizeof(bytes), 1, rangesFile) == 1) {
                _impl->base = (
                    (((uint32_t)bytes[0]) << 24)
                    | (((uint32_t)bytes[1]) << 16)
                    | (((uint32_t)bytes[2]) << 8)
                    | (((uint32_t)bytes[3]) << 0)
                );
            }
            (void)fclose(rangesFile);
        }
    }

    Gpios::~Gpios() {
    }

    uint32_t Gpios::GetBase() {
        return _impl->base;
    }

}
