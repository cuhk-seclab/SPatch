static void sk_prot_free(struct proto *prot, struct sock *sk)
{
	struct kmem_cache *slab;
	struct module *owner;

	owner = prot->owner;
	slab = prot->slab;

	cgroup_sk_free(&sk->sk_cgrp_data);
	security_sk_free(sk);
	if (slab != NULL)
		kmem_cache_free(slab, sk);
	else
		kfree(sk);
	module_put(owner);
}
