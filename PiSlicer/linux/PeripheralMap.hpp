#ifndef PI_SLICER_LINUX_PERIPHERAL_MAP_H
#define PI_SLICER_LINUX_PERIPHERAL_MAP_H

#include <memory>
#include <stdint.h>

namespace PiSlicer {

    /**
     * @todo Needs documentation
     */
    class PeripheralMap {
        // Public methods
    public:
        /**
         * @todo Needs documentation
         */
        PeripheralMap(uint32_t base, uint32_t size);

        /**
         * @todo Needs documentation
         */
        PeripheralMap(const PeripheralMap& other) = delete;

        /**
         * @todo Needs documentation
         */
        PeripheralMap(PeripheralMap&& other) = delete;

        /**
         * @todo Needs documentation
         */
        PeripheralMap& operator=(PeripheralMap& other) = delete;

        /**
         * @todo Needs documentation
         */
        PeripheralMap& operator=(PeripheralMap&& other) = delete;

        /**
         * @todo Needs documentation
         */
        ~PeripheralMap();

        /**
         * @todo Needs documentation
         */
        uint8_t* GetBase();

        // Private properties
    private:
        /**
         * @todo Needs documentation
         */
        std::unique_ptr< struct PeripheralMapImpl > _impl;
    };

}

#endif /* PI_SLICER_LINUX_PERIPHERAL_MAP_H */
