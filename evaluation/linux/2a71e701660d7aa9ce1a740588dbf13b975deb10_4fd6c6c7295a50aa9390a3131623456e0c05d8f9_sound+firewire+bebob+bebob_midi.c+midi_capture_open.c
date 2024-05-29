static int midi_capture_open(struct snd_rawmidi_substream *substream)
{
	struct snd_bebob *bebob = substream->rmidi->private_data;
	int err;

	err = snd_bebob_stream_lock_try(bebob);
	if (err < 0)
		goto end;

	mutex_lock(&bebob->mutex);
	bebob->substreams_counter++;
	err = snd_bebob_stream_start_duplex(bebob, 0);
	mutex_unlock(&bebob->mutex);
	if (err < 0)
		snd_bebob_stream_lock_release(bebob);
end:
	return err;
}
