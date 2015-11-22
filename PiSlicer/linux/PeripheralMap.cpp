#include "PeripheralMap.hpp"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace PiSlicer {

    /**
     * @todo Needs documentation
     */
    struct PeripheralMapImpl {
        /**
         * @todo Needs documentation
         */
        int memFile = -1;

        /**
         * @todo Needs documentation
         */
        uint8_t* memMapping = nullptr;

        /**
         * @todo Needs documentation
         */
        size_t mappingSize = 0;

        /**
         * @todo Needs documentation
         */
        off_t mappingOffset = 0;
    };

    PeripheralMap::PeripheralMap(uint32_t base, uint32_t size)
        : _impl(new PeripheralMapImpl)
    {
        const char* device = "/dev/mem";
        _impl->memFile = open(device, O_RDWR | O_SYNC);
        if (_impl->memFile == -1) {
            fprintf(stderr, "error: opening %s: %s\n", device, strerror(errno));
            return;
        }
        const size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
        _impl->mappingOffset = base - ((base / pageSize) * pageSize);
        _impl->mappingSize = ((size + _impl->mappingOffset + pageSize - 1) / pageSize) * pageSize;
        void* virtualAddress = mmap(0, _impl->mappingSize, PROT_READ | PROT_WRITE, MAP_SHARED, _impl->memFile, (off_t)base - _impl->mappingOffset);
        if (virtualAddress != MAP_FAILED) {
            _impl->memMapping = (uint8_t*)virtualAddress;
            printf("Mapped 0x%08" PRIX32 "--0x%08" PRIX32 " to 0x%08" PRIX32 "\n", base - (uint32_t)_impl->mappingOffset, base - (uint32_t)_impl->mappingOffset + (uint32_t)_impl->mappingSize, (uint32_t)virtualAddress);
        }
    }

    PeripheralMap::~PeripheralMap() {
        if (_impl->memMapping != nullptr) {
            (void)munmap(_impl->memMapping, _impl->mappingSize);
        }
        if (_impl->memFile != -1) {
            (void)close(_impl->memFile);
        }
    }

    uint8_t* PeripheralMap::GetBase() {
        return _impl->memMapping + _impl->mappingOffset;
    }

}
