/**
 * sma420564 - Linux kernel mode driver for four-digit seven-segment display
 *
 * This is a Linux kernel mode driver for an enigmatic four-digit
 * segment-segment light-emitting diode (LED) display whose only
 * distinguishing mark is the identifier "SMA420564" etched in dot-matrix
 * print on the side.
 *
 * This component can be found in various eletronics kits that are
 * made to complement the Arduino and Raspberry Pi.  Its datasheet,
 * if one exists, has proven too elusive to find online.  Nevertheless,
 * the device is basic in its design and operation.  It is a common-cathode
 * design with a cathode pin for each of the four digits and an anode pin
 * for each of the eight LEDS in each segment (seven segments plus
 * decimal point).
 *
 * The pinout of the device is as follows:
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
 * The eight segments are spatially arranged and labeled according to the
 * following, which is consistent with the Wikipedia entry for
 * "Seven-segment display"
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
 * The component has no internal current limiters, and so may require
 * resistors or other such current limiters in a practical design.
 *
 * Also note that the common cathode pins, as they collect the current from
 * up to eight separate anodes, may draw more current than a typical output
 * pin of an Arduino or Raspberry Pi can drive.  NPN transistors can be used
 * to provide the additional current.  For more information, see the
 * following web page:
 *      http://learn.parallax.com/4-digit-7-segment-led-display-arduino-demo
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define NUM_DIGITS 4

#define DEFAULT_REFRESH_RATE_HZ    100
#define DEFAULT_BRIGHTNESS_PERCENT 100

enum sma420564_gpios {
    SMA420564_GPIO_SEGMENT_A = 0,
    SMA420564_GPIO_SEGMENT_B,
    SMA420564_GPIO_SEGMENT_C,
    SMA420564_GPIO_SEGMENT_D,
    SMA420564_GPIO_SEGMENT_E,
    SMA420564_GPIO_SEGMENT_F,
    SMA420564_GPIO_SEGMENT_G,
    SMA420564_GPIO_SEGMENT_P,
    SMA420564_GPIO_DIGIT_1,
    SMA420564_GPIO_DIGIT_2,
    SMA420564_GPIO_DIGIT_3,
    SMA420564_GPIO_DIGIT_4,
    SMA420564_GPIO_MAX
};

static const char* sma420564_gpio_consumers[SMA420564_GPIO_MAX] = {
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

struct sma420564_device {
    struct device dev;

    char digits[NUM_DIGITS];
    int decimal_points[NUM_DIGITS];
    unsigned long refresh_rate_hz;
    int brightness_percent;

    int resting;
    int last_digit;
    int active_digit;
    int segments_out;
    int duty_cycle_percent;
    struct gpio_desc* gpios[SMA420564_GPIO_MAX];
    struct work_struct update_digits_work;
    struct hrtimer digit_timer;
};

static void prepare_update_digits(struct sma420564_device* dev_impl) {
    enum sma420564_gpios gpio;
    int segments_out;
    int segments_lit = 0;

    if (
        (dev_impl->duty_cycle_percent > 0)
        && (dev_impl->duty_cycle_percent < 100)
    ) {
        dev_impl->resting = !dev_impl->resting;
    } else {
        dev_impl->resting = 0;
    }
    if (!dev_impl->resting) {
        if (++dev_impl->active_digit >= NUM_DIGITS) {
            dev_impl->active_digit = 0;
        }

        /*
         * Digit  Segment   Code   Spatial Arrangement
         *       PGFE DCBA
         * ----- ---------  ----   -------------------
         *   0   0011 1111  0x3F
         *   1   0000 0110  0x06          AAAA
         *   2   0101 1011  0x5B         F    B
         *   3   0100 1111  0x4F         F    B
         *   4   0110 0110  0x66          GGGG
         *   5   0110 1101  0x6D         E    C
         *   6   0111 1101  0x7D         E    C
         *   7   0000 0111  0x07          DDDD
         *   8   0111 1111  0x7F
         *   9   0110 1111  0x6F
         *
         *   -   0100 0000  0x40
         *
         *   A   0111 0111  0x77
         *   B   0111 1110  0x7E
         *   C   0011 1001  0x39
         *   D   0011 1110  0x3E
         *   E   0111 1001  0x79
         *   F   0111 0001  0x71
         *   G   0111 1101  0x7D
         *   H   0111 0110  0x76
         *   I   0000 0110  0x06
         *   J   0000 1110  0x0E
         *   K   0111 0110  0x76
         *   L   0011 1000  0x38
         *   M   0011 0111  0x37
         *   N   0011 0111  0x37
         *   O   0011 1111  0x3F
         *   P   0111 0011  0x73
         *   Q   0011 1111  0x3F
         *   R   0111 0111  0x77
         *   S   0110 1101  0x6D
         *   T   0011 0001  0x31
         *   U   0011 1110  0x3E
         *   V   0011 1110  0x3E
         *   W   0011 1110  0x3E
         *   X   0111 0110  0x76
         *   Y   0111 0010  0x72
         *   Z   0101 1011  0x5B
         */
        segments_out = 0;
        switch (dev_impl->digits[dev_impl->active_digit]) {
        case '0': segments_out = 0x3F; break;
        case '1': segments_out = 0x06; break;
        case '2': segments_out = 0x5B; break;
        case '3': segments_out = 0x4F; break;
        case '4': segments_out = 0x66; break;
        case '5': segments_out = 0x6D; break;
        case '6': segments_out = 0x7D; break;
        case '7': segments_out = 0x07; break;
        case '8': segments_out = 0x7F; break;
        case '9': segments_out = 0x6F; break;
        case '-': segments_out = 0x40; break;
        case 'A': segments_out = 0x77; break;
        case 'B': segments_out = 0x7E; break;
        case 'C': segments_out = 0x39; break;
        case 'D': segments_out = 0x3E; break;
        case 'E': segments_out = 0x79; break;
        case 'F': segments_out = 0x71; break;
        case 'G': segments_out = 0x7D; break;
        case 'H': segments_out = 0x76; break;
        case 'I': segments_out = 0x06; break;
        case 'J': segments_out = 0x0E; break;
        case 'K': segments_out = 0x76; break;
        case 'L': segments_out = 0x38; break;
        case 'M': segments_out = 0x37; break;
        case 'N': segments_out = 0x37; break;
        case 'O': segments_out = 0x3F; break;
        case 'P': segments_out = 0x73; break;
        case 'Q': segments_out = 0x3F; break;
        case 'R': segments_out = 0x77; break;
        case 'S': segments_out = 0x6D; break;
        case 'T': segments_out = 0x31; break;
        case 'U': segments_out = 0x3E; break;
        case 'V': segments_out = 0x3E; break;
        case 'W': segments_out = 0x3E; break;
        case 'X': segments_out = 0x76; break;
        case 'Y': segments_out = 0x72; break;
        case 'Z': segments_out = 0x5B; break;
        case ' ':
        default:  segments_out = 0x00; break;
        }
        if (dev_impl->decimal_points[dev_impl->active_digit]) {
            segments_out |= 0x80;
        }
        dev_impl->segments_out = segments_out;
        for (gpio = SMA420564_GPIO_SEGMENT_A; gpio <= SMA420564_GPIO_SEGMENT_P; ++gpio) {
            if ((segments_out & 1) != 0) {
                ++segments_lit;
            }
            segments_out >>= 1;
        }
        dev_impl->duty_cycle_percent = dev_impl->brightness_percent * segments_lit / 8;
    }
}

