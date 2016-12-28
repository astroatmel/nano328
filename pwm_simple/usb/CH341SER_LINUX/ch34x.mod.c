#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xfe2565c8, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x6b065930, __VMLINUX_SYMBOL_STR(usb_serial_deregister_drivers) },
	{ 0xdaf680ca, __VMLINUX_SYMBOL_STR(usb_serial_register_drivers) },
	{ 0x7dfdc178, __VMLINUX_SYMBOL_STR(usb_serial_port_softint) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xa3d778c7, __VMLINUX_SYMBOL_STR(__dynamic_dev_dbg) },
	{ 0x1b2754db, __VMLINUX_SYMBOL_STR(tty_insert_flip_string_flags) },
	{ 0x7cc37910, __VMLINUX_SYMBOL_STR(tty_flip_buffer_push) },
	{ 0xa183c32b, __VMLINUX_SYMBOL_STR(tty_buffer_request_room) },
	{ 0xcf21d241, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xf432dd3d, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x7ff88cb6, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x67c6a176, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xf8bda83, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0xe85b184f, __VMLINUX_SYMBOL_STR(usb_submit_urb) },
	{ 0x99049c20, __VMLINUX_SYMBOL_STR(usb_clear_halt) },
	{ 0xc5aa55a9, __VMLINUX_SYMBOL_STR(usb_kill_urb) },
	{ 0x2cea0e37, __VMLINUX_SYMBOL_STR(tty_encode_baud_rate) },
	{ 0x409873e3, __VMLINUX_SYMBOL_STR(tty_termios_baud_rate) },
	{ 0xf2997713, __VMLINUX_SYMBOL_STR(tty_termios_hw_change) },
	{ 0x67b27ec1, __VMLINUX_SYMBOL_STR(tty_std_termios) },
	{ 0x6f62054e, __VMLINUX_SYMBOL_STR(usb_control_msg) },
	{ 0xfa66f77c, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x34f22f94, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0xbd1763fe, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x8f64aa4, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x9327f5ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=usbserial";

MODULE_ALIAS("usb:v1A86p7523d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v1A86p5523d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "E4C8A47A5DBE4E4A9A41F20");
