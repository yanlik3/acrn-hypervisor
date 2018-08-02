/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VCPU_H_
#define	_VCPU_H_

#define	ACRN_VCPU_MMIO_COMPLETE		(0U)

/* Size of various elements within the VCPU structure */
#define REG_SIZE                            8

/* Number of GPRs saved / restored for guest in VCPU structure */
#define NUM_GPRS                            16U
#define GUEST_STATE_AREA_SIZE               512

#define	CPU_CONTEXT_OFFSET_RAX			0U
#define	CPU_CONTEXT_OFFSET_RCX			8U
#define	CPU_CONTEXT_OFFSET_RDX			16U
#define	CPU_CONTEXT_OFFSET_RBX			24U
#define	CPU_CONTEXT_OFFSET_RSP			32U
#define	CPU_CONTEXT_OFFSET_RBP			40U
#define	CPU_CONTEXT_OFFSET_RSI			48U
#define	CPU_CONTEXT_OFFSET_RDI			56U
#define	CPU_CONTEXT_OFFSET_R8			64U
#define	CPU_CONTEXT_OFFSET_R9			72U
#define	CPU_CONTEXT_OFFSET_R10			80U
#define	CPU_CONTEXT_OFFSET_R11			88U
#define	CPU_CONTEXT_OFFSET_R12			96U
#define	CPU_CONTEXT_OFFSET_R13			104U
#define	CPU_CONTEXT_OFFSET_R14			112U
#define	CPU_CONTEXT_OFFSET_R15			120U
#define	CPU_CONTEXT_OFFSET_CR0			128U
#define	CPU_CONTEXT_OFFSET_CR2			136U
#define	CPU_CONTEXT_OFFSET_CR3			144U
#define	CPU_CONTEXT_OFFSET_CR4			152U
#define	CPU_CONTEXT_OFFSET_RIP			160U
#define	CPU_CONTEXT_OFFSET_RFLAGS		168U
#define	CPU_CONTEXT_OFFSET_TSC_OFFSET		184U
#define	CPU_CONTEXT_OFFSET_IA32_SPEC_CTRL	192U
#define	CPU_CONTEXT_OFFSET_IA32_STAR		200U
#define	CPU_CONTEXT_OFFSET_IA32_LSTAR		208U
#define	CPU_CONTEXT_OFFSET_IA32_FMASK		216U
#define	CPU_CONTEXT_OFFSET_IA32_KERNEL_GS_BASE	224U
#define	CPU_CONTEXT_OFFSET_CS			280U
#define	CPU_CONTEXT_OFFSET_SS			312U
#define	CPU_CONTEXT_OFFSET_DS			344U
#define	CPU_CONTEXT_OFFSET_ES			376U
#define	CPU_CONTEXT_OFFSET_FS			408U
#define	CPU_CONTEXT_OFFSET_GS			440U
#define	CPU_CONTEXT_OFFSET_TR			472U
#define	CPU_CONTEXT_OFFSET_IDTR			504U
#define	CPU_CONTEXT_OFFSET_LDTR			536U
#define	CPU_CONTEXT_OFFSET_GDTR			568U
#define	CPU_CONTEXT_OFFSET_FXSTORE_GUEST_AREA	608U

/*sizes of various registers within the VCPU data structure */
#define VMX_CPU_S_FXSAVE_GUEST_AREA_SIZE    GUEST_STATE_AREA_SIZE

#ifndef ASSEMBLER

#include <guest.h>

enum vcpu_state {
	VCPU_INIT,
	VCPU_RUNNING,
	VCPU_PAUSED,
	VCPU_ZOMBIE,
	VCPU_UNKNOWN_STATE,
};

enum vm_cpu_mode {
	CPU_MODE_REAL,
	CPU_MODE_PROTECTED,
	CPU_MODE_COMPATIBILITY,		/* IA-32E mode (CS.L = 0) */
	CPU_MODE_64BIT,			/* IA-32E mode (CS.L = 1) */
};

/* General-purpose register layout aligned with the general-purpose register idx
 * when vmexit, such as vmexit due to CR access, refer to SMD Vol.3C 27-6.
 */
struct cpu_gp_regs {
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbx;
	uint64_t rsp;
	uint64_t rbp;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
};

struct segment_sel {
	uint16_t selector;
	uint64_t base;
	uint32_t limit;
	uint32_t attr;
};

struct run_context {
/* Contains the guest register set.
 * NOTE: This must be the first element in the structure, so that the offsets
 * in vmx_asm.S match
 */
	union {
		struct cpu_gp_regs regs;
		uint64_t longs[NUM_GPRS];
	} guest_cpu_regs;

	/** The guests CR registers 0, 2, 3 and 4. */
	uint64_t cr0;

	/* VMX_MACHINE_T_GUEST_CR2_OFFSET =
	*  offsetof(struct run_context, cr2) = 128
	*/
	uint64_t cr2;
	uint64_t cr3;
	uint64_t cr4;

	uint64_t rip;
	uint64_t rflags;

	uint64_t dr7;
	uint64_t tsc_offset;

	/* MSRs */
	/* VMX_MACHINE_T_GUEST_SPEC_CTRL_OFFSET =
	*  offsetof(struct run_context, ia32_spec_ctrl) = 192
	*/
	uint64_t ia32_spec_ctrl;
	uint64_t ia32_star;
	uint64_t ia32_lstar;
	uint64_t ia32_fmask;
	uint64_t ia32_kernel_gs_base;

