// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 Loongson Technology Corporation Limited
 */
#include <linux/kernel.h>
#include <linux/acpi.h>
#include <linux/cpu.h>
#include <linux/efi.h>
#include <linux/export.h>
#include <linux/pm.h>
#include <linux/types.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/console.h>
#include <linux/kexec.h>
#include <linux/libfdt.h>
#include <linux/of_fdt.h>

#include <acpi/reboot.h>
#include <asm/idle.h>
#include <asm/loongarch.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_halt(void)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_send_stop();
#endif
	local_irq_disable();
	clear_csr_ecfg(ECFG0_IM);

	pr_notice("\n\n** You can safely turn off the power now **\n\n");
	console_flush_on_panic(CONSOLE_FLUSH_PENDING);

	while (true) {
		__arch_cpu_idle();
	}
}

void machine_power_off(void)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_send_stop();
#endif
	do_kernel_power_off();
#ifdef CONFIG_EFI
	efi.reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
#endif

	while (true) {
		__arch_cpu_idle();
	}
}

void machine_restart(char *command)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_send_stop();
#endif
	do_kernel_restart(command);
#ifdef CONFIG_EFI
	if (efi_capsule_pending(NULL))
		efi_reboot(REBOOT_WARM, NULL);
	else
		efi_reboot(REBOOT_COLD, NULL);
#endif
	if (!acpi_disabled)
		acpi_reboot();

	while (true) {
		__arch_cpu_idle();
	}
}

#ifdef CONFIG_KEXEC

/* 0X80000000~0X80200000 is safe */
#define KEXEC_CTRL_CODE	TO_CACHE(0x100000UL)
#define KEXEC_BLOB_ADDR	TO_CACHE(0x108000UL)

static int loongson_kexec_prepare(struct kimage *image)
{
	int i;
	void *dtb = (void *)KEXEC_BLOB_ADDR;

	for (i = 0; i < image->nr_segments; i++) {
		if (!fdt_check_header(image->segment[i].buf)) {
			memcpy(dtb, image->segment[i].buf, SZ_64K);
			break;
		}
	}

	/* kexec/kdump need a safe page to save reboot_code_buffer */
	image->control_code_page = virt_to_page((void *)KEXEC_CTRL_CODE);

	return 0;
}

static void loongson_kexec_shutdown(void)
{
#ifdef CONFIG_SMP
	int cpu;

	/* All CPUs go to reboot_code_buffer */
	for_each_possible_cpu(cpu)
		if (!cpu_online(cpu))
			cpu_device_up(get_cpu_device(cpu));

	secondary_kexec_args[0] = TO_UNCACHE(0x1fe01000);
#endif
	kexec_args[0] = fw_arg0;
	kexec_args[1] = TO_PHYS(KEXEC_BLOB_ADDR);
}

static void loongson_crash_shutdown(struct pt_regs *regs)
{
	default_machine_crash_shutdown(regs);
#ifdef CONFIG_SMP
	secondary_kexec_args[0] = TO_UNCACHE(0x1fe01000);
#endif
	kexec_args[0] = fw_arg0;
	kexec_args[1] = TO_PHYS(KEXEC_BLOB_ADDR);
}

static int __init loongarch_kexec_setup(void)
{
	_machine_kexec_prepare = loongson_kexec_prepare;
	_machine_kexec_shutdown = loongson_kexec_shutdown;
	_machine_crash_shutdown = loongson_crash_shutdown;

	return 0;
}

arch_initcall(loongarch_kexec_setup);

#endif
