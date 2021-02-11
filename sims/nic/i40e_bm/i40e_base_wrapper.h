/*
 * Copyright 2021 Max Planck Institute for Software Systems, and
 * National University of Singapore
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>

#define PF_DRIVER
#define I40E_MASK(mask, shift) (mask << shift)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;

#include "base/i40e_devids.h"
#include "base/i40e_register.h"
#include "base/i40e_adminq_cmd.h"
#include "base/i40e_rxtxq.h"

/* from i40e_types.h */

/* Checksum and Shadow RAM pointers */
#define I40E_SR_NVM_CONTROL_WORD		0x00
#define I40E_SR_PCIE_ANALOG_CONFIG_PTR		0x03
#define I40E_SR_PHY_ANALOG_CONFIG_PTR		0x04
#define I40E_SR_OPTION_ROM_PTR			0x05
#define I40E_SR_RO_PCIR_REGS_AUTO_LOAD_PTR	0x06
#define I40E_SR_AUTO_GENERATED_POINTERS_PTR	0x07
#define I40E_SR_PCIR_REGS_AUTO_LOAD_PTR		0x08
#define I40E_SR_EMP_GLOBAL_MODULE_PTR		0x09
#define I40E_SR_RO_PCIE_LCB_PTR			0x0A
#define I40E_SR_EMP_IMAGE_PTR			0x0B
#define I40E_SR_PE_IMAGE_PTR			0x0C
#define I40E_SR_CSR_PROTECTED_LIST_PTR		0x0D
#define I40E_SR_MNG_CONFIG_PTR			0x0E
#define I40E_EMP_MODULE_PTR			0x0F
#define I40E_SR_EMP_MODULE_PTR			0x48
#define I40E_SR_PBA_FLAGS			0x15
#define I40E_SR_PBA_BLOCK_PTR			0x16
#define I40E_SR_BOOT_CONFIG_PTR			0x17
#define I40E_NVM_OEM_VER_OFF			0x83
#define I40E_SR_NVM_DEV_STARTER_VERSION		0x18
#define I40E_SR_NVM_WAKE_ON_LAN			0x19
#define I40E_SR_ALTERNATE_SAN_MAC_ADDRESS_PTR	0x27
#define I40E_SR_PERMANENT_SAN_MAC_ADDRESS_PTR	0x28
#define I40E_SR_NVM_MAP_VERSION			0x29
#define I40E_SR_NVM_IMAGE_VERSION		0x2A
#define I40E_SR_NVM_STRUCTURE_VERSION		0x2B
#define I40E_SR_NVM_EETRACK_LO			0x2D
#define I40E_SR_NVM_EETRACK_HI			0x2E
#define I40E_SR_VPD_PTR				0x2F
#define I40E_SR_PXE_SETUP_PTR			0x30
#define I40E_SR_PXE_CONFIG_CUST_OPTIONS_PTR	0x31
#define I40E_SR_NVM_ORIGINAL_EETRACK_LO		0x34
#define I40E_SR_NVM_ORIGINAL_EETRACK_HI		0x35
#define I40E_SR_SW_ETHERNET_MAC_ADDRESS_PTR	0x37
#define I40E_SR_POR_REGS_AUTO_LOAD_PTR		0x38
#define I40E_SR_EMPR_REGS_AUTO_LOAD_PTR		0x3A
#define I40E_SR_GLOBR_REGS_AUTO_LOAD_PTR	0x3B
#define I40E_SR_CORER_REGS_AUTO_LOAD_PTR	0x3C
#define I40E_SR_PHY_ACTIVITY_LIST_PTR		0x3D
#define I40E_SR_PCIE_ALT_AUTO_LOAD_PTR		0x3E
#define I40E_SR_SW_CHECKSUM_WORD		0x3F
#define I40E_SR_1ST_FREE_PROVISION_AREA_PTR	0x40
#define I40E_SR_4TH_FREE_PROVISION_AREA_PTR	0x42
#define I40E_SR_3RD_FREE_PROVISION_AREA_PTR	0x44
#define I40E_SR_2ND_FREE_PROVISION_AREA_PTR	0x46
#define I40E_SR_EMP_SR_SETTINGS_PTR		0x48
#define I40E_SR_FEATURE_CONFIGURATION_PTR	0x49
#define I40E_SR_CONFIGURATION_METADATA_PTR	0x4D
#define I40E_SR_IMMEDIATE_VALUES_PTR		0x4E

/* Auxiliary field, mask and shift definition for Shadow RAM and NVM Flash */
#define I40E_SR_VPD_MODULE_MAX_SIZE		1024
#define I40E_SR_PCIE_ALT_MODULE_MAX_SIZE	1024
#define I40E_SR_CONTROL_WORD_1_SHIFT		0x06
#define I40E_SR_CONTROL_WORD_1_MASK	(0x03 << I40E_SR_CONTROL_WORD_1_SHIFT)
#define I40E_SR_CONTROL_WORD_1_NVM_BANK_VALID	BIT(5)
#define I40E_SR_NVM_MAP_STRUCTURE_TYPE		BIT(12)
#define I40E_PTR_TYPE				BIT(15)
#define I40E_SR_OCP_CFG_WORD0			0x2B
#define I40E_SR_OCP_ENABLED			BIT(15)

/* Shadow RAM related */
#define I40E_SR_SECTOR_SIZE_IN_WORDS	0x800
#define I40E_SR_BUF_ALIGNMENT		4096
#define I40E_SR_WORDS_IN_1KB		512
/* Checksum should be calculated such that after adding all the words,
 * including the checksum word itself, the sum should be 0xBABA.
 */
#define I40E_SR_SW_CHECKSUM_BASE	0xBABA

#define I40E_SRRD_SRCTL_ATTEMPTS	100000


