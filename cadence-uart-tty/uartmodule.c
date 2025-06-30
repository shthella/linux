// uart_driver.c
#include <linux/module.h>
#include <linux/io.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/slab.h>

#define CDNS_UART_CR         0x00
#define CDNS_UART_SR         0x2C
#define CDNS_UART_FIFO       0x30
#define CDNS_UART_IER        0x08
#define CDNS_UART_IMR        0x10
#define CDNS_UART_RXTL       0x20
#define CDNS_UART_ISR        0x14

#define CDNS_UART_CR_TX_EN   0x10
#define CDNS_UART_CR_RX_EN   0x04
#define CDNS_UART_CR_TX_DIS  0x20
#define CDNS_UART_CR_RX_DIS  0x08

#define CDNS_UART_SR_TXEMPTY 0x08
#define CDNS_UART_SR_TXFULL  0x10
#define CDNS_UART_SR_RXEMPTY 0x02

struct cdns_uart_port {
    struct clk *clk;
    void __iomem *membase;
    struct uart_port port;
};

#define CDNS_UART_NR 3

static struct uart_driver cdns_uart_driver = {
    .owner       = THIS_MODULE,
    .driver_name = "cdns_uart_poll",
    .dev_name    = "ttyphani",
    .nr          = CDNS_UART_NR,
};

static irqreturn_t cdns_uart_irq_handler(int irq, void *dev_id)
{
    struct cdns_uart_port *cdns_port = dev_id;
    struct uart_port *port = &cdns_port->port;
    struct tty_port *tport = NULL;
    u32 isr = readl(cdns_port->membase + CDNS_UART_ISR);
    writel(isr, cdns_port->membase + CDNS_UART_ISR);
    while (!(readl(cdns_port->membase + CDNS_UART_SR) & CDNS_UART_SR_RXEMPTY)) {
        u8 ch = readl(cdns_port->membase + CDNS_UART_FIFO) & 0xFF;
        if (port->state && port->state->port.tty) {
            tport = &port->state->port;
            if (tty_buffer_request_room(tport, 1))
                tty_insert_flip_char(tport, ch, TTY_NORMAL);
        }
    }

    if (tport)
        tty_flip_buffer_push(tport);

    return IRQ_HANDLED;
}

static void cdns_uart_start_tx(struct uart_port *port)
{
    struct cdns_uart_port *cdns_port = container_of(port, struct cdns_uart_port, port);
    struct circ_buf *xmit = &port->state->xmit;

    writel(CDNS_UART_CR_TX_EN, cdns_port->membase + CDNS_UART_CR);

    while (!uart_circ_empty(xmit)) {
        while (readl(cdns_port->membase + CDNS_UART_SR) & CDNS_UART_SR_TXFULL)
            cpu_relax();

        writel(xmit->buf[xmit->tail], cdns_port->membase + CDNS_UART_FIFO);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
    }
}

static void cdns_uart_stop_tx(struct uart_port *port)
{
    struct cdns_uart_port *cdns_port = container_of(port, struct cdns_uart_port, port);
    writel(CDNS_UART_CR_TX_DIS, cdns_port->membase + CDNS_UART_CR);
}

static void cdns_uart_stop_rx(struct uart_port *port)
{
    struct cdns_uart_port *cdns_port = container_of(port, struct cdns_uart_port, port);
    writel(CDNS_UART_CR_RX_DIS, cdns_port->membase + CDNS_UART_CR);
}

static unsigned int cdns_uart_tx_empty(struct uart_port *port)
{
    struct cdns_uart_port *cdns_port = container_of(port, struct cdns_uart_port, port);
    return (readl(cdns_port->membase + CDNS_UART_SR) & CDNS_UART_SR_TXEMPTY) ? TIOCSER_TEMT : 0;
}

