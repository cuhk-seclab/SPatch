static struct lu_env *cl_env_new(__u32 ctx_tags, __u32 ses_tags, void *debug)
{
	struct lu_env *env;
	struct cl_env *cle;

	OBD_SLAB_ALLOC_PTR_GFP(cle, cl_env_kmem, GFP_NOFS);
	if (cle != NULL) {
		int rc;

		INIT_LIST_HEAD(&cle->ce_linkage);
		cle->ce_magic = &cl_env_init0;
		env = &cle->ce_lu;
		rc = lu_env_init(env, ctx_tags | LCT_CL_THREAD);
		if (rc == 0) {
			rc = lu_context_init(&cle->ce_ses,
					     ses_tags | LCT_SESSION);
			if (rc == 0) {
				lu_context_enter(&cle->ce_ses);
				env->le_ses = &cle->ce_ses;
				cl_env_init0(cle, debug);
			} else
				lu_env_fini(env);
		}
		if (rc != 0) {
			OBD_SLAB_FREE_PTR(cle, cl_env_kmem);
			env = ERR_PTR(rc);
		} else {
			CL_ENV_INC(create);
			CL_ENV_INC(total);
		}
	} else
		env = ERR_PTR(-ENOMEM);
	return env;
}
