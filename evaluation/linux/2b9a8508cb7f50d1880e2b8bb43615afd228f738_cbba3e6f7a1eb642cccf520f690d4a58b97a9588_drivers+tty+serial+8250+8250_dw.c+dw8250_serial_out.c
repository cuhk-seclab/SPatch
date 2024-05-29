static void dw8250_serial_out(struct uart_port *p, int offset, int value)
{
	writeb(value, p->membase + (offset << p->regshift));

	/* Make sure LCR write wasn't ignored */
	if (offset == UART_LCR) {
		int tries = 1000;
		while (tries--) {
			unsigned int lcr = p->serial_in(p, UART_LCR);
			if ((value & ~UART_LCR_SPAR) == (lcr & ~UART_LCR_SPAR))
				return;
			dw8250_force_idle(p);
			writeb(value, p->membase + (UART_LCR << p->regshift));
		}
		/*
		 * FIXME: this deadlocks if port->lock is already held
		 * dev_err(p->dev, "Couldn't set LCR to %d\n", value);
		 */
	}
}
