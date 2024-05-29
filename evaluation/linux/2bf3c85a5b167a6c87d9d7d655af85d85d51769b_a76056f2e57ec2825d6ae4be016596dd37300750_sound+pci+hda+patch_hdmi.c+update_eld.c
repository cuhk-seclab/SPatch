static void update_eld(struct hda_codec *codec,
		       struct hdmi_spec_per_pin *per_pin,
		       struct hdmi_eld *eld)
{
	struct hdmi_eld *pin_eld = &per_pin->sink_eld;
	struct hdmi_spec *spec = codec->spec;
	bool old_eld_valid = pin_eld->eld_valid;
	bool eld_changed;

	if (spec->dyn_pcm_assign) {
		if (eld->eld_valid)
			hdmi_attach_hda_pcm(spec, per_pin);
		else
			hdmi_detach_hda_pcm(spec, per_pin);
	}

	if (eld->eld_valid)
		snd_hdmi_show_eld(codec, &eld->info);

	eld_changed = (pin_eld->eld_valid != eld->eld_valid);
	if (eld->eld_valid && pin_eld->eld_valid)
		if (pin_eld->eld_size != eld->eld_size ||
		    memcmp(pin_eld->eld_buffer, eld->eld_buffer,
			   eld->eld_size) != 0)
			eld_changed = true;

	pin_eld->eld_valid = eld->eld_valid;
	pin_eld->eld_size = eld->eld_size;
	if (eld->eld_valid)
		memcpy(pin_eld->eld_buffer, eld->eld_buffer, eld->eld_size);
	pin_eld->info = eld->info;

	/*
	 * Re-setup pin and infoframe. This is needed e.g. when
	 * - sink is first plugged-in
	 * - transcoder can change during stream playback on Haswell
	 *   and this can make HW reset converter selection on a pin.
	 */
	if (eld->eld_valid && !old_eld_valid && per_pin->setup) {
		if (is_haswell_plus(codec) || is_valleyview_plus(codec)) {
			intel_verify_pin_cvt_connect(codec, per_pin);
			intel_not_share_assigned_cvt(codec, per_pin->pin_nid,
						     per_pin->mux_idx);
		}

		hdmi_setup_audio_infoframe(codec, per_pin, per_pin->non_pcm);
	}

	if (eld_changed)
		snd_ctl_notify(codec->card,
			       SNDRV_CTL_EVENT_MASK_VALUE |
			       SNDRV_CTL_EVENT_MASK_INFO,
			       &per_pin->eld_ctl->id);
}