static void cdns_uart_set_termios(struct uart_port *port,struct ktermios *termios,const struct ktermios *old)
{
    struct cdns_uart_port *cdns = container_of(port, struct cdns_uart_port, port);
    unsigned long clk_freq = clk_get_rate(cdns->clk);
    unsigned int baud, div, gen;
    unsigned int best_gen = 0, best_div = 0;
    unsigned long best_diff = ~0UL;
    int i, j;
    u32 mode = 0;
    pr_info("UART Clock Rate: %lu\n", clk_freq);
    // 1. Baud rate calculation (Brute force search)
    baud = uart_get_baud_rate(port, termios, old, 9600, 115200);
    for (i = 0; i < 255; i++) {
        for (j = 0; j < 255; j++) {
            unsigned long calc_baud = clk_freq / ((i) * (j + 1));
            long diff = abs(calc_baud - baud);
            if (diff < best_diff) {
                best_diff = diff;
                best_gen = i;
                best_div = j;
                if (diff == 0)
                    goto found;
            }
        }
    }

found:
    writel(best_div, cdns->membase + 0x34);  // Baud_rate_divider  <-- FIRST
    writel(best_gen, cdns->membase + 0x18);  // Baud_rate_gen      <-- THEN
    uart_update_timeout(port, termios->c_cflag, baud);

    // 2. Data bits (Character length) → CHRL (bits [2:1])
    switch (termios->c_cflag & CSIZE) {
    case CS6: mode |= (0x3 << 1); break; // 6 bits: CHRL = 11
    case CS7: mode |= (0x2 << 1); break; // 7 bits: CHRL = 10
    case CS8:
    default:  mode |= (0x0 << 1); break; // 8 bits: CHRL = 0x (00/01 → default to 00)
    }

    // 3. Parity → PAR (bits [5:3])
    if (termios->c_cflag & PARENB) {
        if (termios->c_cflag & PARODD)
            mode |= (0x1 << 3); // Odd parity → PAR=001
        else
            mode |= (0x0 << 3); // Even parity → PAR=000
    } else {
        mode |= (0x4 << 3);     // No parity → PAR=1xx (choose 100)
    }

    // 4. Stop bits → NBSTOP (bits [7:6])
    if (termios->c_cflag & CSTOPB)
        mode |= (0x2 << 6); // 2 stop bits → NBSTOP = 10
    else
        mode |= (0x0 << 6); // 1 stop bit → NBSTOP = 00

    // Optional: WSIZE = 00 (default legacy mode), CHMODE = 00 (normal), CLKS = 0
    // So rest of bits remain 0

    writel(mode, cdns->membase + 0x04);  // Write to Mode Register
    writel(CDNS_UART_CR_RX_DIS | CDNS_UART_CR_TX_DIS, cdns->membase + CDNS_UART_CR);
    writel(CDNS_UART_CR_TX_EN | CDNS_UART_CR_RX_EN, cdns->membase + 0x00);
    dev_info(port->dev, "termios: baud=%u gen=%u div=%u, mode=0x%08x\n",baud, best_gen, best_div, mode);
    dev_info(port->dev, "Mode=0x%x, BRD=%u, BRG=%u\n",
         readl(cdns->membase + 0x04),
         readl(cdns->membase + 0x34),
         readl(cdns->membase + 0x18));
}

static int cdns_uart_startup(struct uart_port *port)
{
    struct cdns_uart_port *cdns_port = container_of(port, struct cdns_uart_port, port);
    writel(CDNS_UART_CR_RX_EN|CDNS_UART_CR_TX_EN, cdns_port->membase + CDNS_UART_CR);
    writel(0x07, cdns_port->membase + CDNS_UART_IER);
    return 0;
}

static void cdns_uart_shutdown(struct uart_port *port)
{
    struct cdns_uart_port *cdns_port = container_of(port, struct cdns_uart_port, port);
    writel(CDNS_UART_CR_RX_DIS | CDNS_UART_CR_TX_DIS, cdns_port->membase + CDNS_UART_CR);
}

static const char *cdns_uart_type(struct uart_port *port) { return "cdns_uart_poll"; }

