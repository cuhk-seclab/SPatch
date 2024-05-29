static int check_preds(struct filter_parse_state *ps)
{
	int n_normal_preds = 0, n_logical_preds = 0;
	struct postfix_elt *elt;
	int cnt = 0;

	list_for_each_entry(elt, &ps->postfix, list) {
		if (elt->op == OP_NONE) {
			cnt++;
			continue;
		}

		if (elt->op == OP_AND || elt->op == OP_OR) {
			n_logical_preds++;
			cnt--;
			continue;
		}
		if (elt->op != OP_NOT)
			cnt--;
		n_normal_preds++;
		/* all ops should have operands */
		if (cnt < 0)
			break;
	}

	if (cnt != 1 || !n_normal_preds || n_logical_preds >= n_normal_preds) {
		parse_error(ps, FILT_ERR_INVALID_FILTER, 0);
		return -EINVAL;
	}

	return 0;
}
