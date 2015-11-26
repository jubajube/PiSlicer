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
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
//#include <linux/platform_device.h>

struct sma420564_device {
    int hello2_created;
    char display[4];
    struct device dev;
};

static ssize_t hello2_show(struct device* dev, struct device_attribute* attr, char* buf) {
    return scnprintf(buf, PAGE_SIZE, "Hello2!\n");
}

static DEVICE_ATTR_RO(hello2);

static ssize_t hello_show(struct device* dev, struct device_attribute* attr, char* buf) {
    return scnprintf(buf, PAGE_SIZE, "Hello!\n");
}

static ssize_t hello_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t len) {
    struct sma420564_device* dev_impl = container_of(dev, struct sma420564_device, dev);
    int trimmed;
    int ret;

    for (trimmed = 0; trimmed < len; ++trimmed) {
        if (buf[trimmed] <= 32) {
            break;
        }
    }

    if (sysfs_streq(buf, "create")) {
        if (dev_impl->hello2_created) {
            pr_info("hello2 attribute already created\n");
        } else {
            ret = device_create_file(dev, &dev_attr_hello2);
            if (ret) {
                pr_err("unable to create hello2 attribute: error code %d\n", ret);
            } else {
                pr_info("created hello2 attribute\n");
                dev_impl->hello2_created = 1;
            }
        }
    } else if (sysfs_streq(buf, "remove")) {
        if (dev_impl->hello2_created) {
            device_remove_file(dev, &dev_attr_hello2);
            dev_impl->hello2_created = 0;
            pr_info("removed hello2 attribute\n");
        } else {
            pr_info("did not remove hello2 attribute because it was not present\n");
        }
    }
    return len;
}

static DEVICE_ATTR_RW(hello);

static struct attribute* sma420564_default_attrs[] = {
    &dev_attr_hello.attr,
    NULL
};

static const struct attribute_group sma420564_attr_group = {
    .attrs = sma420564_default_attrs,
};

static const struct attribute_group* sma420564_attr_groups[] = {
    &sma420564_attr_group,
    NULL
};

static void sma420564_device_release(struct device* dev) {
    pr_info("device released\n");
}

static struct sma420564_device sma420564_device = {
    .display = {' ', ' ', ' ', ' '},
    .dev = {
        .release = sma420564_device_release,
        .groups = sma420564_attr_groups,
    },
};

static int sma420564_init(void) {
    int ret;

    pr_info("Hello, World!\n");

    ret = dev_set_name(&sma420564_device.dev, "sma420564");
    if (ret) {
        pr_err("unable to set root device name: error code %d\n", ret);
        return ret;
    }
    ret = device_register(&sma420564_device.dev);
    if (ret) {
        pr_err("unable to register root device: error code %d\n", ret);
        return ret;
    }

    return 0;
}

static void sma420564_exit(void) {
    pr_info("Goodbye, World!\n");
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
