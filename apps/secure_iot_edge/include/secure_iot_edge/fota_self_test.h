/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_FOTA_SELF_TEST_H_
#define SECURE_IOT_EDGE_FOTA_SELF_TEST_H_

struct app_context;

/** Run post-OTA checks before MCUboot image confirm. Returns 0 if all pass. */
int fota_self_test_run(struct app_context *ctx);
void fota_self_test_log_summary(void);

#endif /* SECURE_IOT_EDGE_FOTA_SELF_TEST_H_ */