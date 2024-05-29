int sctp_transport_hold(struct sctp_transport *transport)
{
	return atomic_add_unless(&transport->refcnt, 1, 0);
}
