#include <inttypes.h>
#include <PiSlicer/Gpios.hpp>
#include <PiSlicer/PiSlicer.hpp>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("Hello, World!\n");
    PiSlicer::SayHello();

    PiSlicer::Gpios gpios;
    const uint32_t base = gpios.GetBase();
    printf("GPIO Base address: 0x%08" PRIX32 "\n", base);

    return EXIT_SUCCESS;
}