static void execute_update_digits(struct work_struct* work) {
    struct sma420564_device* dev_impl = container_of(work, struct sma420564_device, update_digits_work);
    enum sma420564_gpios gpio;
    int segments_out = dev_impl->segments_out;

    gpiod_set_value_cansleep(dev_impl->gpios[SMA420564_GPIO_DIGIT_1 + dev_impl->last_digit], 0);
    if (!dev_impl->resting) {
        for (gpio = SMA420564_GPIO_SEGMENT_A; gpio <= SMA420564_GPIO_SEGMENT_P; ++gpio) {
            gpiod_set_value_cansleep(dev_impl->gpios[gpio], (segments_out & 1));
            segments_out >>= 1;
        }
        gpiod_set_value_cansleep(dev_impl->gpios[SMA420564_GPIO_DIGIT_1 + dev_impl->active_digit], 1);
        dev_impl->last_digit = dev_impl->active_digit;
    }
}

static enum hrtimer_restart sma420564_digit_timer_tick(struct hrtimer* data) {
    struct sma420564_device* dev_impl = container_of(data, struct sma420564_device, digit_timer);
    unsigned long period = 1000000000 / (NUM_DIGITS * dev_impl->refresh_rate_hz);

    prepare_update_digits(dev_impl);
    if (
        (dev_impl->duty_cycle_percent > 0)
        && (dev_impl->duty_cycle_percent < 100)
    ) {
        period = period / 100 * (dev_impl->resting ? (100 - dev_impl->duty_cycle_percent) : dev_impl->duty_cycle_percent);
    }
    (void)schedule_work(&dev_impl->update_digits_work);
    hrtimer_add_expires_ns(&dev_impl->digit_timer, period);
    return HRTIMER_RESTART;
}

static void sma420564_device_release(struct device* dev) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    (void)hrtimer_cancel(&dev_impl->digit_timer);
    (void)cancel_work_sync(&dev_impl->update_digits_work);
    pr_info("device removed: %s\n", dev_name(dev));
    kfree(dev_impl);
}

