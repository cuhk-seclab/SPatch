static int kvm_vcpu_ioctl_x86_set_vcpu_events(struct kvm_vcpu *vcpu,
					      struct kvm_vcpu_events *events)
{
	if (events->flags & ~(KVM_VCPUEVENT_VALID_NMI_PENDING
			      | KVM_VCPUEVENT_VALID_SIPI_VECTOR
			      | KVM_VCPUEVENT_VALID_SHADOW
			      | KVM_VCPUEVENT_VALID_SMM))
		return -EINVAL;

	process_nmi(vcpu);
	vcpu->arch.exception.pending = events->exception.injected;
	vcpu->arch.exception.nr = events->exception.nr;
	vcpu->arch.exception.has_error_code = events->exception.has_error_code;
	vcpu->arch.exception.error_code = events->exception.error_code;

	vcpu->arch.interrupt.pending = events->interrupt.injected;
	vcpu->arch.interrupt.nr = events->interrupt.nr;
	vcpu->arch.interrupt.soft = events->interrupt.soft;
	if (events->flags & KVM_VCPUEVENT_VALID_SHADOW)
		kvm_x86_ops->set_interrupt_shadow(vcpu,
						  events->interrupt.shadow);

	vcpu->arch.nmi_injected = events->nmi.injected;
	if (events->flags & KVM_VCPUEVENT_VALID_NMI_PENDING)
		vcpu->arch.nmi_pending = events->nmi.pending;
	kvm_x86_ops->set_nmi_mask(vcpu, events->nmi.masked);

	if (events->flags & KVM_VCPUEVENT_VALID_SIPI_VECTOR &&
	    lapic_in_kernel(vcpu))
		vcpu->arch.apic->sipi_vector = events->sipi_vector;

	if (events->flags & KVM_VCPUEVENT_VALID_SMM) {
		if (events->smi.smm)
			vcpu->arch.hflags |= HF_SMM_MASK;
		else
			vcpu->arch.hflags &= ~HF_SMM_MASK;
		vcpu->arch.smi_pending = events->smi.pending;
		if (events->smi.smm_inside_nmi)
			vcpu->arch.hflags |= HF_SMM_INSIDE_NMI_MASK;
		else
			vcpu->arch.hflags &= ~HF_SMM_INSIDE_NMI_MASK;
		if (lapic_in_kernel(vcpu)) {
			if (events->smi.latched_init)
				set_bit(KVM_APIC_INIT, &vcpu->arch.apic->pending_events);
			else
				clear_bit(KVM_APIC_INIT, &vcpu->arch.apic->pending_events);
		}
	}

	kvm_make_request(KVM_REQ_EVENT, vcpu);

	return 0;
}
