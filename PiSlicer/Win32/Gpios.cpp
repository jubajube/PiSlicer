#include "../Gpio.hpp"

namespace PiSlicer {

    struct GpiosImpl {
    };

    Gpios::Gpios()
        : _impl(new GpiosImpl)
    {
    }

    Gpios::~Gpios() {
    }

    uint32_t Gpios::GetBase() {
        return 0;
    }

}