static ssize_t digits_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
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
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    int digit_in = 0;
    int digit_out;

    for (digit_out = 0; digit_out < NUM_DIGITS; ++digit_out) {
        dev_impl->digits[digit_out] = ' ';
        dev_impl->decimal_points[digit_out] = 0;
    }

    digit_out = 0;
    for (digit_in = 0; digit_in < len; ++digit_in) {
        if (
            (buf[digit_in] < 32)
            || (digit_out >= NUM_DIGITS)
        ) {
            break;
        }

        if (
            (buf[digit_in] == '.')
            && (digit_out > 0)
        ) {
            dev_impl->decimal_points[digit_out - 1] = 1;
        } else {
            dev_impl->digits[digit_out++] = buf[digit_in];
        }
    }

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

    return len;
}

static DEVICE_ATTR_RW(digits);

static ssize_t refresh_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    return scnprintf(buf, PAGE_SIZE, "%lu", dev_impl->refresh_rate_hz);
}

static ssize_t refresh_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    (void)sscanf(buf, "%lu", &dev_impl->refresh_rate_hz);
    return len;
}

static DEVICE_ATTR_RW(refresh);

static ssize_t brightness_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    return scnprintf(buf, PAGE_SIZE, "%d", dev_impl->brightness_percent);
}

static ssize_t brightness_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    (void)sscanf(buf, "%d", &dev_impl->brightness_percent);
    return len;
}

static DEVICE_ATTR_RW(brightness);

static struct attribute* sma420564_attrs[] = {
    &dev_attr_digits.attr,
    &dev_attr_refresh.attr,
    &dev_attr_brightness.attr,
    NULL
};

static const struct attribute_group sma420564_attr_group = {
    .attrs = sma420564_attrs,
};

static const struct attribute_group* sma420564_attr_groups[] = {
    &sma420564_attr_group,
    NULL
};

struct sma420564_driver {
    int num_devices;
    struct sma420564_device* devices[];
};

static int sma420564_probe(struct platform_device* pdev) {
    struct sma420564_driver* drv;
    struct fwnode_handle* child;
    struct device_node* np;
    int count, digit, ret;
    enum sma420564_gpios gpio;
    struct sma420564_device* cdev;

    count = device_get_child_node_count(&pdev->dev);
    if (!count) {
        return -ENODEV;
    }

    drv = devm_kzalloc(&pdev->dev, sizeof(*drv) + sizeof(*drv->devices) * count, GFP_KERNEL);
    if (!drv) {
        return -ENOMEM;
    }
    platform_set_drvdata(pdev, drv);

    device_for_each_child_node(&pdev->dev, child) {
        np = of_node(child);

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
        cdev->dev.release = sma420564_device_release;
        cdev->dev.groups = sma420564_attr_groups;

        for (gpio = 0; gpio < SMA420564_GPIO_MAX; ++gpio) {
            cdev->gpios[gpio] = devm_get_gpiod_from_child(&pdev->dev, sma420564_gpio_consumers[gpio], child);
            if (IS_ERR(cdev->gpios[gpio])) {
                ret = PTR_ERR(cdev->gpios[gpio]);
                pr_err("unable to get %s GPIO: error code %d\n", sma420564_gpio_consumers[gpio], ret);
                goto unwind_dev_partial;
            }
            ret = gpiod_direction_output(cdev->gpios[gpio], 0);
            if (ret) {
                pr_err("unable to set %s GPIO direction: error code %d\n", sma420564_gpio_consumers[gpio], ret);
                goto unwind_dev_partial;
            }
        }

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

        hrtimer_init(&cdev->digit_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
        cdev->digit_timer.function = sma420564_digit_timer_tick;
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

static int sma420564_remove(struct platform_device* pdev) {
    struct sma420564_driver* drv = platform_get_drvdata(pdev);
    int count;

    for (count = drv->num_devices - 1; count >= 0; --count) {
        device_unregister(&drv->devices[count]->dev);
    }
    return 0;
}

static const struct of_device_id of_sma420564_match[] = {
    { .compatible = "sma420564", },
    {},
};

MODULE_DEVICE_TABLE(of, of_sma420564_match);

static struct platform_driver sma420564_driver = {
    .probe = sma420564_probe,
    .remove = sma420564_remove,
    .driver = {
        .name = "sma420564",
        .of_match_table = of_sma420564_match,
    },
};

module_platform_driver(sma420564_driver);

MODULE_DESCRIPTION("SMA420564 LED Driver");
MODULE_AUTHOR("Richard Walters <jubajube@gmail.com>");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_ALIAS("platform:sma420564");
