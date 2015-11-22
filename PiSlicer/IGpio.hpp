#ifndef PI_SLICER_I_GPIO_H
#define PI_SLICER_I_GPIO_H

namespace PiSlicer {

    /**
     * @todo Needs documentation
     */
    class IGpio {
        // Custom types
    public:
        /**
         * @todo Needs documentation
         */
        enum class Function {
            Input,
            Output,
            AlternateFunction0,
            AlternateFunction1,
            AlternateFunction2,
            AlternateFunction3,
            AlternateFunction4,
            AlternateFunction5,
        };

        /**
         * @todo Needs documentation
         */
        enum class PullMode {
            Off,
            PullDown,
            PullUp,
        };

        // Public methods
    public:
        /**
         * @todo Needs documentation
         */
        virtual ~IGpio() {}

        /**
         * @todo Needs documentation
         */
        virtual void SetFunction(Function function) = 0;

        /**
         * @todo Needs documentation
         */
        virtual void SetPullMode(PullMode pullMode) = 0;

        /**
         * @todo Needs documentation
         */
        virtual bool GetInput() = 0;

        /**
         * @todo Needs documentation
         */
        virtual void SetOutput(bool output) = 0;
    };

}

#endif /* PI_SLICER_I_GPIO_H */
