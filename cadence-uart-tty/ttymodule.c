#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/slab.h>

#define TTYPHANI_TTY_MINORS 3

struct cdns_uart_port {
    struct clk *clk;
    void __iomem *membase;
    struct uart_port port;
};

struct ttyphani_port {
    struct uart_port *uart;
    struct tty_port *tty_port;
};

struct ttyphani_ctx {
    struct tty_driver *driver;
    bool driver_registered;
    struct ttyphani_port *ports[TTYPHANI_TTY_MINORS];
};

MODULE_SOFTDEP("pre: cdns_uart_poll");

static struct uart_port *find_cadence_uart_port(struct device_node *np)
{
    struct platform_device *pdev = of_find_device_by_node(np);
    if (!pdev)
        return NULL;

    struct cdns_uart_port *cdns = platform_get_drvdata(pdev);
    if (!cdns)
        return NULL;

    return &cdns->port;
}

static int dummy_activate(struct tty_port *port, struct tty_struct *tty)
{
struct ttyphani_port *tport = tty->driver_data;
    struct uart_port *uart = tport->uart;
    const struct uart_ops *ops = uart->ops;
    if(ops->startup)
    	ops->startup(uart);
    return 0;
}

static void dummy_shutdown(struct tty_port *port)
{
struct tty_struct *tty = tty_port_tty_get(port);
    if (!tty)
        return;
struct ttyphani_port *tport = tty->driver_data;
    struct uart_port *uart = tport->uart;
    const struct uart_ops *ops = uart->ops;
    if(ops->shutdown)
    	ops->shutdown(uart);
}

static const struct tty_port_operations ttyphani_port_ops = {
    .activate = dummy_activate,
    .shutdown = dummy_shutdown,
};

static int ttyphani_open(struct tty_struct *tty, struct file *file)
{
    struct ttyphani_ctx *ctx = tty->driver->driver_state;
    struct ttyphani_port *tport = ctx->ports[tty->index];
    if (!tport || !tport->uart)
        return -ENODEV;
    tty->driver_data = tport;
    tty->termios.c_lflag &= ~(ECHO | ECHONL | ICANON);
    tty->termios.c_oflag &= ~(OPOST);
    tty->termios.c_iflag &= ~(IXON | IXOFF | ICRNL);

    int ret = tty_port_open(tport->tty_port, tty, file);
    tty_port_tty_set(tport->tty_port, tty);
    tport->uart->state->port.tty = tty;
    tport->uart->private_data = tport;

    return ret;
}

static void ttyphani_close(struct tty_struct *tty, struct file *file)
{
    struct ttyphani_port *tport = tty->driver_data;
    if (tport) {
        tty_port_close(tport->tty_port, tty, file);
        tty_port_tty_set(tport->tty_port, NULL);
    }
}

static ssize_t ttyphani_write(struct tty_struct *tty, const u8 *buf, size_t count)
{
    struct ttyphani_port *tport = tty->driver_data;
    if (!tport || !tport->uart || !tport->uart->state || !tport->uart->ops)
        return -ENODEV;

    struct uart_port *uart = tport->uart;
    struct circ_buf *circ = &uart->state->xmit;
    const struct uart_ops *ops = uart->ops;

    if (!circ->buf)
        return -ENODEV;

    ssize_t written = 0;
    while (written < count && uart_circ_chars_free(circ) > 0) {
    	//char ch = buf[written++];
        circ->buf[circ->head] = buf[written++];
        circ->head = (circ->head + 1) & (UART_XMIT_SIZE - 1);
         /*if (ch == '\n' && uart_circ_chars_free(circ) > 0) {
            circ->buf[circ->head] = '\r';
            circ->head = (circ->head + 1) & (UART_XMIT_SIZE - 1);
        }*/
    }
    while((ops->tx_empty(uart)))
    {
    	
    }
    ops->start_tx(uart);
    return written;
}

static unsigned int ttyphani_write_room(struct tty_struct *tty)
{
    struct ttyphani_port *tport = tty->driver_data;
    if (!tport || !tport->uart || !tport->uart->state)
        return 0;

    return uart_circ_chars_free(&tport->uart->state->xmit);
}

static unsigned int ttyphani_chars_in_buffer(struct tty_struct *tty)
{
    return 0;
}

static int ttyphani_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
    return -ENOIOCTLCMD;
}

