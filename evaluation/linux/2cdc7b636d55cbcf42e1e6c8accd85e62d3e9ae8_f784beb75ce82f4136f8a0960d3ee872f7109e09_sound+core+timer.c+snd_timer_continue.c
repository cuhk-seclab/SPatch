int snd_timer_continue(struct snd_timer_instance *timeri)
{
	struct snd_timer *timer;
	int result = -EINVAL;
	unsigned long flags;

	if (timeri == NULL)
		return result;
	if (timeri->flags & SNDRV_TIMER_IFLG_SLAVE)
		return snd_timer_start_slave(timeri);
	timer = timeri->timer;
	if (! timer)
		return -EINVAL;
	if (timer->card && timer->card->shutdown)
		return -ENODEV;
	spin_lock_irqsave(&timer->lock, flags);
	if (timeri->flags & SNDRV_TIMER_IFLG_RUNNING) {
		result = -EBUSY;
		goto unlock;
	}
	if (!timeri->cticks)
		timeri->cticks = 1;
	timeri->pticks = 0;
	result = snd_timer_start1(timer, timeri, timer->sticks);
 unlock:
	spin_unlock_irqrestore(&timer->lock, flags);
	snd_timer_notify1(timeri, SNDRV_TIMER_EVENT_CONTINUE);
	return result;
}
