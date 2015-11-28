/**
 * gpio-segled - driver for GPIO-based segmented LEDs
 *
 * This is a Linux kernel mode driver for multi-digit simple segmented
 * Light-Emitting Diode (LED) devices driven directly from General Purpose
 * Input/Output (GPIO) pins and not through separate decoder or
 * controller hardware.
 *
 * Simple segmented LED devices are common-anode or common-cathode in nature,
 * where for each digit all the anodes or all the cathodes are connected to
 * a single pin, which selects (turns on or off) that digit.  Other pins
 * select segments, where the same segment in all digits are tied together.
 * Thus every individual LED can be conceived as being a cell in a
 * two-dimensional matrix, with the rows and columns of the matrix being the
 * digits and the segments.
 *
 * Due to the fact that pins are common between either segments or digits,
 * it is impossible to display different information on different digits
 * with a static set of levels applied to the pins.  Instead, typically,
 * only one digit at a time is turned on, and the driver will rapidly "scan"
 * the digits, cycling through each digit rapidly (typically on the order of
 * 100Hz) in order to construct an illusion (via "persistence of vision")
 * that all the digits are on at the same time.  Typically, dedicated hardware
 * is used to perform this function.  This driver is useful when no such
 * hardware is present but the device can be driven directly through GPIO
 * pins instead.
 *
 * As a side benefit of the scanning function of the driver, the brightness
 * of the LEDs can be easily changed on the fly by adjusting the duty cycle
 * of each digit.  In designs where current is limited at the digit selector
 * pins, the driver may also be configured to automatically adjust the
 * brightness of each digit to match those of other digits, by taking into
 * consideration how many segments are lit (in other words, a '4' may need
 * to be kept on twice as long as a '1', because only 2 segments are lit for
 * a '1' versus 4 segments for a '4').
 *
 * Only seven-segment devices (really eight-segment, but the decimal point
 * segment is often not counted) are currently supported by this driver.
 * The LED segments are spatially arranged and labeled according to the
 * following, which is consistent with the de-facto standard described in
 * Wikipedia entry for "Seven-segment display"
 * (https://en.wikipedia.org/wiki/Seven-segment_display):
 *
 *    AAAA
 *   F    B
 *   F    B
 *    GGGG
 *   E    C
 *   E    C
 *    DDDD  P
 *
 * An example of this type of device is the package with model identifier
 * "SMA420564" etched in dot-matrix print on the side that can be found in
 * various electronic kits that are made to complement the Arduino and
 * Raspberry Pi.  Its datasheet, if one exists, has proven too elusive to
 * find online.  Nevertheless, the segment and digit pins are easily
 * determined through experimentation to be as follows:
 *
 *   Pin    Function
 *    1     Segment E anode
 *    2     Segment D anode
 *    3     Segment P (decimal point) anode
 *    4     Segment C anode
 *    5     Segment G anode
 *    6     Digit 4 cathode
 *    7     Segment B anode
 *    8     Digit 3 cathode
 *    9     Digit 2 cathode
 *   10     Segment F anode
 *   11     Segment A anode
 *   12     Digit 1 cathode
 *
 * Notes for hardware designers:
 * 1. The component has no internal current limiters, and so requires
 *    resistors or other such current limiters in an any actual design.
 *
 * 2. If limiting current at the digit selector (common anode/cathode),
 *    add "seg-adjust;" as a property to the device in the device tree,
 *    in order to configure the driver to automatically adjust the duty
 *    cycle of each digit to even out brightness between different digits.
 *
 * 3. The common anode/cathode pins, as they collect the current from
 *    up to eight separate segments, may draw more current than a typical
 *    GPIO pin can drive.  NPN transistors can be used to provide the
 *    additional current.  For an example, see the following
 *      http://learn.parallax.com/4-digit-7-segment-led-display-arduino-demo
 */

/**
 * This macro makes use of a hook in the pr_* macros to automatically
 * insert a prefix to any kernel log message from our driver.
 *
 * This needs to be defined before including any kernel headers.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/map_to_7segment.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

/**
 * This is the number of digits the device is expected to have.
 *
 * @todo
 *     Lift this restriction and get the actual number of digits
 *     from the device tree.
 */
#define NUM_DIGITS 4

