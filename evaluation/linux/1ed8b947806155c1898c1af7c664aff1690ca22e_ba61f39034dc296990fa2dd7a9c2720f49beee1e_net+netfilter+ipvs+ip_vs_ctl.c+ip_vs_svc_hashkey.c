static inline unsigned int
ip_vs_svc_hashkey(struct netns_ipvs *ipvs, int af, unsigned int proto,
		  const union nf_inet_addr *addr, __be16 port)
{
	register unsigned int porth = ntohs(port);
	__be32 addr_fold = addr->ip;
	__u32 ahash;

#ifdef CONFIG_IP_VS_IPV6
	if (af == AF_INET6)
		addr_fold = addr->ip6[0]^addr->ip6[1]^
			    addr->ip6[2]^addr->ip6[3];
#endif
	ahash = ntohl(addr_fold);
	ahash ^= ((size_t) ipvs >> 8);

	return (proto ^ ahash ^ (porth >> IP_VS_SVC_TAB_BITS) ^ porth) &
	       IP_VS_SVC_TAB_MASK;
}
