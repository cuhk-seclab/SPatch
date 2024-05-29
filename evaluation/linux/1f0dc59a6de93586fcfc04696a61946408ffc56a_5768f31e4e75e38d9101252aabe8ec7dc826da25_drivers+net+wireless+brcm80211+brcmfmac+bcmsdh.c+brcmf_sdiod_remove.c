static int brcmf_sdiod_remove(struct brcmf_sdio_dev *sdiodev)
{
	sdiodev->state = BRCMF_SDIOD_DOWN;
	if (sdiodev->bus) {
		brcmf_sdio_remove(sdiodev->bus);
		sdiodev->bus = NULL;
	}

	brcmf_sdiod_freezer_detach(sdiodev);

	/* Disable Function 2 */
	sdio_claim_host(sdiodev->func[2]);
	sdio_disable_func(sdiodev->func[2]);
	sdio_release_host(sdiodev->func[2]);

	/* Disable Function 1 */
	sdio_claim_host(sdiodev->func[1]);
	sdio_disable_func(sdiodev->func[1]);
	sdio_release_host(sdiodev->func[1]);

	sg_free_table(&sdiodev->sgtable);
	sdiodev->sbwad = 0;

	pm_runtime_allow(sdiodev->func[1]->card->host->parent);
	return 0;
}
