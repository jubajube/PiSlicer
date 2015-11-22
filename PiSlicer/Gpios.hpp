#ifndef PI_SLICER_GPIOS_H
#define PI_SLICER_GPIOS_H

#include "IGpio.hpp"

#include <memory>
#include <stdint.h>

namespace PiSlicer {

    /**
     * @todo Needs documentation
     */
    class Gpios {
        // Public methods
    public:
        /**
         * @todo Needs documentation
         */
        Gpios();

        /**
         * @todo Needs documentation
         */
        ~Gpios();

        /**
         * @todo Needs documentation
         */
        std::shared_ptr< IGpio > GetGpio(int gpioNumber);

        // Private properties
    private:
        /**
         * @todo Needs documentation
         */
        std::unique_ptr< struct GpiosImpl > _impl;
    };

}

#endif /* PI_SLICER_GPIOS_H */
