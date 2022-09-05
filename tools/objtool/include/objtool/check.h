/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#ifndef _CHECK_H
#define _CHECK_H

#include <stdbool.h>
#include <objtool/cfi.h>
#include <objtool/arch.h>

struct insn_state {
	struct cfi_state cfi;
	unsigned int uaccess_stack;
	bool uaccess;
	bool df;
	bool noinstr;
	s8 instr;
};

struct alt_group {
	/*
	 * Pointer from a replacement group to the original group.  NULL if it
	 * *is* the original group.
	 */
	struct alt_group *orig_group;

	/* First and last instructions in the group */
	struct instruction *first_insn, *last_insn;

	/*
	 * Byte-offset-addressed len-sized array of pointers to CFI structs.
	 * This is shared with the other alt_groups in the same alternative.
	 */
	struct cfi_state **cfi;
};

struct instruction {
	struct list_head list;
	struct hlist_node hash;
	struct list_head call_node;
	struct list_head orbit_node;
	struct section *sec;
	unsigned long offset;
	unsigned int len;
	enum insn_type type;
	unsigned long immediate;

	u16 dead_end		: 1,
	   ignore		: 1,
	   ignore_alts		: 1,
	   hint			: 1,
	   save			: 1,
	   restore		: 1,
	   retpoline_safe	: 1,
	   noendbr		: 1,
	   entry		: 1,
	   not_sibling_call	: 1;
		/* 6 bit hole */

	s8 instr;
	u8 visited;

	struct alt_group *alt_group;
	struct symbol *call_dest;
	struct instruction *jump_dest;
	struct instruction *first_jump_src;
	struct reloc *jump_table;
	struct reloc *reloc;
	struct list_head alts;
	struct symbol *func;
	struct list_head stack_ops;
	struct cfi_state *cfi;
};

#define VISITED_BRANCH		0x01
#define VISITED_BRANCH_UACCESS	0x02
#define VISITED_BRANCH_MASK	0x03
#define VISITED_ENTRY		0x04

static inline bool is_static_jump(struct instruction *insn)
{
	return insn->type == INSN_JUMP_CONDITIONAL ||
	       insn->type == INSN_JUMP_UNCONDITIONAL;
}

static inline bool is_dynamic_jump(struct instruction *insn)
{
	return insn->type == INSN_JUMP_DYNAMIC ||
	       insn->type == INSN_JUMP_DYNAMIC_CONDITIONAL;
}

static inline bool is_jump(struct instruction *insn)
{
	return is_static_jump(insn) || is_dynamic_jump(insn);
}

bool is_sibling_call(struct instruction *insn);
void save_reg(struct cfi_state *cfi, unsigned char reg, int base, int offset);
void restore_reg(struct cfi_state *cfi, unsigned char reg);

struct instruction *find_insn(struct objtool_file *file,
			      struct section *sec, unsigned long offset);
struct instruction *next_insn_same_func(struct objtool_file *file,
					struct instruction *insn);
struct instruction *prev_insn_same_sym(struct objtool_file *file,
				       struct instruction *insn);
int add_jump_table(struct objtool_file *file, struct instruction *insn,
				struct reloc *table);
bool arch_has_valid_stack_frame(struct insn_state *state);
int arch_classify_symbols(struct objtool_file *file);
int arch_create_static_call_sections(struct objtool_file *file);
int arch_handle_insn_ops(struct instruction *insn, struct instruction *next_insn, struct insn_state *state);

#define for_each_insn(file, insn)					\
	list_for_each_entry(insn, &file->insn_list, list)

#define sec_for_each_insn(file, sec, insn)				\
	for (insn = find_insn(file, sec, 0);				\
	     insn && &insn->list != &file->insn_list &&			\
			insn->sec == sec;				\
	     insn = list_next_entry(insn, list))


#define func_last_orbit(p)						\
	(list_first_entry_or_null(p, struct instruction, orbit_node))

#define func_for_each_insn(file, func, insn)				\
	for (insn = find_insn(file, func->sec, func->offset);		\
	     insn;							\
	     insn = next_insn_same_func(file, insn))

#define sym_for_each_insn(file, sym, insn)				\
	for (insn = find_insn(file, sym->sec, sym->offset);		\
	     insn && &insn->list != &file->insn_list &&			\
		insn->sec == sym->sec &&				\
		insn->offset < sym->offset + sym->len;			\
	     insn = list_next_entry(insn, list))

#define sym_for_each_insn_continue_reverse(file, sym, insn)		\
	for (insn = list_prev_entry(insn, list);			\
	     &insn->list != &file->insn_list &&				\
		insn->sec == sym->sec && insn->offset >= sym->offset;	\
	     insn = list_prev_entry(insn, list))

#define sec_for_each_insn_from(file, insn)				\
	for (; insn; insn = next_insn_same_sec(file, insn))

#define sec_for_each_insn_continue(file, insn)				\
	for (insn = next_insn_same_sec(file, insn); insn;		\
	     insn = next_insn_same_sec(file, insn))

#endif /* _CHECK_H */
