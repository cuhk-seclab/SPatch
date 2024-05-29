static int
pcm_playback_hw_params(struct snd_pcm_substream *substream,
		       struct snd_pcm_hw_params *hw_params)
{
	struct snd_bebob *bebob = substream->private_data;
	int err;

	err = snd_pcm_lib_alloc_vmalloc_buffer(substream,
					       params_buffer_bytes(hw_params));
	if (err < 0)
		return err;

	if (substream->runtime->status->state == SNDRV_PCM_STATE_OPEN) {
		mutex_lock(&bebob->mutex);
		bebob->substreams_counter++;
		mutex_unlock(&bebob->mutex);
	}

	amdtp_am824_set_pcm_format(&bebob->rx_stream, params_format(hw_params));

	return 0;
}
