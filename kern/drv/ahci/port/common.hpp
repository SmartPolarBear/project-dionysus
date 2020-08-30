#pragma once
#include "system/types.h"
#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/ahci/ahci.hpp"
#include "drivers/ahci/ata/ata.hpp"
#include "drivers/ahci/ata/ata_string.hpp"
#include "drivers/ahci/atapi/atapi.hpp"

error_code common_identify_device(ahci::ahci_port* port, bool atapi);