/**
 * This is the default rate at which to "scan" the digits of the
 * device, in Hertz.
 */
#define DEFAULT_REFRESH_RATE_HZ    100

/**
 * This is the default brightness adjustment to make through duty
 * cycle manipulation, in percent of full duty cycle brightness.
 */
#define DEFAULT_BRIGHTNESS_PERCENT 100

/**
 * These are the internal identifiers of the GPIOs that
 * are expected of the device.
 */
enum gpio_segled_gpios {
    SEGLED_GPIO_SEGMENT_A = 0,
    SEGLED_GPIO_SEGMENT_B,
    SEGLED_GPIO_SEGMENT_C,
    SEGLED_GPIO_SEGMENT_D,
    SEGLED_GPIO_SEGMENT_E,
    SEGLED_GPIO_SEGMENT_F,
    SEGLED_GPIO_SEGMENT_G,
    SEGLED_GPIO_SEGMENT_P,
    SEGLED_GPIO_DIGIT_1,
    SEGLED_GPIO_DIGIT_2,
    SEGLED_GPIO_DIGIT_3,
    SEGLED_GPIO_DIGIT_4,
    SEGLED_GPIO_MAX
};

/**
 * These are the external identifiers (consumer identifiers in the
 * device tree) of the GPIOs that are expected of the device.
 */
static const char* gpio_segled_gpio_consumers[SEGLED_GPIO_MAX] = {
    "sa",
    "sb",
    "sc",
    "sd",
    "se",
    "sf",
    "sg",
    "sp",
    "d1",
    "d2",
    "d3",
    "d4",
};

/**
 * This is the state structure for a single LED panel.
 */
struct gpio_segled_device {
    // Linux driver model base
    struct device dev;

    // Attributes
    char digits[NUM_DIGITS];
    int decimal_points[NUM_DIGITS];
    unsigned long refresh_rate_hz;
    int brightness_percent;

    // Internal state (non-attributes)
    int resting;
    int last_digit;
    int active_digit;
    int segments_out;
    int duty_cycle_percent;

    // seg-adjust - if set in the device tree, the design uses
    // current limiters on the common anode/cathode pins, so we need
    // to adjust duty cycles to match brightness across digits.
    int seg_adjust;

    // Kernel resources
    struct gpio_desc* gpios[SEGLED_GPIO_MAX];
    struct work_struct update_digits_work;
    struct hrtimer digit_timer;
};

/**
 * Define a standard seven-segment LED conversion map, used to convert
 * from a desired digit into the signals to send to each segment pin.
 */
static SEG7_CONVERSION_MAP(gpio_segled_seg7map, MAP_ASCII7SEG_ALPHANUM_LC);

/**
 * This function sets up the device state in preparation for driving
 * the digit and segments that are next in the scanning cycle.
 *
 * It is called directly from the scanning timer callback.
 */
static void prepare_update_digits(struct gpio_segled_device* dev_impl) {
    enum gpio_segled_gpios gpio;
    int segments_out;
    int segments_lit = 0;

    // If duty cycle is valid and less than 100%, alternate
    // between resting (all digits off) and not resting (show active digit).
    // Otherwise, never rest.
    if (
        (dev_impl->duty_cycle_percent > 0)
        && (dev_impl->duty_cycle_percent < 100)
    ) {
        dev_impl->resting = !dev_impl->resting;
    } else {
        dev_impl->resting = 0;
    }

    // Nothing else to configure if reseting.
    if (dev_impl->resting) {
        return;
    }

    // Advance to next digit, returning to the first digit at the end.
    if (++dev_impl->active_digit >= NUM_DIGITS) {
        dev_impl->active_digit = 0;
    }

    // Convert character to display into bitmap selecting the segment
    // GPIOs to switch on in order to display the desired character.
    segments_out = map_to_seg7(&gpio_segled_seg7map, dev_impl->digits[dev_impl->active_digit]);

    // Mix in decimal point, if one is present at this position.
    if (dev_impl->decimal_points[dev_impl->active_digit]) {
        segments_out |= 0x80;
    }

    // Save GPIO selection bitmap for use when GPIOs are actually switched
    // in execute_update_digits.
    dev_impl->segments_out = segments_out;

    // Compute duty cycle as follows:
    // 1. Start with brightness setting.
    // 2. Factor in number of segments lit, if seg-adjust was set
    //    in device tree.
    dev_impl->duty_cycle_percent = dev_impl->brightness_percent;
    if (dev_impl->seg_adjust) {
        for (gpio = SEGLED_GPIO_SEGMENT_A; gpio <= SEGLED_GPIO_SEGMENT_P; ++gpio) {
            if ((segments_out & 1) != 0) {
                ++segments_lit;
            }
            segments_out >>= 1;
        }
        dev_impl->duty_cycle_percent = dev_impl->duty_cycle_percent * segments_lit / 8;
    }
}

