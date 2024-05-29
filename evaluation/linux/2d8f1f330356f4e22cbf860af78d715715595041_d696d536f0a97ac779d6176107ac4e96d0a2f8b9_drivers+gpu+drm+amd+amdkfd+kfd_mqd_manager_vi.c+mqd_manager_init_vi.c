struct mqd_manager *mqd_manager_init_vi(enum KFD_MQD_TYPE type,
		struct kfd_dev *dev)
{
	struct mqd_manager *mqd;

	BUG_ON(!dev);
	BUG_ON(type >= KFD_MQD_TYPE_MAX);

	pr_debug("kfd: In func %s\n", __func__);

	mqd = kzalloc(sizeof(struct mqd_manager), GFP_KERNEL);
	if (!mqd)
		return NULL;

	mqd->dev = dev;

	switch (type) {
	case KFD_MQD_TYPE_CP:
	case KFD_MQD_TYPE_COMPUTE:
		mqd->init_mqd = init_mqd;
		mqd->uninit_mqd = uninit_mqd;
		mqd->load_mqd = load_mqd;
		mqd->update_mqd = update_mqd;
		mqd->destroy_mqd = destroy_mqd;
		mqd->is_occupied = is_occupied;
		break;
	case KFD_MQD_TYPE_HIQ:
		mqd->init_mqd = init_mqd_hiq;
		mqd->uninit_mqd = uninit_mqd;
		mqd->load_mqd = load_mqd;
		mqd->update_mqd = update_mqd_hiq;
		mqd->destroy_mqd = destroy_mqd;
		mqd->is_occupied = is_occupied;
		break;
	case KFD_MQD_TYPE_SDMA:
		break;
	default:
		kfree(mqd);
		return NULL;
	}

	return mqd;
}
