#include <inttypes.h>
#include <PiSlicer/Gpios.hpp>
#include <PiSlicer/PiSlicer.hpp>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("Hello, World!\n");
    PiSlicer::SayHello();

    PiSlicer::Gpios gpios;
    auto gpio = gpios.GetGpio(24);
    gpio->SetFunction(PiSlicer::IGpio::Function::Output);
    gpio->SetOutput(false);

    return EXIT_SUCCESS;
}