/**
 * This function reconfigures the GPIOs to drive the digit and segments
 * that are next in the scanning cycle.
 *
 * It is called from a kernel worker process scheduled from the scanning
 * timer callback.
 */
static void execute_update_digits(struct work_struct* work) {
    struct gpio_segled_device* dev_impl = container_of(work, struct gpio_segled_device, update_digits_work);
    enum gpio_segled_gpios gpio;
    int segments_out = dev_impl->segments_out;

    // Make sure the last digit lit is turned off.
    gpiod_set_value_cansleep(dev_impl->gpios[SEGLED_GPIO_DIGIT_1 + dev_impl->last_digit], 0);

    // Nothing else to do if resting.
    if (dev_impl->resting) {
        return;
    }

    // Switch GPIOs to match bitmap of desired character.
    for (gpio = SEGLED_GPIO_SEGMENT_A; gpio <= SEGLED_GPIO_SEGMENT_P; ++gpio) {
        gpiod_set_value_cansleep(dev_impl->gpios[gpio], (segments_out & 1));
        segments_out >>= 1;
    }

    // Light the active digit.
    gpiod_set_value_cansleep(dev_impl->gpios[SEGLED_GPIO_DIGIT_1 + dev_impl->active_digit], 1);
    dev_impl->last_digit = dev_impl->active_digit;
}

/**
 * This is the callback for the scanning timer.  It updates the device state
 * to reflect the next digit and segments to be driven, and schedules a work
 * item to reconfigure the GPIOs to match.
 *
 * The timer expiration is updated according to the proper duty cycle
 * configured for the current digit.
 */
static enum hrtimer_restart gpio_segled_digit_timer_tick(struct hrtimer* data) {
    struct gpio_segled_device* dev_impl = container_of(data, struct gpio_segled_device, digit_timer);
    unsigned long period = 1000000000 / (NUM_DIGITS * dev_impl->refresh_rate_hz);

    // Advance device state one step in the scanning cycle.
    prepare_update_digits(dev_impl);

    // Calculate next timer period based on duty cycle and whether or
    // not we're currently resting.
    if (
        (dev_impl->duty_cycle_percent > 0)
        && (dev_impl->duty_cycle_percent < 100)
    ) {
        period = period / 100 * (dev_impl->resting ? (100 - dev_impl->duty_cycle_percent) : dev_impl->duty_cycle_percent);
    }

    // Schedule GPIO switching.
    (void)schedule_work(&dev_impl->update_digits_work);

    // Update the timer to tick again when the current period expires.
    hrtimer_add_expires_ns(&dev_impl->digit_timer, period);
    return HRTIMER_RESTART;
}

/**
 * This is called by the kernel whenever a device is removed.
 */
static void gpio_segled_device_release(struct device* dev) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    (void)hrtimer_cancel(&dev_impl->digit_timer);
    (void)cancel_work_sync(&dev_impl->update_digits_work);
    pr_info("device removed: %s\n", dev_name(dev));
    kfree(dev_impl);
}

// digits attribute: the characters to show on the LEDs

static ssize_t digits_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    return scnprintf(
        buf, PAGE_SIZE,
        "%c%s%c%s%c%s%c%s",
        dev_impl->digits[0], dev_impl->decimal_points[0] ? "." : "",
        dev_impl->digits[1], dev_impl->decimal_points[1] ? "." : "",
        dev_impl->digits[2], dev_impl->decimal_points[2] ? "." : "",
        dev_impl->digits[3], dev_impl->decimal_points[3] ? "." : ""
    );
}

