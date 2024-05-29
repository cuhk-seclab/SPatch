static struct sctp_association *__sctp_lookup_association(
					struct net *net,
					const union sctp_addr *local,
					const union sctp_addr *peer,
					struct sctp_transport **pt)
{
	struct sctp_transport *t;

	rcu_read_lock();
	t = sctp_addrs_lookup_transport(net, local, peer);
	if (!t || t->dead)
		return NULL;

	sctp_association_hold(t->asoc);
	*pt = t;


out:
	rcu_read_unlock();
	return t->asoc;
}