static const struct uart_ops cdns_uart_ops = {
    .tx_empty   = cdns_uart_tx_empty,
    .start_tx   = cdns_uart_start_tx,
    .stop_tx    = cdns_uart_stop_tx,
    .stop_rx    = cdns_uart_stop_rx,
    .startup    = cdns_uart_startup,
    .shutdown   = cdns_uart_shutdown,
    .set_termios = cdns_uart_set_termios,
    .type       = cdns_uart_type,
};

static int cdns_uart_probe(struct platform_device *pdev)
{
    struct cdns_uart_port *cdns_port;
    struct resource *res;
    int ret;
printk("Probe started in uart driver...\n");
    cdns_port = devm_kzalloc(&pdev->dev, sizeof(*cdns_port), GFP_KERNEL);
    if (!cdns_port)
        return -ENOMEM;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    cdns_port->membase = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(cdns_port->membase))
        return PTR_ERR(cdns_port->membase);

    cdns_port->port.mapbase = res->start;
    cdns_port->port.membase = cdns_port->membase;
    cdns_port->port.ops = &cdns_uart_ops;
    cdns_port->port.iotype = UPIO_MEM;
    cdns_port->port.irq = platform_get_irq(pdev, 0);
    cdns_port->port.fifosize = 64;
    cdns_port->port.flags = UPF_BOOT_AUTOCONF;
    cdns_port->port.line = of_alias_get_id(pdev->dev.of_node, "serial");
    cdns_port->port.dev = &pdev->dev;
    cdns_port->port.type = PORT_16550;

    cdns_port->clk = devm_clk_get(&pdev->dev, NULL);
    if (IS_ERR(cdns_port->clk))
        return PTR_ERR(cdns_port->clk);

    ret = clk_prepare_enable(cdns_port->clk);
    if (ret)
        return ret;

    cdns_port->port.state = devm_kzalloc(&pdev->dev, sizeof(struct uart_state), GFP_KERNEL);
    if (!cdns_port->port.state)
        return -ENOMEM;

    cdns_port->port.state->xmit.buf = devm_kzalloc(&pdev->dev, UART_XMIT_SIZE, GFP_KERNEL);
    if (!cdns_port->port.state->xmit.buf)
        return -ENOMEM;

    tty_port_init(&cdns_port->port.state->port);
    platform_set_drvdata(pdev, cdns_port);

    writel(0x07, cdns_port->membase + CDNS_UART_IER);
    writel(1, cdns_port->membase + CDNS_UART_RXTL);

    ret = devm_request_irq(&pdev->dev, cdns_port->port.irq, cdns_uart_irq_handler, 0, "cdns_uart_rx", cdns_port);
    printk("Probe success in uart driver...\n");
    unsigned long clk_freq = clk_get_rate(cdns_port->clk);
    pr_info("UART Clock Rate: %lu\n", clk_freq);
    return ret;
}

static int cdns_uart_remove(struct platform_device *pdev)
{
    struct cdns_uart_port *cdns_port = platform_get_drvdata(pdev);
    clk_disable_unprepare(cdns_port->clk);
    return 0;
}

static const struct of_device_id cdns_uart_of_match[] = {
    { .compatible = "my_uart", },
    {}
};
MODULE_DEVICE_TABLE(of, cdns_uart_of_match);

static struct platform_driver cdns_uart_platform_driver = {
    .probe  = cdns_uart_probe,
    .remove = cdns_uart_remove,
    .driver = {
        .name = "cdns_uart_poll",
        .of_match_table = cdns_uart_of_match,
    },
};

static int __init cdns_uart_init(void)
{
    int ret = uart_register_driver(&cdns_uart_driver);
    if (ret)
        return ret;

    ret = platform_driver_register(&cdns_uart_platform_driver);
    if (ret)
        uart_unregister_driver(&cdns_uart_driver);

    return ret;
}

static void __exit cdns_uart_exit(void)
{
    platform_driver_unregister(&cdns_uart_platform_driver);
    uart_unregister_driver(&cdns_uart_driver);
}

module_init(cdns_uart_init);
module_exit(cdns_uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phani Kumar");
MODULE_DESCRIPTION("Cadence UART Driver without uart_add_one_port because tty device is not created by uart driver");

