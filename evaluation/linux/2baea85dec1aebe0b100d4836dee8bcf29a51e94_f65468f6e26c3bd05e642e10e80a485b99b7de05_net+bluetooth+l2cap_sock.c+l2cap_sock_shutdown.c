static int l2cap_sock_shutdown(struct socket *sock, int how)
{
	struct sock *sk = sock->sk;
	struct l2cap_chan *chan;
	struct l2cap_conn *conn;
	int err = 0;

	BT_DBG("sock %p, sk %p", sock, sk);

	if (!sk)
		return 0;

	/* prevent sk structure from being freed whilst unlocked */
	sock_hold(sk);

	chan = l2cap_pi(sk)->chan;
	/* prevent chan structure from being freed whilst unlocked */
	l2cap_chan_hold(chan);
	conn = chan->conn;

	BT_DBG("chan %p state %s", chan, state_to_string(chan->state));

	if (conn)
		mutex_lock(&conn->chan_lock);

	l2cap_chan_lock(chan);
	lock_sock(sk);

	if (!sk->sk_shutdown) {
		if (chan->mode == L2CAP_MODE_ERTM &&
		    chan->unacked_frames > 0 &&
		    chan->state == BT_CONNECTED)
			err = __l2cap_wait_ack(sk, chan);

		sk->sk_shutdown = SHUTDOWN_MASK;

		release_sock(sk);
		l2cap_chan_close(chan, 0);
		lock_sock(sk);

		if (sock_flag(sk, SOCK_LINGER) && sk->sk_lingertime &&
		    !(current->flags & PF_EXITING))
			err = bt_sock_wait_state(sk, BT_CLOSED,
						 sk->sk_lingertime);
	}

	if (!err && sk->sk_err)
		err = -sk->sk_err;

	release_sock(sk);
	l2cap_chan_unlock(chan);

	if (conn)
		mutex_unlock(&conn->chan_lock);

	l2cap_chan_put(chan);
	sock_put(sk);

	return err;
}
