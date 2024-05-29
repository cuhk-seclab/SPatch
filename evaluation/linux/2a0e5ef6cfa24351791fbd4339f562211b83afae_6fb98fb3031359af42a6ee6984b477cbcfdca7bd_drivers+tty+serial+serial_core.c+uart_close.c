static void uart_close(struct tty_struct *tty, struct file *filp)
{
	struct uart_state *state = tty->driver_data;
	struct tty_port *port;
	struct uart_port *uport;

	if (!state) {
		struct uart_driver *drv = tty->driver->driver_state;

		state = drv->state + tty->index;
		port = &state->port;
		spin_lock_irq(&port->lock);
		--port->count;
		spin_unlock_irq(&port->lock);
		return;
	}

	uport = state->uart_port;
	port = &state->port;

	pr_debug("uart_close(%d) called\n", uport ? uport->line : -1);

	if (!port->count || tty_port_close_start(port, tty, filp) == 0)
		return;

	/*
	 * At this point, we stop accepting input.  To do this, we
	 * disable the receive line status interrupts.
	 */
	if (port->flags & ASYNC_INITIALIZED) {
		spin_lock_irq(&uport->lock);
		uport->ops->stop_rx(uport);
		spin_unlock_irq(&uport->lock);
		/*
		 * Before we drop DTR, make sure the UART transmitter
		 * has completely drained; this is especially
		 * important if there is a transmit FIFO!
		 */
		uart_wait_until_sent(tty, uport->timeout);
	}

	mutex_lock(&port->mutex);
	uart_shutdown(tty, state);
	tty_port_tty_set(port, NULL);

	spin_lock_irq(&port->lock);

	if (port->blocked_open) {
		spin_unlock_irq(&port->lock);
		if (port->close_delay)
			msleep_interruptible(jiffies_to_msecs(port->close_delay));
		spin_lock_irq(&port->lock);
	} else if (!uart_console(uport)) {
		spin_unlock_irq(&port->lock);
		uart_change_pm(state, UART_PM_STATE_OFF);
		spin_lock_irq(&port->lock);
	}

	/*
	 * Wake up anyone trying to open this port.
	 */
	clear_bit(ASYNCB_NORMAL_ACTIVE, &port->flags);
	clear_bit(ASYNCB_CLOSING, &port->flags);
	spin_unlock_irq(&port->lock);
	wake_up_interruptible(&port->open_wait);
	wake_up_interruptible(&port->close_wait);

	mutex_unlock(&port->mutex);

	tty_ldisc_flush(tty);
	tty->closing = 0;
}
