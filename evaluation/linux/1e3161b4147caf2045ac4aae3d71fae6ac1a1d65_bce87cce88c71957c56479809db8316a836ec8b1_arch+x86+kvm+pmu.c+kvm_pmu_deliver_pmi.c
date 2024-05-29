void kvm_pmu_deliver_pmi(struct kvm_vcpu *vcpu)
{
	if (lapic_in_kernel(vcpu))
		kvm_apic_local_deliver(vcpu->arch.apic, APIC_LVTPC);
}
