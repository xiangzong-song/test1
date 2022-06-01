#ifndef HCI_TEST_H
#define HCI_TEST_H

#include "stdint.h"

void LightSdk_hci_test_adjust(void);
void LightSdk_hci_test_init(uint8_t* s_version, uint8_t* h_version);
int LightSdk_hci_test_check(void);

#endif
