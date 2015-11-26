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
 *    3     Decimal point anode
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
 * The seven segments are spatially arranged and labeled according to the
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
 *    DDDD
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
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
//#include <linux/platform_device.h>
#include <linux/workqueue.h>

enum sma420564_gpios {
    SMA420564_GPIO_SEGMENT_A = 0,
    SMA420564_GPIO_SEGMENT_B,
    SMA420564_GPIO_SEGMENT_C,
    SMA420564_GPIO_SEGMENT_D,
    SMA420564_GPIO_SEGMENT_E,
    SMA420564_GPIO_SEGMENT_F,
    SMA420564_GPIO_SEGMENT_G,
    SMA420564_GPIO_DIGIT_1,
    SMA420564_GPIO_DIGIT_2,
    SMA420564_GPIO_DIGIT_3,
    SMA420564_GPIO_DIGIT_4,
    SMA420564_GPIO_MAX
};

struct sma420564_gpio_definition {
    const char* con_id;
//    enum gpiod_flags flags;
    unsigned gpio;
    unsigned long flags;
};

static struct sma420564_gpio_definition sma420564_gpio_definitions[SMA420564_GPIO_MAX] = {
    { "Segment A anode", 25, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Segment B anode", 8, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Segment C anode", 7, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Segment D anode", 12, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Segment E anode", 16, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Segment F anode", 20, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Segment G anode", 21, GPIOF_OUT_INIT_LOW /* GPIOD_OUT_LOW */ },
    { "Digit 1 cathode", 6, GPIOF_OUT_INIT_HIGH /* GPIOD_OUT_HIGH */ },
    { "Digit 2 cathode", 13, GPIOF_OUT_INIT_HIGH /* GPIOD_OUT_HIGH */ },
    { "Digit 3 cathode", 19, GPIOF_OUT_INIT_HIGH /* GPIOD_OUT_HIGH */ },
    { "Digit 4 cathode", 26, GPIOF_OUT_INIT_HIGH /* GPIOD_OUT_HIGH */ }
};

struct sma420564_device {
    struct device dev;
    char digits[4];
    struct gpio_desc* gpios[SMA420564_GPIO_MAX];
    struct work_struct update_digits_work;
};

static ssize_t digits_show(struct device* dev, struct device_attribute* attr, char* buf) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    return scnprintf(
        buf, PAGE_SIZE,
        "%c%c%c%c",
        dev_impl->digits[0],
        dev_impl->digits[1],
        dev_impl->digits[2],
        dev_impl->digits[3]
    );
}

static void update_digits(struct work_struct* work) {
    struct sma420564_device* dev_impl = container_of(work, struct sma420564_device, update_digits_work);
    enum sma420564_gpios gpio;
    int segments_out;

    /*
     *   0GFE DCBA
     * 0 0011 1111  0x3F
     * 1 0000 0110  0x06
     * 2 0101 1011  0x5B
     * 3 0100 1111  0x4F
     * 4 0110 0110  0x66
     * 5 0110 1101  0x6D
     * 6 0111 1101  0x7D
     * 7 0000 0111  0x07
     * 8 0111 1111  0x7F
     * 9 0110 1111  0x6F
     */
    segments_out = 0;
    switch (dev_impl->digits[3]) {
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
    case ' ':
    default:  segments_out = 0x00; break;
    }
    for (gpio = SMA420564_GPIO_SEGMENT_A; gpio <= SMA420564_GPIO_SEGMENT_G; ++gpio) {
        gpiod_set_value_cansleep(dev_impl->gpios[gpio], (segments_out & 1));
        segments_out >>= 1;
    }
}

static ssize_t digits_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    int digit_in = 3;
    int digit_out = 3;

    if (digit_in >= len) {
        digit_in = len - 1;
    }
    while (
        (digit_in >= 0)
        && (buf[digit_in] <= 32)
    ) {
        --digit_in;
    }
    while (digit_out >= 0) {
        if (digit_in >= 0) {
            dev_impl->digits[digit_out--] = buf[digit_in--];
        } else {
            dev_impl->digits[digit_out--] = ' ';
        }
    }

    schedule_work(&dev_impl->update_digits_work);

    return len;
}

static DEVICE_ATTR_RW(digits);

static struct attribute* sma420564_attrs[] = {
    &dev_attr_digits.attr,
    NULL
};

static const struct attribute_group sma420564_attr_group = {
    .attrs = sma420564_attrs,
};

static const struct attribute_group* sma420564_attr_groups[] = {
    &sma420564_attr_group,
    NULL
};

static void sma420564_device_release(struct device* dev) {
    pr_info("device released\n");
}

static struct sma420564_device sma420564_device = {
    .digits = {' ', ' ', ' ', ' '},
    .dev = {
        .release = sma420564_device_release,
        .groups = sma420564_attr_groups,
    },
};

static int sma420564_init(void) {
    enum sma420564_gpios gpio;
    int ret;

    ret = dev_set_name(&sma420564_device.dev, KBUILD_MODNAME);
    if (ret) {
        pr_err("unable to set root device name: error code %d\n", ret);
        return ret;
    }
    ret = device_register(&sma420564_device.dev);
    if (ret) {
        pr_err("unable to register root device: error code %d\n", ret);
        return ret;
    }

    for (gpio = 0; gpio < SMA420564_GPIO_MAX; ++gpio) {
        ret = devm_gpio_request_one(
            &sma420564_device.dev,
            sma420564_gpio_definitions[gpio].gpio,
            sma420564_gpio_definitions[gpio].flags,
            sma420564_gpio_definitions[gpio].con_id
        );
        if (ret) {
            pr_err(
                "unable to obtain GPIO %u for %s\n",
                sma420564_gpio_definitions[gpio].gpio,
                sma420564_gpio_definitions[gpio].con_id
            );
        } else {
            sma420564_device.gpios[gpio] = gpio_to_desc(sma420564_gpio_definitions[gpio].gpio);
            if (IS_ERR(sma420564_device.gpios[gpio])) {
                pr_err("unable to get description for GPIO %u\n", sma420564_gpio_definitions[gpio].gpio);
            }
        }
    }

    INIT_WORK(&sma420564_device.update_digits_work, update_digits);

    schedule_work(&sma420564_device.update_digits_work);

    return 0;
}

static void sma420564_exit(void) {
    cancel_work_sync(&sma420564_device.update_digits_work);
    device_unregister(&sma420564_device.dev);
}

//static int sma420564_probe(struct platform_device* pdev) {
//    printk(KERN_INFO "Hello world 1.\n");
//    return 0;
//}
//
//static int sma420564_remove(struct platform_device* pdev) {
//    printk(KERN_INFO "Goodbye world 1.\n");
//    return 0;
//}
//
//static const struct of_device_id of_sma420564_match[] = {
//    { .compatible = "sma420564", },
//    {},
//};
//
//MODULE_DEVICE_TABLE(of, of_sma420564_match);
//
//static struct platform_driver sma420564_driver = {
//    .probe = sma420564_probe,
//    .remove = sma420564_remove,
//    .driver = {
//        .name = "sma420564",
//        .of_match_table = of_sma420564_match,
//    },
//};
//
//module_platform_driver(sma420564_driver);

module_init(sma420564_init);
module_exit(sma420564_exit);

MODULE_DESCRIPTION("");
MODULE_AUTHOR("Richard Walters <jubajube@gmail.com>");
MODULE_LICENSE("Dual MIT/GPL");
