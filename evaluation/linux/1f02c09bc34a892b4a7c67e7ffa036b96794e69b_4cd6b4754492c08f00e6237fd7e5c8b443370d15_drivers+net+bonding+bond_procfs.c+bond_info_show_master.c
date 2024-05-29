static void bond_info_show_master(struct seq_file *seq)
{
	struct bonding *bond = seq->private;
	const struct bond_opt_value *optval;
	struct slave *curr, *primary;
	int i;

	curr = rcu_dereference(bond->curr_active_slave);

	seq_printf(seq, "Bonding Mode: %s",
		   bond_mode_name(BOND_MODE(bond)));

	if (BOND_MODE(bond) == BOND_MODE_ACTIVEBACKUP &&
	    bond->params.fail_over_mac) {
		optval = bond_opt_get_val(BOND_OPT_FAIL_OVER_MAC,
					  bond->params.fail_over_mac);
		seq_printf(seq, " (fail_over_mac %s)", optval->string);
	}

	seq_printf(seq, "\n");

	if (bond_mode_uses_xmit_hash(bond)) {
		optval = bond_opt_get_val(BOND_OPT_XMIT_HASH,
					  bond->params.xmit_policy);
		seq_printf(seq, "Transmit Hash Policy: %s (%d)\n",
			   optval->string, bond->params.xmit_policy);
	}

	if (bond_uses_primary(bond)) {
		primary = rcu_dereference(bond->primary_slave);
		seq_printf(seq, "Primary Slave: %s",
			   primary ? primary->dev->name : "None");
		if (primary) {
			optval = bond_opt_get_val(BOND_OPT_PRIMARY_RESELECT,
						  bond->params.primary_reselect);
			seq_printf(seq, " (primary_reselect %s)",
				   optval->string);
		}

		seq_printf(seq, "\nCurrently Active Slave: %s\n",
			   (curr) ? curr->dev->name : "None");
	}

	seq_printf(seq, "MII Status: %s\n", netif_carrier_ok(bond->dev) ?
		   "up" : "down");
	seq_printf(seq, "MII Polling Interval (ms): %d\n", bond->params.miimon);
	seq_printf(seq, "Up Delay (ms): %d\n",
		   bond->params.updelay * bond->params.miimon);
	seq_printf(seq, "Down Delay (ms): %d\n",
		   bond->params.downdelay * bond->params.miimon);


	/* ARP information */
	if (bond->params.arp_interval > 0) {
		int printed = 0;
		seq_printf(seq, "ARP Polling Interval (ms): %d\n",
				bond->params.arp_interval);

		seq_printf(seq, "ARP IP target/s (n.n.n.n form):");

		for (i = 0; (i < BOND_MAX_ARP_TARGETS); i++) {
			if (!bond->params.arp_targets[i])
				break;
			if (printed)
				seq_printf(seq, ",");
			seq_printf(seq, " %pI4", &bond->params.arp_targets[i]);
			printed = 1;
		}
		seq_printf(seq, "\n");
	}

	if (BOND_MODE(bond) == BOND_MODE_8023AD) {
		struct ad_info ad_info;

		seq_puts(seq, "\n802.3ad info\n");
		seq_printf(seq, "LACP rate: %s\n",
			   (bond->params.lacp_fast) ? "fast" : "slow");
		seq_printf(seq, "Min links: %d\n", bond->params.min_links);
		optval = bond_opt_get_val(BOND_OPT_AD_SELECT,
					  bond->params.ad_select);
		seq_printf(seq, "Aggregator selection policy (ad_select): %s\n",
			   optval->string);
		if (capable(CAP_NET_ADMIN)) {
			seq_printf(seq, "System priority: %d\n",
				   BOND_AD_INFO(bond).system.sys_priority);
			seq_printf(seq, "System MAC address: %pM\n",
				   &BOND_AD_INFO(bond).system.sys_mac_addr);

			if (__bond_3ad_get_active_agg_info(bond, &ad_info)) {
				seq_printf(seq,
					   "bond %s has no active aggregator\n",
					   bond->dev->name);
			} else {
				seq_printf(seq, "Active Aggregator Info:\n");

				seq_printf(seq, "\tAggregator ID: %d\n",
					   ad_info.aggregator_id);
				seq_printf(seq, "\tNumber of ports: %d\n",
					   ad_info.ports);
				seq_printf(seq, "\tActor Key: %d\n",
					   ad_info.actor_key);
				seq_printf(seq, "\tPartner Key: %d\n",
					   ad_info.partner_key);
				seq_printf(seq, "\tPartner Mac Address: %pM\n",
					   ad_info.partner_system);
			}
		}
	}
}
