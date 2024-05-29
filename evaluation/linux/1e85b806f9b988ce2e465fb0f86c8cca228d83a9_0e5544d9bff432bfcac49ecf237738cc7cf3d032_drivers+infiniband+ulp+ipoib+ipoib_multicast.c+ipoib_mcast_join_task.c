void ipoib_mcast_join_task(struct work_struct *work)
{
	struct ipoib_dev_priv *priv =
		container_of(work, struct ipoib_dev_priv, mcast_task.work);
	struct net_device *dev = priv->dev;
	struct ib_port_attr port_attr;
	unsigned long delay_until = 0;
	struct ipoib_mcast *mcast = NULL;
	int create = 1;

	if (!test_bit(IPOIB_FLAG_OPER_UP, &priv->flags))
		return;

	if (ib_query_port(priv->ca, priv->port, &port_attr) ||
	    port_attr.state != IB_PORT_ACTIVE) {
		ipoib_dbg(priv, "port state is not ACTIVE (state = %d) suspending join task\n",
			  port_attr.state);
		return;
	}
	priv->local_lid = port_attr.lid;

	if (ib_query_gid(priv->ca, priv->port, 0, &priv->local_gid))
		ipoib_warn(priv, "ib_query_gid() failed\n");
	else
		memcpy(priv->dev->dev_addr + 4, priv->local_gid.raw, sizeof (union ib_gid));

	spin_lock_irq(&priv->lock);
	if (!test_bit(IPOIB_FLAG_OPER_UP, &priv->flags))
		goto out;

	if (!priv->broadcast) {
		struct ipoib_mcast *broadcast;

		broadcast = ipoib_mcast_alloc(dev, 0);
		if (!broadcast) {
			ipoib_warn(priv, "failed to allocate broadcast group\n");
			/*
			 * Restart us after a 1 second delay to retry
			 * creating our broadcast group and attaching to
			 * it.  Until this succeeds, this ipoib dev is
			 * completely stalled (multicast wise).
			 */
			__ipoib_mcast_schedule_join_thread(priv, NULL, 1);
			goto out;
		}

		memcpy(broadcast->mcmember.mgid.raw, priv->dev->broadcast + 4,
		       sizeof (union ib_gid));
		priv->broadcast = broadcast;

		__ipoib_mcast_add(dev, priv->broadcast);
	}

	if (!test_bit(IPOIB_MCAST_FLAG_ATTACHED, &priv->broadcast->flags)) {
		if (IS_ERR_OR_NULL(priv->broadcast->mc) &&
		    !test_bit(IPOIB_MCAST_FLAG_BUSY, &priv->broadcast->flags)) {
			mcast = priv->broadcast;
			create = 0;
			if (mcast->backoff > 1 &&
			    time_before(jiffies, mcast->delay_until)) {
				delay_until = mcast->delay_until;
				mcast = NULL;
			}
		}
		goto out;
	}

	/*
	 * We'll never get here until the broadcast group is both allocated
	 * and attached
	 */
	list_for_each_entry(mcast, &priv->multicast_list, list) {
		if (IS_ERR_OR_NULL(mcast->mc) &&
		    !test_bit(IPOIB_MCAST_FLAG_BUSY, &mcast->flags) &&
		    (!test_bit(IPOIB_MCAST_FLAG_SENDONLY, &mcast->flags) ||
		     !skb_queue_empty(&mcast->pkt_queue))) {
			if (mcast->backoff == 1 ||
			    time_after_eq(jiffies, mcast->delay_until)) {
				/* Found the next unjoined group */
				init_completion(&mcast->done);
				set_bit(IPOIB_MCAST_FLAG_BUSY, &mcast->flags);
				if (test_bit(IPOIB_MCAST_FLAG_SENDONLY, &mcast->flags))
					create = 0;
				else
					create = 1;
				spin_unlock_irq(&priv->lock);
				ipoib_mcast_join(dev, mcast, create);
				spin_lock_irq(&priv->lock);
			} else if (!delay_until ||
				 time_before(mcast->delay_until, delay_until))
				delay_until = mcast->delay_until;
		}
	}

	mcast = NULL;
	ipoib_dbg_mcast(priv, "successfully started all multicast joins\n");

out:
	if (delay_until) {
		cancel_delayed_work(&priv->mcast_task);
		queue_delayed_work(priv->wq, &priv->mcast_task,
				   delay_until - jiffies);
	}
	if (mcast) {
		init_completion(&mcast->done);
		set_bit(IPOIB_MCAST_FLAG_BUSY, &mcast->flags);
	}
	spin_unlock_irq(&priv->lock);
	if (mcast)
		ipoib_mcast_join(dev, mcast, create);
}
