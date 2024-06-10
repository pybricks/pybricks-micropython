// tary, 0:20 2011/8/1
#ifndef __ARM920T_H__
#define __ARM920T_H__

// ARM920T (Rev 1) Technical Reference Manual

#define XPSR_MODE_MASK			0x1F
#define XPSR_Mode_User			0x10	//usr
#define XPSR_Mode_FIQ			0x11	//fiq
#define XPSR_Mode_IRQ			0x12	//irq
#define XPSR_Mode_Supervisor		0x13	//svc
#define XPSR_Mode_Abort			0x17	//abt
#define XPSR_Mode_Undefined		0x1B	//und
#define XPSR_Mode_System		0x1F	//sys
#define XPSR_F_Thumb			(0x1 << 5)
#define XPSR_F_FIQ			(0x1 << 6)
#define XPSR_F_IRQ			(0x1 << 7)
//#define XPSR_F_Q			(0x1 << 27)
#define XPSR_F_oVerflow			(0x1 << 28)
#define XPSR_F_Carry			(0x1 << 29)
#define XPSR_F_Zero			(0x1 << 30)
#define XPSR_F_Negative			(0x1 << 31)

#ifndef __ASSEMBLY__
void arm_intr_disable(void);
void arm_intr_enable(void);
#else
.EXTERN arm_intr_disable
.EXTERN arm_intr_enable
.EXTERN arm_mmu_disable
.EXTERN arm_mmu_enable
#endif

// 2.3 CP15 register map summary

#define CP15_C0_IDCode			c0
#define CP15_C0_CacheType		c0
#define CP15_C1_Control			c1
#define CP15_C2_TransTableBase		c2
#define CP15_C3_Domain			c3
//#define CP15_C4_Unpredictable		c4
#define CP15_C5_FaultStatus		c5
#define CP15_C6_FaultAddress		c6
#define CP15_C7_CacheOps		c7
#define CP15_C8_TLBOps			c8
#define CP15_C9_CacheLockdown		c9
#define CP15_C10_TLBLockdown		c10
//#define CP15_C11_Unpredictable	c11
//#define CP15_C12_Unpredictable	c12
#define CP15_C13_FCSEPID		c13
//#define CP15_C14_Unpredictable	c14
#define CP15_C15_TestConfig		c15

#ifndef __ASSEMBLY__
// c0, ID code register, read-only
typedef struct {
	unsigned LayoutRev:		4;
	unsigned PartNumber:		12;
	unsigned Architecture:		4;
	unsigned SpecRev:		4;
	unsigned Implementer:		8;
} cp15_id_code_t;

static inline cp15_id_code_t arm_read_cp15_id_code(void) {
	cp15_id_code_t c0;

	asm volatile(
	"mrc	p15, 0, %0, c0, c0, 0\n"
	: "=r" (c0)
	);
	return c0;
}
int arm_mmu_show_id_code(void);
#endif

#define CP15_C0_LayoutRev_MASK		(0xF << 0)
#define CP15_C0_PartNumber_MASK		(0xFFF << 4)
#define CP15_C0_Architecture_MASK	(0xF << 16)
#define CP15_C0_SpecRev_MASK		(0xF << 20)
#define CP15_C0_Implementer_MASK	(0xFF << 24)

#ifndef __ASSEMBLY__
// c0, cache type register, read-only
typedef struct {
	unsigned Ilen:			2;
	unsigned IM:			1;
	unsigned Iassoc:		3;
	unsigned Isize:			3;
	unsigned Ires:			3;
	unsigned Dlen:			2;
	unsigned DM:			1;
	unsigned Dassoc:		3;
	unsigned Dsize:			3;
	unsigned Dres:			3;
	unsigned S:			1;
	unsigned ctype:			4;
} cp15_cache_type_t;

static inline cp15_cache_type_t arm_read_cp15_cache_type(void) {
	cp15_cache_type_t c0;

	asm volatile(
	"mrc	p15, 0, %0, c0, c0, 1\n"
	: "=r" (c0)
	);
	return c0;
}
int arm_mmu_show_cache_type(void);
#endif

