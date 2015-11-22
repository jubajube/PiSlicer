#include "../Gpios.hpp"
#include "PeripheralMap.hpp"

#include <stdint.h>
#include <stdio.h>

namespace PiSlicer {

    /**
     * @todo Needs documentation
     */
    struct GpiosImpl {
        /**
         * @todo Needs documentation
         */
        std::unique_ptr< PeripheralMap > peripheralMap;
    };

    /**
     * @todo Needs documentation
     */
    class Gpio: public IGpio {
        // Public methods
    public:
        /**
         * @todo Needs documentation
         */
        Gpio(int gpioNumber) {
        }

        /**
         * @todo Needs documentation
         */
        ~Gpio() {
        }

        // IGpio
    public:
        virtual void SetFunction(Function function) override {
        }

        virtual void SetPullMode(PullMode pullMode) override {
        }

        virtual bool GetInput() override {
        }

        virtual void SetOutput(bool output) override {
        }

        // Private properties
    private:
    };

    Gpios::Gpios()
        : _impl(new GpiosImpl)
    {
        // Get SOC peripherals address range from device tree.
        uint32_t base = 0;
        uint32_t size = 0;
        FILE* rangesFile = fopen("/proc/device-tree/soc/ranges", "rb");
        if (rangesFile == NULL) {
            return;
        }
        (void)fseek(rangesFile, 4, SEEK_SET);
        uint8_t bytes[8];
        if (fread(bytes, sizeof(bytes), 1, rangesFile) == 1) {
            base = (
                (((uint32_t)bytes[0]) << 24)
                | (((uint32_t)bytes[1]) << 16)
                | (((uint32_t)bytes[2]) << 8)
                | (((uint32_t)bytes[3]) << 0)
            );
            size = (
                (((uint32_t)bytes[4]) << 24)
                | (((uint32_t)bytes[5]) << 16)
                | (((uint32_t)bytes[6]) << 8)
                | (((uint32_t)bytes[7]) << 0)
            );
        }
        (void)fclose(rangesFile);

        // Map peripherals address range to process virtual memory.
        _impl->peripheralMap.reset(new PeripheralMap(base, size));
    }

    Gpios::~Gpios() {
    }

    std::shared_ptr< IGpio > Gpios::GetGpio(int gpioNumber) {
        return std::make_shared< Gpio >(gpioNumber);
    }

}