	uint64_t ia32_pat;
	uint64_t vmx_ia32_pat;
	uint64_t ia32_efer;
	uint32_t ia32_sysenter_cs;
	uint64_t ia32_sysenter_esp;
	uint64_t ia32_sysenter_eip;
	uint64_t ia32_debugctl;

	uint64_t vmx_cr0;
	uint64_t vmx_cr4;

	/* segment registers */
	struct segment_sel cs;
	struct segment_sel ss;
	struct segment_sel ds;
	struct segment_sel es;
	struct segment_sel fs;
	struct segment_sel gs;
	struct segment_sel tr;
	struct segment_sel idtr;
	struct segment_sel ldtr;
	struct segment_sel gdtr;

	/* The 512 bytes area to save the FPU/MMX/SSE states for the guest */
	uint64_t
	fxstore_guest_area[VMX_CPU_S_FXSAVE_GUEST_AREA_SIZE / sizeof(uint64_t)]
	__aligned(16);
};

/* 2 worlds: 0 for Normal World, 1 for Secure World */
#define NR_WORLD	2
#define NORMAL_WORLD	0
#define SECURE_WORLD	1

struct event_injection_info {
	uint32_t intr_info;
	uint32_t error_code;
};

struct vcpu_arch {
	int cur_context;
	struct run_context contexts[NR_WORLD];

	/* A pointer to the VMCS for this CPU. */
	void *vmcs;
	uint16_t vpid;

	/* Holds the information needed for IRQ/exception handling. */
	struct {
		/* The number of the exception to raise. */
		uint32_t exception;

		/* The error number for the exception. */
		uint32_t error;
	} exception_info;

	uint8_t lapic_mask;
	uint32_t irq_window_enabled;
	uint32_t nrexits;

	/* Auxiliary TSC value */
	uint64_t msr_tsc_aux;

	/* VCPU context state information */
	uint32_t exit_reason;
	uint32_t idt_vectoring_info;
	uint64_t exit_qualification;
	uint32_t inst_len;

	/* Information related to secondary / AP VCPU start-up */
	enum vm_cpu_mode cpu_mode;
	uint8_t nr_sipi;
	uint32_t sipi_vector;

	/* interrupt injection information */
	uint64_t pending_req;
	bool inject_event_pending;
	struct event_injection_info inject_info;

	/* per vcpu lapic */
	void *vlapic;
};

struct vm;
struct vcpu {
	uint16_t pcpu_id;	/* Physical CPU ID of this VCPU */
	uint16_t vcpu_id;	/* virtual identifier for VCPU */
	struct vcpu_arch arch_vcpu;
		/* Architecture specific definitions for this VCPU */
	struct vm *vm;		/* Reference to the VM this VCPU belongs to */
	void *entry_addr;  /* Entry address for this VCPU when first started */

	/* State of this VCPU before suspend */
	volatile enum vcpu_state prev_state;
	volatile enum vcpu_state state;	/* State of this VCPU */
	/* State of debug request for this VCPU */
	volatile enum vcpu_state dbg_req_state;
	uint64_t sync;	/*hold the bit events*/
	struct acrn_vlapic *vlapic;	/* per vCPU virtualized LAPIC */

	struct list_head run_list; /* inserted to schedule runqueue */
	uint64_t pending_pre_work; /* any pre work pending? */
	bool launched; /* Whether the vcpu is launched on target pcpu */
	uint32_t paused_cnt; /* how many times vcpu is paused */
	uint32_t running; /* vcpu is picked up and run? */

	struct io_request req; /* used by io/ept emulation */

	/* save guest msr tsc aux register.
	 * Before VMENTRY, save guest MSR_TSC_AUX to this fields.
	 * After VMEXIT, restore this fields to guest MSR_TSC_AUX.
	 * This is only temperary workaround. Once MSR emulation
	 * is enabled, we should remove this fields and related
	 * code.
	 */
	uint64_t msr_tsc_aux_guest;
	uint64_t *guest_msrs;
#ifdef CONFIG_MTRR_ENABLED
	struct mtrr_state mtrr;
#endif
};

#define	is_vcpu_bsp(vcpu)	((vcpu)->vcpu_id == BOOT_CPU_ID)
/* do not update Guest RIP for next VM Enter */
static inline void vcpu_retain_rip(struct vcpu *vcpu)
{
	(vcpu)->arch_vcpu.inst_len = 0U;
}

/* External Interfaces */
struct vcpu* get_ever_run_vcpu(uint16_t pcpu_id);
int create_vcpu(uint16_t pcpu_id, struct vm *vm, struct vcpu **rtn_vcpu_handle);
int start_vcpu(struct vcpu *vcpu);
int shutdown_vcpu(struct vcpu *vcpu);
void destroy_vcpu(struct vcpu *vcpu);

void reset_vcpu(struct vcpu *vcpu);
void pause_vcpu(struct vcpu *vcpu, enum vcpu_state new_state);
void resume_vcpu(struct vcpu *vcpu);
void schedule_vcpu(struct vcpu *vcpu);
int prepare_vcpu(struct vm *vm, uint16_t pcpu_id);

void request_vcpu_pre_work(struct vcpu *vcpu, uint16_t pre_work_id);

#endif

#endif
