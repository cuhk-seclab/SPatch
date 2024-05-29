int pciehp_sysfs_disable_slot(struct slot *p_slot)
{
	int retval = -ENODEV;
	struct controller *ctrl = p_slot->ctrl;

	mutex_lock(&p_slot->lock);
	switch (p_slot->state) {
	case BLINKINGOFF_STATE:
		cancel_delayed_work(&p_slot->work);
	case STATIC_STATE:
		p_slot->state = POWEROFF_STATE;
		mutex_unlock(&p_slot->lock);
		mutex_lock(&p_slot->hotplug_lock);
		retval = pciehp_disable_slot(p_slot);
		mutex_unlock(&p_slot->hotplug_lock);
		mutex_lock(&p_slot->lock);
		p_slot->state = STATIC_STATE;
		break;
	case POWEROFF_STATE:
		ctrl_info(ctrl, "Slot %s is already in powering off state\n",
			  slot_name(p_slot));
		break;
	case BLINKINGON_STATE:
	case POWERON_STATE:
		ctrl_info(ctrl, "Already disabled on slot %s\n",
			  slot_name(p_slot));
		break;
	default:
		ctrl_err(ctrl, "invalid state %#x on slot %s\n",
			 p_slot->state, slot_name(p_slot));
		break;
	}
	mutex_unlock(&p_slot->lock);

	return retval;
}