static ssize_t digits_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    int digit_in = 0;
    int digit_out;

    // Initialize digits with all blanks.
    for (digit_out = 0; digit_out < NUM_DIGITS; ++digit_out) {
        dev_impl->digits[digit_out] = ' ';
        dev_impl->decimal_points[digit_out] = 0;
    }

    // Read in characters one at at time, copying them to the digit
    // buffer or setting decimal point flags as appropriate.
    digit_out = 0;
    for (digit_in = 0; digit_in < len; ++digit_in) {
        // Stop early if a non-printable character is encountered
        // or we run out of output digits.
        if (
            (buf[digit_in] < 32)
            || (digit_out >= NUM_DIGITS)
        ) {
            break;
        }

        // If the character is a decimal point, activate decimal point
        // for the previous digit (if any).  Otherwise copy the character
        // into the digit buffer.
        if (
            (buf[digit_in] == '.')
            && (digit_out > 0)
        ) {
            dev_impl->decimal_points[digit_out - 1] = 1;
        } else {
            dev_impl->digits[digit_out++] = buf[digit_in];
        }
    }

    // If not all digits were populated, shift them to the right, padding
    // the left with blanks.
    if (digit_out < NUM_DIGITS) {
        digit_in = digit_out - 1;
        for (digit_out = NUM_DIGITS - 1; digit_out >= 0; --digit_out, --digit_in) {
            if (digit_in >= 0) {
                dev_impl->digits[digit_out] = dev_impl->digits[digit_in];
                dev_impl->decimal_points[digit_out] = dev_impl->decimal_points[digit_in];
            } else {
                dev_impl->digits[digit_out] = ' ';
                dev_impl->decimal_points[digit_out] = 0;
            }
        }
    }

    // Always return size of input buffer to prevent the user from doing
    // something silly like trying to write for a second time.
    return len;
}

static DEVICE_ATTR_RW(digits);

// refresh attribute: desired refresh rate of the device in Hertz

static ssize_t refresh_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    return scnprintf(buf, PAGE_SIZE, "%lu", dev_impl->refresh_rate_hz);
}

static ssize_t refresh_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    (void)sscanf(buf, "%lu", &dev_impl->refresh_rate_hz);
    return len;
}

static DEVICE_ATTR_RW(refresh);

// brightness attribute: desired brightness in percent of maximum

static ssize_t brightness_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    return scnprintf(buf, PAGE_SIZE, "%d", dev_impl->brightness_percent);
}

static ssize_t brightness_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct gpio_segled_device* dev_impl = container_of(dev, struct gpio_segled_device, dev);
    (void)sscanf(buf, "%d", &dev_impl->brightness_percent);
    return len;
}

static DEVICE_ATTR_RW(brightness);

// attribute groups

static struct attribute* gpio_segled_attrs[] = {
    &dev_attr_digits.attr,
    &dev_attr_refresh.attr,
    &dev_attr_brightness.attr,
    NULL
};

static const struct attribute_group gpio_segled_attr_group = {
    .attrs = gpio_segled_attrs,
};

static const struct attribute_group* gpio_segled_attr_groups[] = {
    &gpio_segled_attr_group,
    NULL
};

/**
 * This is the state structure for the overall device driver.
 */
struct gpio_segled_driver {
    /**
     * This is the number of devices registered with the kernel.
     */
    int num_devices;

    /**
     * These are the pointers to the individual devices registered
     * with the kernel.
     */
    struct gpio_segled_device* devices[];
};

/**
 * This is called by the kernel whenever the driver is loaded, to set
 * up any configured devices.
 */
