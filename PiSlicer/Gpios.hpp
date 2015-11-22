#ifndef PI_SLICER_GPIO_H
#define PI_SLICER_GPIO_H

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
        uint32_t GetBase();

        // Private properties
    private:
        /**
         * @todo Needs documentation
         */
        std::unique_ptr< struct GpiosImpl > _impl;
    };

}

#endif /* PI_SLICER_GPIO_H */