#define CP15_C0_Isize_MASK		(0xFFF << 0)
#define CP15_C0_Dsize_MASK		(0xFFF << 12)
#define CP15_C0_S_MASK			(0x1 << 24)
#define CP15_C0_ctype_MASK		(0xF << 25)

#ifndef __ASSEMBLY__
// c1, control register, read-write
typedef struct {
	unsigned M:			1; // 0
	unsigned A:			1; // 1
	unsigned C:			1; // 2
	unsigned SBO0:			4; // 3
	unsigned B:			1; // 7
	unsigned S:			1; // 8
	unsigned R:			1; // 9
	unsigned SBZ0:			2; // 10
	unsigned I:			1; // 12
	unsigned V:			1; // 13
	unsigned RR:			1; // 14
	unsigned L4:			1; // 15
	unsigned SBO1:			1; // 16
	unsigned SBZ1:			1; // 17
	unsigned SBO2:			1; // 18
	unsigned RES0:			13;// 19
} cp15_control_t;

static inline cp15_control_t arm_read_cp15_control(void) {
	cp15_control_t c1;

	asm volatile(
	"mrc	p15, 0, %0, c1, c0, 0\n"
	: "=r" (c1)
	);
	return c1;
}
static inline cp15_control_t arm_write_cp15_control(cp15_control_t c1) {
	asm volatile(
	"mcr	p15, 0, %0, c1, c0, 0\n"
	: "=r" (c1)
	);
	return c1;
}
int arm_mmu_show_control(const cp15_control_t* c1);
#endif

#define CP15_C1_ClockMode_MASK		(0x3 << 30)
#define CP15_C1_ClockMode_FastBus	(0x0 << 30)
#define CP15_C1_ClockMode_Sync		(0x1 << 30)
#define CP15_C1_ClockMode_Async		(0x3 << 30)
#define CP15_C1_M_MMU_Enable		(0x1 << 0)
#define CP15_C1_A_AlignFaultEnable	(0x1 << 1)
#define CP15_C1_C_DCacheEnable		(0x1 << 2)
#define CP15_C1_B_BigEndian		(0x1 << 7)
#define CP15_C1_S_SystemProtect		(0x1 << 8)
#define CP15_C1_R_ROM_Protect		(0x1 << 9)
#define CP15_C1_I_ICacheEnable		(0x1 << 12)
#define CP15_C1_V_ExceptionHighAddr	(0x1 << 13)
#define CP15_C1_RR_RoundRobin		(0x1 << 14)
#define CP15_C1_nF_notFastBus		(0x1 << 30)
#define CP15_C1_iA_AsyncClock		(0x1 << 31)

// c2, translation table base, read-write
#define CP15_C2_TTB_MASK		0xFFFFC000

// c3, domain access control register, read-write
#define CP15_C3_Domain_MASK(x)		(0x3 << (x << 1))
#define CP15_C3_Domain_X(acc, x)	((acc) << (x << 1))

// c5, fault status registers, read-modify-write
#define CP15_C5_FaultType_MASK		(0xF << 0)
#define CP15_C5_DomainAccess_MASK	(0xF << 4)
#define CP15_C5_DataFSR			c5, c0, 0
#define CP15_C5_PrefetchFSR		c5, c0, 1

// c6, fault address register, read-write
#define CP15_C6_MASK			((unsigned long)-1)

#ifndef __ASSEMBLY__
unsigned arm_read_cp15_fault_address(void);
#else
.EXTERN arm_read_cp15_fault_address
#endif

// c7, cache operation register, write-only
#define CP15_C7_InvalidICacheDCache	c7, c7, 0
#define CP15_C7_InvalidICache		c7, c5, 0
#define CP15_C7_InvalidICacheEntry	c7, c5, 1
#define CP15_C7_PretechICacheLine	c7, c13, 1
#define CP15_C7_InvalidDCache		c7, c6, 0
#define CP15_C7_InvalidDCacheEntry	c7, c6, 1
#define CP15_C7_CleanDCacheEntry	c7, c10, 1
#define CP15_C7_CleanInvalidDCacheEntry	c7, c14, 1
#define CP15_C7_CleanDCacheIndex	c7, c10, 2
#define CP15_C7_CleanInvalidDCacheIndex	c7, c14, 2
#define CP15_C7_DrainWriteBuffer	c7, c10, 4
#define CP15_C7_WaitForInterrupt	c7, c0, 4

