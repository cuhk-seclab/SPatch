static int update_netprio(const void *v, struct file *file, unsigned n)
{
	int err;
	struct socket *sock = sock_from_file(file, &err);
	if (sock) {
		spin_lock(&cgroup_sk_update_lock);
		sock_cgroup_set_prioidx(&sock->sk->sk_cgrp_data,
					(unsigned long)v);
		spin_unlock(&cgroup_sk_update_lock);
	}
	return 0;
}
