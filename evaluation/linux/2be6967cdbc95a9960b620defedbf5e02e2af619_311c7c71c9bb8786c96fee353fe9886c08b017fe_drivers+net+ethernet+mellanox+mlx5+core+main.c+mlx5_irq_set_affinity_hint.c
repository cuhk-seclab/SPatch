static int mlx5_irq_set_affinity_hint(struct mlx5_core_dev *mdev, int i)
{
	struct mlx5_priv *priv  = &mdev->priv;
	struct msix_entry *msix = priv->msix_arr;
	int irq                 = msix[i + MLX5_EQ_VEC_COMP_BASE].vector;
	int numa_node           = priv->numa_node;
	int err;

	if (!zalloc_cpumask_var(&priv->irq_info[i].mask, GFP_KERNEL)) {
		mlx5_core_warn(mdev, "zalloc_cpumask_var failed");
		return -ENOMEM;
	}

	cpumask_set_cpu(cpumask_local_spread(i, numa_node),
			priv->irq_info[i].mask);

	err = irq_set_affinity_hint(irq, priv->irq_info[i].mask);
	if (err) {
		mlx5_core_warn(mdev, "irq_set_affinity_hint failed,irq 0x%.4x",
			       irq);
		goto err_clear_mask;
	}

	return 0;

err_clear_mask:
	free_cpumask_var(priv->irq_info[i].mask);
	return err;
}
