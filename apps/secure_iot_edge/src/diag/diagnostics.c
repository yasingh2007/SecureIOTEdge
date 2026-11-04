/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(diag, CONFIG_LOG_DEFAULT_LEVEL);

static uint32_t heartbeat_count;
static int64_t boot_time_ms;

int diagnostics_init(void)
{
	boot_time_ms = k_uptime_get();
	LOG_INF("Secure IoT Edge diagnostics active (app %s)", CONFIG_SECURE_IOT_EDGE_APP_VERSION);
	return 0;
}

void diagnostics_periodic(void)
{
	heartbeat_count++;

	if ((heartbeat_count % 6U) == 0U) {
		LOG_DBG("uptime %lld s, heartbeats %u", (k_uptime_get() - boot_time_ms) / 1000,
			heartbeat_count);
	}
}

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_DIAG_SHELL)
#include <zephyr/shell/shell.h>

static int cmd_diag_reboot(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_info(sh, "Rebooting...");
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}

SHELL_CMD_REGISTER(diag_reboot, NULL, "Cold reboot", cmd_diag_reboot);
#endif
