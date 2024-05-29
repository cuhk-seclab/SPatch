static int psmouse_extensions(struct psmouse *psmouse,
			      unsigned int max_proto, bool set_properties)
{
	bool synaptics_hardware = false;

	/*
	 * Always check for focaltech, this is safe as it uses pnp-id
	 * matching.
	 */
	if (psmouse_do_detect(focaltech_detect, psmouse, set_properties) == 0) {
		if (max_proto > PSMOUSE_IMEX) {
			if (IS_ENABLED(CONFIG_MOUSE_PS2_FOCALTECH) &&
			    (!set_properties || focaltech_init(psmouse) == 0)) {
				return PSMOUSE_FOCALTECH;
			}
		}
		/*
		 * Restrict psmouse_max_proto so that psmouse_initialize()
		 * does not try to reset rate and resolution, because even
		 * that upsets the device.
		 * This also causes us to basically fall through to basic
		 * protocol detection, where we fully reset the mouse,
		 * and set it up as bare PS/2 protocol device.
		 */
		psmouse_max_proto = max_proto = PSMOUSE_PS2;
	}

	/*
	 * We always check for LifeBook because it does not disturb mouse
	 * (it only checks DMI information).
	 */
	if (psmouse_do_detect(lifebook_detect, psmouse, set_properties) == 0) {
		if (max_proto > PSMOUSE_IMEX) {
			if (!set_properties || lifebook_init(psmouse) == 0)
				return PSMOUSE_LIFEBOOK;
		}
	}

	if (psmouse_do_detect(vmmouse_detect, psmouse, set_properties) == 0) {
		if (max_proto > PSMOUSE_IMEX) {
			if (!set_properties || vmmouse_init(psmouse) == 0)
				return PSMOUSE_VMMOUSE;
		}
	}

	/*
	 * Try Kensington ThinkingMouse (we try first, because Synaptics
	 * probe upsets the ThinkingMouse).
	 */
	if (max_proto > PSMOUSE_IMEX &&
	    psmouse_do_detect(thinking_detect, psmouse, set_properties) == 0) {
		return PSMOUSE_THINKPS;
	}

	/*
	 * Try Synaptics TouchPad. Note that probing is done even if
	 * Synaptics protocol support is disabled in config - we need to
	 * know if it is Synaptics so we can reset it properly after
	 * probing for IntelliMouse.
	 */
	if (max_proto > PSMOUSE_PS2 &&
	    psmouse_do_detect(synaptics_detect, psmouse, set_properties) == 0) {
		synaptics_hardware = true;

		if (max_proto > PSMOUSE_IMEX) {
			/*
			 * Try activating protocol, but check if support is
			 * enabled first, since we try detecting Synaptics
			 * even when protocol is disabled.
			 */
			if (IS_ENABLED(CONFIG_MOUSE_PS2_SYNAPTICS) &&
			    (!set_properties || synaptics_init(psmouse) == 0)) {
				return PSMOUSE_SYNAPTICS;
			}

			/*
			 * Some Synaptics touchpads can emulate extended
			 * protocols (like IMPS/2).  Unfortunately
			 * Logitech/Genius probes confuse some firmware
			 * versions so we'll have to skip them.
			 */
			max_proto = PSMOUSE_IMEX;
		}

		/*
		 * Make sure that touchpad is in relative mode, gestures
		 * (taps) are enabled.
		 */
		synaptics_reset(psmouse);
	}

	/*
	 * Try Cypress Trackpad. We must try it before Finger Sensing Pad
	 * because Finger Sensing Pad probe upsets some modules of Cypress
	 * Trackpads.
	 */
	if (max_proto > PSMOUSE_IMEX &&
	    psmouse_do_detect(cypress_detect, psmouse, set_properties) == 0) {
		if (!set_properties || cypress_init(psmouse) == 0)
			return PSMOUSE_CYPRESS;

		/*
		 * Finger Sensing Pad probe upsets some modules of
		 * Cypress Trackpad, must avoid Finger Sensing Pad
		 * probe if Cypress Trackpad device detected.
		 */
		max_proto = PSMOUSE_IMEX;
	}

	/* Try ALPS TouchPad */
	if (max_proto > PSMOUSE_IMEX) {
		ps2_command(&psmouse->ps2dev, NULL, PSMOUSE_CMD_RESET_DIS);
		if (psmouse_do_detect(alps_detect,
				      psmouse, set_properties) == 0) {
			if (!set_properties || alps_init(psmouse) == 0)
				return PSMOUSE_ALPS;

			/* Init failed, try basic relative protocols */
			max_proto = PSMOUSE_IMEX;
		}
	}

	/* Try OLPC HGPK touchpad */
	if (max_proto > PSMOUSE_IMEX &&
	    psmouse_do_detect(hgpk_detect, psmouse, set_properties) == 0) {
		if (!set_properties || hgpk_init(psmouse) == 0)
			return PSMOUSE_HGPK;
			/* Init failed, try basic relative protocols */
		max_proto = PSMOUSE_IMEX;
	}

	/* Try Elantech touchpad */
	if (max_proto > PSMOUSE_IMEX &&
	    psmouse_do_detect(elantech_detect, psmouse, set_properties) == 0) {
		if (!set_properties || elantech_init(psmouse) == 0)
			return PSMOUSE_ELANTECH;
		/* Init failed, try basic relative protocols */
		max_proto = PSMOUSE_IMEX;
	}

	if (max_proto > PSMOUSE_IMEX) {
		if (psmouse_do_detect(genius_detect,
				      psmouse, set_properties) == 0)
			return PSMOUSE_GENPS;

		if (psmouse_do_detect(ps2pp_init,
				      psmouse, set_properties) == 0)
			return PSMOUSE_PS2PP;

		if (psmouse_do_detect(trackpoint_detect,
				      psmouse, set_properties) == 0)
			return PSMOUSE_TRACKPOINT;

		if (psmouse_do_detect(touchkit_ps2_detect,
				      psmouse, set_properties) == 0)
			return PSMOUSE_TOUCHKIT_PS2;
	}

	/*
	 * Try Finger Sensing Pad. We do it here because its probe upsets
	 * Trackpoint devices (causing TP_READ_ID command to time out).
	 */
	if (max_proto > PSMOUSE_IMEX) {
		if (psmouse_do_detect(fsp_detect,
				      psmouse, set_properties) == 0) {
			if (!set_properties || fsp_init(psmouse) == 0)
				return PSMOUSE_FSP;
			/* Init failed, try basic relative protocols */
			max_proto = PSMOUSE_IMEX;
		}
	}

	/*
	 * Reset to defaults in case the device got confused by extended
	 * protocol probes. Note that we follow up with full reset because
	 * some mice put themselves to sleep when they see PSMOUSE_RESET_DIS.
	 */
	ps2_command(&psmouse->ps2dev, NULL, PSMOUSE_CMD_RESET_DIS);
	psmouse_reset(psmouse);

	if (max_proto >= PSMOUSE_IMEX &&
	    psmouse_do_detect(im_explorer_detect,
			      psmouse, set_properties) == 0) {
		return PSMOUSE_IMEX;
	}

	if (max_proto >= PSMOUSE_IMPS &&
	    psmouse_do_detect(intellimouse_detect,
			      psmouse, set_properties) == 0) {
		return PSMOUSE_IMPS;
	}

	/*
	 * Okay, all failed, we have a standard mouse here. The number of
	 * the buttons is still a question, though. We assume 3.
	 */
	psmouse_do_detect(ps2bare_detect, psmouse, set_properties);

	if (synaptics_hardware) {
		/*
		 * We detected Synaptics hardware but it did not respond to
		 * IMPS/2 probes.  We need to reset the touchpad because if
		 * there is a track point on the pass through port it could
		 * get disabled while probing for protocol extensions.
		 */
		psmouse_reset(psmouse);
	}

	return PSMOUSE_PS2;
}
