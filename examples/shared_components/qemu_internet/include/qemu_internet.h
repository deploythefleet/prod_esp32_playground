#ifndef PRODESP32_QEMU_INTERNET_H
#define PRODESP32_QEMU_INTERNET_H

#include "esp_err.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void qemu_internet_disconnect(void);
esp_err_t qemu_internet_connect(void);

#ifdef __cplusplus
}
#endif

#endif  // PRODESP32_QEMU_INTERNET_H