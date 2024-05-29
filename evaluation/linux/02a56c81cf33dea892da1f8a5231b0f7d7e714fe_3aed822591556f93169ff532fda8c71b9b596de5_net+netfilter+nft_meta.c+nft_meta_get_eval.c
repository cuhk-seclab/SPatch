void nft_meta_get_eval(const struct nft_expr *expr,
		       struct nft_regs *regs,
		       const struct nft_pktinfo *pkt)
{
	const struct nft_meta *priv = nft_expr_priv(expr);
	const struct sk_buff *skb = pkt->skb;
	const struct net_device *in = pkt->in, *out = pkt->out;
	u32 *dest = &regs->data[priv->dreg];

	switch (priv->key) {
	case NFT_META_LEN:
		*dest = skb->len;
		break;
	case NFT_META_PROTOCOL:
		*dest = 0;
		*(__be16 *)dest = skb->protocol;
		break;
	case NFT_META_NFPROTO:
		*dest = pkt->pf;
		break;
	case NFT_META_L4PROTO:
		*dest = pkt->tprot;
		break;
	case NFT_META_PRIORITY:
		*dest = skb->priority;
		break;
	case NFT_META_MARK:
		*dest = skb->mark;
		break;
	case NFT_META_IIF:
		if (in == NULL)
			goto err;
		*dest = in->ifindex;
		break;
	case NFT_META_OIF:
		if (out == NULL)
			goto err;
		*dest = out->ifindex;
		break;
	case NFT_META_IIFNAME:
		if (in == NULL)
			goto err;
		strncpy((char *)dest, in->name, IFNAMSIZ);
		break;
	case NFT_META_OIFNAME:
		if (out == NULL)
			goto err;
		strncpy((char *)dest, out->name, IFNAMSIZ);
		break;
	case NFT_META_IIFTYPE:
		if (in == NULL)
			goto err;
		*dest = 0;
		*(u16 *)dest = in->type;
		break;
	case NFT_META_OIFTYPE:
		if (out == NULL)
			goto err;
		*dest = 0;
		*(u16 *)dest = out->type;
		break;
	case NFT_META_SKUID:
		if (skb->sk == NULL || !sk_fullsock(skb->sk))
			goto err;

		if (sk->sk_socket == NULL ||
		    sk->sk_socket->file == NULL) {
		read_lock_bh(&skb->sk->sk_callback_lock);
			read_unlock_bh(&skb->sk->sk_callback_lock);
			goto err;
		}

		*dest =	from_kuid_munged(&init_user_ns,
				skb->sk->sk_socket->file->f_cred->fsuid);
		read_unlock_bh(&skb->sk->sk_callback_lock);
		break;
	case NFT_META_SKGID:
		if (skb->sk == NULL || !sk_fullsock(skb->sk))
			goto err;

		if (sk->sk_socket == NULL ||
		    sk->sk_socket->file == NULL) {
		read_lock_bh(&skb->sk->sk_callback_lock);
			read_unlock_bh(&skb->sk->sk_callback_lock);
			goto err;
		}
		*dest =	from_kgid_munged(&init_user_ns,
				 skb->sk->sk_socket->file->f_cred->fsgid);
		read_unlock_bh(&skb->sk->sk_callback_lock);
		break;
#ifdef CONFIG_IP_ROUTE_CLASSID
	case NFT_META_RTCLASSID: {
		const struct dst_entry *dst = skb_dst(skb);

		if (dst == NULL)
			goto err;
		*dest = dst->tclassid;
		break;
	}
#endif
#ifdef CONFIG_NETWORK_SECMARK
	case NFT_META_SECMARK:
		*dest = skb->secmark;
		break;
#endif
	case NFT_META_PKTTYPE:
		if (skb->pkt_type != PACKET_LOOPBACK) {
			*dest = skb->pkt_type;
			break;
		}

		switch (pkt->pf) {
		case NFPROTO_IPV4:
			if (ipv4_is_multicast(ip_hdr(skb)->daddr))
				*dest = PACKET_MULTICAST;
			else
				*dest = PACKET_BROADCAST;
			break;
		case NFPROTO_IPV6:
			if (ipv6_hdr(skb)->daddr.s6_addr[0] == 0xFF)
				*dest = PACKET_MULTICAST;
			else
				*dest = PACKET_BROADCAST;
			break;
		default:
			WARN_ON(1);
			goto err;
		}
		break;
	case NFT_META_CPU:
		*dest = raw_smp_processor_id();
		break;
	case NFT_META_IIFGROUP:
		if (in == NULL)
			goto err;
		*dest = in->group;
		break;
	case NFT_META_OIFGROUP:
		if (out == NULL)
			goto err;
		*dest = out->group;
		break;
#ifdef CONFIG_CGROUP_NET_CLASSID
	case NFT_META_CGROUP:
		sk = skb_to_full_sk(skb);
		if (!sk || !sk_fullsock(sk))
			goto err;
		*dest = sk->sk_classid;
		break;
#endif
	default:
		WARN_ON(1);
		goto err;
	}
	return;

err:
	regs->verdict.code = NFT_BREAK;
}