#ifndef __ASSEMBLY__
static inline int arm_wfi(void) {
	asm volatile(
	"mov	r0, #0\n"
	"mcr	p15, 0, r0, c7, c0, 4\n"	/* wait for interrupt */
	::: "r0"
	);
	return 0;
}
#endif

#define CP15_C7_MVA_MASK		0xFFFFFFE0
#define CP15_C7_IndexIdx_MASK		(0x3F << 26)
#define CP15_C7_IndexIdx_X(x)		((x) << 26)
#define CP15_C7_IndexSeg_MASK		(0x7 << 5)
#define CP15_C7_IndexSeg_X(x)		((x) << 5)

// c8, TLB operations register, write-only
#define CP15_C8_InvalidTLB		c8, c7, 0
#define CP15_C8_InvalidITLB		c8, c5, 0
#define CP15_C8_InvalidITLBEntry	c8, c5, 1
#define CP15_C8_InvalidDTLB		c8, c6, 0
#define CP15_C8_InvalidDTLBEntry	c8, c6, 1

#ifndef __ASSEMBLY__
static inline int arm_tlb_invalid_all(void) {
	asm volatile(
	"mov	r0, #0\n"
	"mcr	p15, 0, r0, c7, c10, 4\n"	/* drain WB */
	"mcr	p15, 0, r0, c8, c7, 0\n"	/* invalidate I & D TLBs */
	::: "r0"
	);
	return 0;
}
#endif


#define CP15_C8_MVA_MASK		0xFFFFFC00

// c9, cache lockdown register, read-write
#define CP15_C9_DCacheLockdown		c9, c0, 0
#define CP15_C9_ICacheLockdown		c9, c0, 1

#define CP15_C9_Index_MASK		(0x3F << 26)

// c10, TLB lockdown register, read-write
#define CP15_C10_DTLBLockdown		c10, c0, 0
#define CP15_C10_ITLBLockdown		c10, c0, 1

#define CP15_C10_Base_MASK		(0x3F << 26)
#define CP15_C10_Victim_MASK		(0x3F << 20)
#define CP15_C10_P_PreserveMASK		(0x1 << 0)

// c13, FCSE PID register, read-write
#define CP15_C13_FCSE_PID_MASK		(0x7F << 25)

// c15, test configuration register


// 3.3.3 Level one descriptor
#define DESC_L1_TYPE_MASK		(0x3 << 0)
#define DESC_L1_TYPE_Invalid		0x0
#define DESC_L1_TYPE_CoarsePage		0x1
#define DESC_L1_TYPE_Section		0x2
#define DESC_L1_TYPE_FinePage		0x3

#define DESC_CB_MASK			(0x3 << 2)
#define DESC_NCNB			(0x0 << 2)	//Noncached, nonbuffered
#define DESC_NCB			(0x1 << 2)	//Noncached, buffered
#define DESC_WT				(0x2 << 2)	//Cached write-through mode
#define DESC_WB				(0x3 << 2)	//Cached write-back mode

#define DESC_L1_CONST_Bit4		(0x1 << 4)
#define DESC_L1_Domain_MASK		(0xF << 5)
#define DESC_L1_Domain_X(x)		((x) << 5)
#define DESC_L1_Sec_AP_MASK		(0x3 << 10)
#define DESC_L1_Sec_AP_X(x)		((x) << 10)

// coarse page tables have 256 entries
// occupy space 256 * 4 = 1KB
// 1 entry for (1MB / 256) = 4KB
#define DESC_L1_CoarseBase_MASK		0xFFFFFC00
// 1 section entry for 1MB
#define DESC_L1_SectionBase_MASK	0xFFF00000
// fine page tables have 1024 entries
// occupy space 1024 * 4 = 4KB
// 1 entry for (1MB / 1024) = 1KB
#define DESC_L1_FineBase_MASK		0xFFFFF000

