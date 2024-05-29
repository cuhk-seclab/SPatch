void tty_write_message(struct tty_struct *tty, char *msg)
{
	if (tty) {
		mutex_lock(&tty->atomic_write_lock);
		tty_lock(tty);
		if (tty->ops->write && tty->count > 0)
			tty->ops->write(tty, msg, strlen(msg));
		tty_unlock(tty);
		tty_write_unlock(tty);
	}
	return;
}
