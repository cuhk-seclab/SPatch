int mlx5_buf_alloc(struct mlx5_core_dev *dev, int size, struct mlx5_buf *buf)
{
	return mlx5_buf_alloc_node(dev, size, buf, dev->priv.numa_node);
}
