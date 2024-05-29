void snd_bebob_stream_stop_duplex(struct snd_bebob *bebob)
{
	struct amdtp_stream *master, *slave;

	if (bebob->master == &bebob->rx_stream) {
		slave  = &bebob->tx_stream;
		master = &bebob->rx_stream;
	} else {
		slave  = &bebob->rx_stream;
		master = &bebob->tx_stream;
	}

	if (bebob->substreams_counter == 0) {
		amdtp_stream_pcm_abort(master);
		amdtp_stream_stop(master);

		amdtp_stream_pcm_abort(slave);
		amdtp_stream_stop(slave);

		break_both_connections(bebob);
	}
}
