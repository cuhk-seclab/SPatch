int snd_hda_gen_build_pcms(struct hda_codec *codec)
{
	struct hda_gen_spec *spec = codec->spec;
	struct hda_pcm *info;
	bool have_multi_adcs;

	if (spec->no_analog)
		goto skip_analog;

	fill_pcm_stream_name(spec->stream_name_analog,
			     sizeof(spec->stream_name_analog),
			     " Analog", codec->chip_name);
	info = snd_hda_codec_pcm_new(codec, "%s", spec->stream_name_analog);
	if (!info)
		return -ENOMEM;
	spec->pcm_rec[0] = info;

	if (spec->multiout.num_dacs > 0) {
		setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_PLAYBACK],
				 &pcm_analog_playback,
				 spec->stream_analog_playback,
				 spec->multiout.dac_nids[0]);
		info->stream[SNDRV_PCM_STREAM_PLAYBACK].channels_max =
			spec->multiout.max_channels;
		if (spec->autocfg.line_out_type == AUTO_PIN_SPEAKER_OUT &&
		    spec->autocfg.line_outs == 2)
			info->stream[SNDRV_PCM_STREAM_PLAYBACK].chmap =
				snd_pcm_2_1_chmaps;
	}
	if (spec->num_adc_nids) {
		setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_CAPTURE],
				 (spec->dyn_adc_switch ?
				  &dyn_adc_pcm_analog_capture : &pcm_analog_capture),
				 spec->stream_analog_capture,
				 spec->adc_nids[0]);
	}

 skip_analog:
	/* SPDIF for stream index #1 */
	if (spec->multiout.dig_out_nid || spec->dig_in_nid) {
		fill_pcm_stream_name(spec->stream_name_digital,
				     sizeof(spec->stream_name_digital),
				     " Digital", codec->chip_name);
		info = snd_hda_codec_pcm_new(codec, "%s",
					     spec->stream_name_digital);
		if (!info)
			return -ENOMEM;
		codec->slave_dig_outs = spec->multiout.slave_dig_outs;
		spec->pcm_rec[1] = info;
		if (spec->dig_out_type)
			info->pcm_type = spec->dig_out_type;
		else
			info->pcm_type = HDA_PCM_TYPE_SPDIF;
		if (spec->multiout.dig_out_nid)
			setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_PLAYBACK],
					 &pcm_digital_playback,
					 spec->stream_digital_playback,
					 spec->multiout.dig_out_nid);
		if (spec->dig_in_nid)
			setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_CAPTURE],
					 &pcm_digital_capture,
					 spec->stream_digital_capture,
					 spec->dig_in_nid);
	}

	if (spec->no_analog)
		return 0;

	/* If the use of more than one ADC is requested for the current
	 * model, configure a second analog capture-only PCM.
	 */
	have_multi_adcs = (spec->num_adc_nids > 1) &&
		!spec->dyn_adc_switch && !spec->auto_mic;
	/* Additional Analaog capture for index #2 */
	if (spec->alt_dac_nid || have_multi_adcs) {
		fill_pcm_stream_name(spec->stream_name_alt_analog,
				     sizeof(spec->stream_name_alt_analog),
			     " Alt Analog", codec->chip_name);
		info = snd_hda_codec_pcm_new(codec, "%s",
					     spec->stream_name_alt_analog);
		if (!info)
			return -ENOMEM;
		spec->pcm_rec[2] = info;
		if (spec->alt_dac_nid)
			setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_PLAYBACK],
					 &pcm_analog_alt_playback,
					 spec->stream_analog_alt_playback,
					 spec->alt_dac_nid);
		else
			setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_PLAYBACK],
					 &pcm_null_stream, NULL, 0);
		if (have_multi_adcs) {
			setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_CAPTURE],
					 &pcm_analog_alt_capture,
					 spec->stream_analog_alt_capture,
					 spec->adc_nids[1]);
			info->stream[SNDRV_PCM_STREAM_CAPTURE].substreams =
				spec->num_adc_nids - 1;
		} else {
			setup_pcm_stream(&info->stream[SNDRV_PCM_STREAM_CAPTURE],
					 &pcm_null_stream, NULL, 0);
		}
	}

	return 0;
}
