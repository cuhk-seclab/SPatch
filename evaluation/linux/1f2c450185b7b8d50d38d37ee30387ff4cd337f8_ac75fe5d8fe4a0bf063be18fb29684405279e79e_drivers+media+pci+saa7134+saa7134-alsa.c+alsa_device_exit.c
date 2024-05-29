static int alsa_device_exit(struct saa7134_dev *dev)
{
	if (!snd_saa7134_cards[dev->nr])
		return 1;

	snd_card_free(snd_saa7134_cards[dev->nr]);
	snd_saa7134_cards[dev->nr] = NULL;
	return 1;
}
