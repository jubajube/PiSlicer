#include "PiSlicer.hpp"
#include "Platform.hpp"

#include <stdio.h>

namespace PiSlicer {

    void SayHello() {
        printf("Hello, World! [from: PiSlicer, built on %s]\n", GetPlatformString().c_str());
    }

}