static void ttyphani_set_termios(struct tty_struct *tty, const struct ktermios *old_termios)
{
    struct ttyphani_port *tport = tty->driver_data;
    struct uart_port *uart = tport ? tport->uart : NULL;
    unsigned int baud;

    if (!uart)
        return;

    // 1. Get user-requested baud rate
    baud = tty_get_baud_rate(tty);
    if (!baud)
        baud = 115200;  // default fallback

    // 2. Update UART timeout (used internally by kernel)
    uart_update_timeout(uart, tty->termios.c_cflag, baud);

    // 3. Optional: update hardware flow control if needed (future expansion)
    uart->hw_stopped = 0;

    // 4. Call low-level driver's set_termios
    if (uart->ops->set_termios)
        uart->ops->set_termios(uart, &tty->termios, old_termios);

    pr_info("ttyphani: termios updated: baud=%u, c_cflag=0x%x\n", baud, tty->termios.c_cflag);
}

static const struct tty_operations ttyphani_ops = {
    .open = ttyphani_open,
    .close = ttyphani_close,
    .write = ttyphani_write,
    .write_room = ttyphani_write_room,
    .chars_in_buffer = ttyphani_chars_in_buffer,
    .ioctl = ttyphani_ioctl,
    .set_termios = ttyphani_set_termios,
};

static int ttyphani_probe(struct platform_device *pdev)
{
    struct device_node *uart_np = of_parse_phandle(pdev->dev.of_node, "uart", 0);
    if (!uart_np)
        return -EINVAL;

    struct uart_port *uart = find_cadence_uart_port(uart_np);
    if (!uart || !uart->state || !uart->ops)
        return -EPROBE_DEFER;

    int index = uart->line;
    if (index >= TTYPHANI_TTY_MINORS)
        return -EINVAL;

    struct ttyphani_ctx *ctx = platform_get_drvdata(pdev);
    if (!ctx) {
        ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
        if (!ctx)
            return -ENOMEM;
        platform_set_drvdata(pdev, ctx);
    }

    struct ttyphani_port *tport = devm_kzalloc(&pdev->dev, sizeof(*tport), GFP_KERNEL);
    if (!tport)
        return -ENOMEM;

    tport->uart = uart;
    tport->tty_port = &uart->state->port;
    ctx->ports[index] = tport;

    if (!ctx->driver) {
        ctx->driver = tty_alloc_driver(TTYPHANI_TTY_MINORS, TTY_DRIVER_DYNAMIC_DEV);
        if (!ctx->driver)
            return -ENOMEM;

        ctx->driver->owner = THIS_MODULE;
        ctx->driver->driver_name = "ttyphani";
        ctx->driver->name = "ttyphani";
        ctx->driver->minor_start = 0;
        ctx->driver->name_base = 0;
        ctx->driver->major = 0;
        ctx->driver->type = TTY_DRIVER_TYPE_SERIAL;
        ctx->driver->subtype = SERIAL_TYPE_NORMAL;
        ctx->driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
        ctx->driver->init_termios = tty_std_termios;
        ctx->driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
        ctx->driver->init_termios.c_ispeed = 115200;
        ctx->driver->init_termios.c_ospeed = 115200;
        ctx->driver->ops = &ttyphani_ops;

        // Save context in driver for .open()
        ctx->driver->driver_state = ctx;

        int ret = tty_register_driver(ctx->driver);
        if (ret) {
            tty_driver_kref_put(ctx->driver);
            ctx->driver = NULL;
            return ret;
        }
        ctx->driver_registered = true;
    }

    tport->tty_port->ops = &ttyphani_port_ops;
    tty_port_set_initialized(tport->tty_port, 1);
    struct device *tty_dev = tty_port_register_device(tport->tty_port, ctx->driver, index, &pdev->dev);
    if (IS_ERR(tty_dev)) {
        ctx->ports[index] = NULL;
        return PTR_ERR(tty_dev);
    }

    return 0;
}

static int ttyphani_remove(struct platform_device *pdev)
{
    struct ttyphani_ctx *ctx = platform_get_drvdata(pdev);
    if (!ctx || !ctx->driver)
        return 0;

    for (int i = 0; i < TTYPHANI_TTY_MINORS; i++) {
        if (ctx->ports[i]) {
            tty_unregister_device(ctx->driver, i);
            ctx->ports[i] = NULL;
        }
    }

    if (ctx->driver_registered) {
        tty_unregister_driver(ctx->driver);
        tty_driver_kref_put(ctx->driver);
        ctx->driver = NULL;
        ctx->driver_registered = false;
    }

    return 0;
}

static const struct of_device_id ttyphani_of_match[] = {
    { .compatible = "customttyphani", },
    {}
};
MODULE_DEVICE_TABLE(of, ttyphani_of_match);

static struct platform_driver ttyphani_platform_driver = {
    .probe = ttyphani_probe,
    .remove = ttyphani_remove,
    .driver = {
        .name = "ttyphani",
        .of_match_table = ttyphani_of_match,
    },
};
module_platform_driver(ttyphani_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phani Kumar");
MODULE_DESCRIPTION("TTY driver for Cadence UART which bypasses Serial core");

