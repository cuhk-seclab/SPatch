struct sctp_association *sctp_unpack_cookie(
	const struct sctp_endpoint *ep,
	const struct sctp_association *asoc,
	struct sctp_chunk *chunk, gfp_t gfp,
	int *error, struct sctp_chunk **errp)
{
	struct sctp_association *retval = NULL;
	struct sctp_signed_cookie *cookie;
	struct sctp_cookie *bear_cookie;
	int headersize, bodysize, fixed_size;
	__u8 *digest = ep->digest;
	unsigned int len;
	sctp_scope_t scope;
	struct sk_buff *skb = chunk->skb;
	ktime_t kt;

	/* Header size is static data prior to the actual cookie, including
	 * any padding.
	 */
	headersize = sizeof(sctp_chunkhdr_t) +
		     (sizeof(struct sctp_signed_cookie) -
		      sizeof(struct sctp_cookie));
	bodysize = ntohs(chunk->chunk_hdr->length) - headersize;
	fixed_size = headersize + sizeof(struct sctp_cookie);

	/* Verify that the chunk looks like it even has a cookie.
	 * There must be enough room for our cookie and our peer's
	 * INIT chunk.
	 */
	len = ntohs(chunk->chunk_hdr->length);
	if (len < fixed_size + sizeof(struct sctp_chunkhdr))
		goto malformed;

	/* Verify that the cookie has been padded out. */
	if (bodysize % SCTP_COOKIE_MULTIPLE)
		goto malformed;

	/* Process the cookie.  */
	cookie = chunk->subh.cookie_hdr;
	bear_cookie = &cookie->c;

	if (!sctp_sk(ep->base.sk)->hmac)
		goto no_hmac;

	/* Check the signature.  */
		SHASH_DESC_ON_STACK(desc, sctp_sk(ep->base.sk)->hmac);
		int err;

		desc->tfm = sctp_sk(ep->base.sk)->hmac;
		desc->flags = 0;

		err = crypto_shash_setkey(desc->tfm, ep->secret_key,
					  sizeof(ep->secret_key)) ?:
		      crypto_shash_digest(desc, (u8 *)bear_cookie, bodysize,
					  digest);
		shash_desc_zero(desc);

		if (err) {
			*error = -SCTP_IERROR_NOMEM;
			goto fail;
	sg_init_one(&sg, bear_cookie, bodysize);
		goto fail;
	}

	if (memcmp(digest, cookie->signature, SCTP_SIGNATURE_SIZE)) {
		*error = -SCTP_IERROR_BAD_SIG;
		goto fail;
	}

no_hmac:
	/* IG Section 2.35.2:
	 *  3) Compare the port numbers and the verification tag contained
	 *     within the COOKIE ECHO chunk to the actual port numbers and the
	 *     verification tag within the SCTP common header of the received
	 *     packet. If these values do not match the packet MUST be silently
	 *     discarded,
	 */
	if (ntohl(chunk->sctp_hdr->vtag) != bear_cookie->my_vtag) {
		*error = -SCTP_IERROR_BAD_TAG;
		goto fail;
	}

	if (chunk->sctp_hdr->source != bear_cookie->peer_addr.v4.sin_port ||
	    ntohs(chunk->sctp_hdr->dest) != bear_cookie->my_port) {
		*error = -SCTP_IERROR_BAD_PORTS;
		goto fail;
	}

	/* Check to see if the cookie is stale.  If there is already
	 * an association, there is no need to check cookie's expiration
	 * for init collision case of lost COOKIE ACK.
	 * If skb has been timestamped, then use the stamp, otherwise
	 * use current time.  This introduces a small possibility that
	 * that a cookie may be considered expired, but his would only slow
	 * down the new association establishment instead of every packet.
	 */
	if (sock_flag(ep->base.sk, SOCK_TIMESTAMP))
		kt = skb_get_ktime(skb);
	else
		kt = ktime_get_real();

	if (!asoc && ktime_before(bear_cookie->expiration, kt)) {
		/*
		 * Section 3.3.10.3 Stale Cookie Error (3)
		 *
		 * Cause of error
		 * ---------------
		 * Stale Cookie Error:  Indicates the receipt of a valid State
		 * Cookie that has expired.
		 */
		len = ntohs(chunk->chunk_hdr->length);
		*errp = sctp_make_op_error_space(asoc, chunk, len);
		if (*errp) {
			suseconds_t usecs = ktime_to_us(ktime_sub(kt, bear_cookie->expiration));
			__be32 n = htonl(usecs);

			sctp_init_cause(*errp, SCTP_ERROR_STALE_COOKIE,
					sizeof(n));
			sctp_addto_chunk(*errp, sizeof(n), &n);
			*error = -SCTP_IERROR_STALE_COOKIE;
		} else
			*error = -SCTP_IERROR_NOMEM;

		goto fail;
	}

	/* Make a new base association.  */
	scope = sctp_scope(sctp_source(chunk));
	retval = sctp_association_new(ep, ep->base.sk, scope, gfp);
	if (!retval) {
		*error = -SCTP_IERROR_NOMEM;
		goto fail;
	}

	/* Set up our peer's port number.  */
	retval->peer.port = ntohs(chunk->sctp_hdr->source);

	/* Populate the association from the cookie.  */
	memcpy(&retval->c, bear_cookie, sizeof(*bear_cookie));

	if (sctp_assoc_set_bind_addr_from_cookie(retval, bear_cookie,
						 GFP_ATOMIC) < 0) {
		*error = -SCTP_IERROR_NOMEM;
		goto fail;
	}

	/* Also, add the destination address. */
	if (list_empty(&retval->base.bind_addr.address_list)) {
		sctp_add_bind_addr(&retval->base.bind_addr, &chunk->dest,
				SCTP_ADDR_SRC, GFP_ATOMIC);
	}

	retval->next_tsn = retval->c.initial_tsn;
	retval->ctsn_ack_point = retval->next_tsn - 1;
	retval->addip_serial = retval->c.initial_tsn;
	retval->adv_peer_ack_point = retval->ctsn_ack_point;
	retval->peer.prsctp_capable = retval->c.prsctp_capable;
	retval->peer.adaptation_ind = retval->c.adaptation_ind;

	/* The INIT stuff will be done by the side effects.  */
	return retval;

fail:
	if (retval)
		sctp_association_free(retval);

	return NULL;

malformed:
	/* Yikes!  The packet is either corrupt or deliberately
	 * malformed.
	 */
	*error = -SCTP_IERROR_MALFORMED;
	goto fail;
}
