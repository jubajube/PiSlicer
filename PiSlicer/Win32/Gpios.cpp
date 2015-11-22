#include "../Gpios.hpp"

#include <memory>

namespace PiSlicer {

    struct GpiosImpl {
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
        Gpio(int gpioNumber)
            : _gpioNumber(gpioNumber)
        {
        }

        /**
         * @todo Needs documentation
         */
        ~Gpio() {
        }

        // IGpio
    public:
        virtual void SetFunction(Function function) override {
            const char* functionName;
            switch (function) {
                case Function::Input: {
                    functionName = "input";
                } break;
                case Function::Output: {
                    functionName = "output";
                } break;
                case Function::AlternateFunction0: {
                    functionName = "alternate function 0";
                } break;
                case Function::AlternateFunction1: {
                    functionName = "alternate function 1";
                } break;
                case Function::AlternateFunction2: {
                    functionName = "alternate function 2";
                } break;
                case Function::AlternateFunction3: {
                    functionName = "alternate function 3";
                } break;
                case Function::AlternateFunction4: {
                    functionName = "alternate function 4";
                } break;
                case Function::AlternateFunction5: {
                    functionName = "alternate function 5";
                } break;
                default: {
                    return;
                }
            }
            printf("GPIO%d function set to %s\n", _gpioNumber, functionName);
        }

        virtual void SetPullMode(PullMode pullMode) override {
            const char* pullModeName;
            switch (pullMode) {
                case PullMode::Off: {
                    pullModeName = "off";
                } break;
                case PullMode::PullDown: {
                    pullModeName = "down";
                } break;
                case PullMode::PullUp: {
                    pullModeName = "up";
                } break;
                default: {
                    return;
                }
            }
            printf("GPIO%d pull mode set to %s\n", _gpioNumber, pullModeName);
        }

        virtual bool GetInput() override {
            printf("GPIO%d input sampled\n", _gpioNumber);
            return false;
        }

        virtual void SetOutput(bool output) override {
            printf("GPIO%d output set to %s\n", _gpioNumber, output ? "high" : "low");
        }

        // Private properties
    private:
        /**
         * @todo Needs documentation
         */
        const int _gpioNumber;
    };

    Gpios::Gpios()
        : _impl(new GpiosImpl)
    {
    }

    Gpios::~Gpios() {
    }

    std::shared_ptr< IGpio > Gpios::GetGpio(int gpioNumber) {
        return std::make_shared< Gpio >(gpioNumber);
    }

}