static int gpio_segled_probe(struct platform_device* pdev) {
    struct gpio_segled_driver* drv;
    struct fwnode_handle* child;
    struct device_node* np;
    int count, digit, ret;
    enum gpio_segled_gpios gpio;
    struct gpio_segled_device* cdev;

    // Get the number of devices listed in the device tree.  Return early
    // with an error if none are found.
    count = device_get_child_node_count(&pdev->dev);
    if (!count) {
        return -ENODEV;
    }

    // Allocate memory for the driver state, with enough space to fit
    // as many device pointers as there are devices listed in the device tree.
    // Stash the driver state in the platform private data so we can get
    // back to it from other platform callbacks such as gpio_segled_remove.
    drv = devm_kzalloc(&pdev->dev, sizeof(*drv) + sizeof(*drv->devices) * count, GFP_KERNEL);
    if (!drv) {
        return -ENOMEM;
    }
    platform_set_drvdata(pdev, drv);

    // Configure and register each device.
    device_for_each_child_node(&pdev->dev, child) {
        np = of_node(child);

        // Allocate and initialize the state for the new device.
        cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
        if (!cdev) {
            ret = -ENOMEM;
            goto unwind;
        }
        device_initialize(&cdev->dev);
        for (digit = 0; digit < NUM_DIGITS; ++digit) {
            cdev->digits[digit] = ' ';
        }
        cdev->refresh_rate_hz = DEFAULT_REFRESH_RATE_HZ;
        cdev->brightness_percent = DEFAULT_BRIGHTNESS_PERCENT;
        cdev->dev.parent = &pdev->dev;
        cdev->dev.release = gpio_segled_device_release;
        cdev->dev.groups = gpio_segled_attr_groups;

        // Attempt to reserve and configure the GPIOs listed for
        // the device in the device tree.
        for (gpio = 0; gpio < SEGLED_GPIO_MAX; ++gpio) {
            if (fwnode_property_present(child, "seg-adjust")) {
                cdev->seg_adjust = 1;
            }
            cdev->gpios[gpio] = devm_get_gpiod_from_child(&pdev->dev, gpio_segled_gpio_consumers[gpio], child);
            if (IS_ERR(cdev->gpios[gpio])) {
                ret = PTR_ERR(cdev->gpios[gpio]);
                pr_err("unable to get %s GPIO: error code %d\n", gpio_segled_gpio_consumers[gpio], ret);
                goto unwind_dev_partial;
            }
            ret = gpiod_direction_output(cdev->gpios[gpio], 0);
            if (ret) {
                pr_err("unable to set %s GPIO direction: error code %d\n", gpio_segled_gpio_consumers[gpio], ret);
                goto unwind_dev_partial;
            }
        }

        // Finish configuring the device and register it with the kernel.
        INIT_WORK(&cdev->update_digits_work, execute_update_digits);
        ret = dev_set_name(&cdev->dev, np->name);
        if (ret) {
            pr_err("unable to set %s device name: error code %d\n", np->name, ret);
            goto unwind_dev_partial;
        }
        ret = device_add(&cdev->dev);
        if (ret) {
            pr_err("unable to register %s device: error code %d\n", np->name, ret);
            goto unwind;
        }
        drv->devices[drv->num_devices++] = cdev;
        pr_info("device added: %s\n", np->name);

        // Configure and start the digit scanning timer.
        hrtimer_init(&cdev->digit_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
        cdev->digit_timer.function = gpio_segled_digit_timer_tick;
        hrtimer_start(&cdev->digit_timer, ktime_get(), HRTIMER_MODE_ABS);
    }

    return 0;
unwind_dev_partial:
    kfree(cdev);
unwind:
    for (count = drv->num_devices - 1; count >= 0; --count) {
        device_unregister(&drv->devices[count]->dev);
    }
    return ret;
}

/**
 * This is called by the kernel whenever the driver is unloaded,
 * in order to clean up any state and release any resources held
 * directly by the driver.
 */
static int gpio_segled_remove(struct platform_device* pdev) {
    struct gpio_segled_driver* drv = platform_get_drvdata(pdev);
    int count;

    for (count = drv->num_devices - 1; count >= 0; --count) {
        device_unregister(&drv->devices[count]->dev);
    }
    return 0;
}

// Open Firmware (OF) information for this driver

static const struct of_device_id of_gpio_segled_match[] = {
    { .compatible = "gpio-segled", },
    {},
};

MODULE_DEVICE_TABLE(of, of_gpio_segled_match);

// Registration with platform (Linux kernel driver model)

static struct platform_driver gpio_segled_driver = {
    .probe = gpio_segled_probe,
    .remove = gpio_segled_remove,
    .driver = {
        .name = "gpio-segled",
        .of_match_table = of_gpio_segled_match,
    },
};
module_platform_driver(gpio_segled_driver);

// Linux kernel module metadata

MODULE_DESCRIPTION("GPIO-Based Segmented LED Driver");
MODULE_AUTHOR("Richard Walters <jubajube@gmail.com>");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_ALIAS("platform:gpio-segled");
