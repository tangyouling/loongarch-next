/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Most of this ideas comes from x86.
 *
 * Copyright (C) 2022 Loongson Technology Corporation Limited
 */
#ifndef _ASM_UNWIND_H
#define _ASM_UNWIND_H

#include <linux/sched.h>
#include <linux/module.h>

#include <asm/stacktrace.h>

enum unwinder_type {
	UNWINDER_GUESS,
	UNWINDER_PROLOGUE,
	UNWINDER_ORC,
};

struct unwind_state {
	char type; /* UNWINDER_XXX */
	struct stack_info stack_info;
	struct task_struct *task;
	int graph_idx;
	bool first, error;
	unsigned long sp, pc, fp, ra;
};

void unwind_start(struct unwind_state *state,
		  struct task_struct *task, struct pt_regs *regs);
bool unwind_next_frame(struct unwind_state *state);
unsigned long unwind_get_return_address(struct unwind_state *state);

static inline bool unwind_done(struct unwind_state *state)
{
	return state->stack_info.type == STACK_TYPE_UNKNOWN;
}

static inline bool unwind_error(struct unwind_state *state)
{
	return state->error;
}

#ifdef CONFIG_UNWINDER_ORC
void unwind_init(void);
void unwind_module_init(struct module *mod, void *orc_ip, size_t orc_ip_size,
			void *orc, size_t orc_size);
#else
static inline void unwind_init(void) {}
static inline void unwind_module_init(struct module *mod, void *orc_ip,
			size_t orc_ip_size, void *orc, size_t orc_size) {}
#endif /* CONFIG_UNWINDER_ORC */
#endif /* _ASM_UNWIND_H */
