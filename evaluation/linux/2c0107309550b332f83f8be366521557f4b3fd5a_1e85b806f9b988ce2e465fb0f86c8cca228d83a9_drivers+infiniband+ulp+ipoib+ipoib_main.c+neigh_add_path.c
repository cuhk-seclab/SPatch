static void neigh_add_path(struct sk_buff *skb, u8 *daddr,
			   struct net_device *dev)
{
	struct ipoib_dev_priv *priv = netdev_priv(dev);
	struct ipoib_path *path;
	struct ipoib_neigh *neigh;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	neigh = ipoib_neigh_alloc(daddr, dev);
	if (!neigh) {
		spin_unlock_irqrestore(&priv->lock, flags);
		++dev->stats.tx_dropped;
		dev_kfree_skb_any(skb);
		return;
	}

	path = __path_find(dev, daddr + 4);
	if (!path) {
		path = path_rec_create(dev, daddr + 4);
		if (!path)
			goto err_path;

		__path_add(dev, path);
	}

	list_add_tail(&neigh->list, &path->neigh_list);

	if (path->ah) {
		kref_get(&path->ah->ref);
		neigh->ah = path->ah;

		if (ipoib_cm_enabled(dev, neigh->daddr)) {
			if (!ipoib_cm_get(neigh))
				ipoib_cm_set(neigh, ipoib_cm_create_tx(dev, path, neigh));
			if (!ipoib_cm_get(neigh)) {
				ipoib_neigh_free(neigh);
				goto err_drop;
			}
			if (skb_queue_len(&neigh->queue) < IPOIB_MAX_PATH_REC_QUEUE)
				__skb_queue_tail(&neigh->queue, skb);
			else {
				ipoib_warn(priv, "queue length limit %d. Packet drop.\n",
					   skb_queue_len(&neigh->queue));
				goto err_drop;
			}
		} else {
			spin_unlock_irqrestore(&priv->lock, flags);
			ipoib_send(dev, skb, path->ah, IPOIB_QPN(daddr));
			ipoib_neigh_put(neigh);
			return;
		}
	} else {
		neigh->ah  = NULL;

		if (!path->query && path_rec_start(dev, path))
			goto err_path;
		if (skb_queue_len(&neigh->queue) < IPOIB_MAX_PATH_REC_QUEUE)
			__skb_queue_tail(&neigh->queue, skb);
		else
			goto err_drop;
	}

	spin_unlock_irqrestore(&priv->lock, flags);
	ipoib_neigh_put(neigh);
	return;

err_path:
	ipoib_neigh_free(neigh);
err_drop:
	++dev->stats.tx_dropped;
	dev_kfree_skb_any(skb);

	spin_unlock_irqrestore(&priv->lock, flags);
	ipoib_neigh_put(neigh);
}
