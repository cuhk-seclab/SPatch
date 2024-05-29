void kvm_apic_accept_events(struct kvm_vcpu *vcpu)
{
	struct kvm_lapic *apic = vcpu->arch.apic;
	u8 sipi_vector;
	unsigned long pe;

	if (!lapic_in_kernel(vcpu) || !apic->pending_events)
		return;

	/*
	 * INITs are latched while in SMM.  Because an SMM CPU cannot
	 * be in KVM_MP_STATE_INIT_RECEIVED state, just eat SIPIs
	 * and delay processing of INIT until the next RSM.
	 */
	if (is_smm(vcpu)) {
		WARN_ON_ONCE(vcpu->arch.mp_state == KVM_MP_STATE_INIT_RECEIVED);
		if (test_bit(KVM_APIC_SIPI, &apic->pending_events))
			clear_bit(KVM_APIC_SIPI, &apic->pending_events);
		return;
	}

	pe = xchg(&apic->pending_events, 0);
	if (test_bit(KVM_APIC_INIT, &pe)) {
		kvm_lapic_reset(vcpu, true);
		kvm_vcpu_reset(vcpu, true);
		if (kvm_vcpu_is_bsp(apic->vcpu))
			vcpu->arch.mp_state = KVM_MP_STATE_RUNNABLE;
		else
			vcpu->arch.mp_state = KVM_MP_STATE_INIT_RECEIVED;
	}
	if (test_bit(KVM_APIC_SIPI, &pe) &&
	    vcpu->arch.mp_state == KVM_MP_STATE_INIT_RECEIVED) {
		/* evaluate pending_events before reading the vector */
		smp_rmb();
		sipi_vector = apic->sipi_vector;
		apic_debug("vcpu %d received sipi with vector # %x\n",
			 vcpu->vcpu_id, sipi_vector);
		kvm_vcpu_deliver_sipi_vector(vcpu, sipi_vector);
		vcpu->arch.mp_state = KVM_MP_STATE_RUNNABLE;
	}
}