// 3.3.8 Level two descriptor
#define DESC_L2_TYPE_MASK		(0x3 << 0)
#define DESC_L2_TYPE_Invalid		0x0
#define DESC_L2_TYPE_Large		0x1
#define DESC_L2_TYPE_Small		0x2
#define DESC_L2_TYPE_Tiny		0x3

// x = 0..3
#define DESC_L2_APx_MASK(x)		(0x30 << ((x) << 1))
#define DESC_L2_Tiny_ap_MASK		0x30

// 64KB, 16 identical entries in Coarse table
//       64 identical entries in Fine table
#define DESC_L2_LargeBase_MASK		0xFFFF0000
//  4KB, 1 entry in Coarse table
//       4 identical entries in Fine table
#define DESC_L2_SmallBase_MASK		0xFFFFF000
// only provide by Fine page table entry
//  1KB, 1 entry in Fine table
#define DESC_L2_TinyBase_MASK		0xFFFFFC00

// 3.4 MMU faults and CPU aborts
// Alignment fault isn't affected by whether 
// or not the MMU is enabled.
// Translation, domain, permission faults
// are only generated when the MMU is enabled.

// 3.5 Fault address and fault status register
// You can find the address for a prefetch fault in R14.
// The FAR is not updated by faults caused by instruction prefetches.

// 3.6 Domain access control
#define DOMAIN_NoAccess			0x0
#define DOMAIN_Client			0x1
#define DOMAIN_Manager			0x3

// When AP == 0
// Super read-only or Super & User read-only
#define RS_NoAccess			0x0
#define RS_OnlySuperRead		0x1
#define RS_OnlyRead			0x2

// Super for read-write, set for user permission
#define AP_SelectRS			0x0
#define AP_NoUserAccess			0x1
#define AP_NoUserWrite			0x2
#define AP_NoCheck			0x3

// 3.7 Fault checking sequence
// 1 Alignment fault
//	Check address alignment = Misaligned
// 2 Section translation fault
//	Get Level one descriptor = Invalid
// 3 Page translation fault
//	If page access and get page table entry = Invalid
// 4 Section or Page domain fault
//	Check domain status = No access
// 5 Section or Page permission fault
//	If domain client and check access permissions = Violation

// 3.9 Interaction of the MMU and caches
// 3.9.1 Enabling the MMU
// 1 Program the TTB and domain access control registers
// 2 Program level 1 and level 2 page tables as required
// 3 Enable the MMU by setting bit 0 in the control register
// You can enable the ICache and DCache simultaneously with
// the MMU using a single MCR instruction
// 3.9.2 Disabling the MMU
// The data cache must be disabled prior to, 
// or at the same time as, disable the MMU.
// If the MMU is enabled, then disabled and subsequently
// re-enabled, the contents of the TLBs are preserved.
// If these are now invalid, you must invalidate the TLBs
// before re-enabling the MMU.

// 4.2 ICache
// 4.2.2 Enabling and disabling the ICache
// If the cache is subsequently re-enabled its contents
// are unchanged. If the contents are no longer coherent
// with main memory, you must invalidate the ICache before
// you re-enable it.
// 4.2.3 ICache operation
// Ctt bit in the relevant MMU translation table descriptor
// is only used for cache miss, whether there is a eight-word
// linefill or a noncacheable single nonsequential memory access
// appears.

// 4.3 DCache and write buffer
// 4.3.1 Enabling and disabling the DCache and write buffer
// If the cache is subsequently re-enabled its contents are
// unchanged. Depending on the software system design, you might
// have to clean the cache after it is disabled, and invalidate it
// before you re-enable it.
// 4.3.2 DCache and write buffer operation
// If the DCache is enabled, a DCache lookup is performed for each
// data access initiated by the ARM9TDMI CPU core, regardless of
// the value of Ctt bit in the relevant MMU translation table
// descriptor.
// If the operating system has to change the C and B bits of a page
// table discriptor, it must ensure that the caches do not contain
// any data controlled by that descriptor. In some circumstances,
// the operating system might have to clean and flush the caches to
// ensure this.

#endif//__ARM920T_H__
