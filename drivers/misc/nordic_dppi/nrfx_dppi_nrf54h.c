#include <zephyr/drivers/misc/ppi/nrfx_dppi.h>
#include <zephyr/drivers/misc/ppi/nrfx_dppi_nrf54h.h>

uint32_t nrf_dppi_get_domain_id(uint32_t addr)
{
	uint32_t domain = (addr >> 24) & 0xf;
	uint32_t apb = (addr >> 16) & 0xff;

	if (domain == 0x3) {
		return (apb == 2) ? NRF_DPPI_DOMAIN_APB2 : NRF_DPPI_DOMAIN_APB3;
	}

	__ASSERT_NO_MSG(domain == 0xf);

	if (apb < 0x92) {
		return NRF_DPPI_DOMAIN_APB22;
	} else if (apb <= 0x93) {
		return NRF_DPPI_DOMAIN_APB32;
	} else {
		return apb - 0x98 + NRF_DPPI_DOMAIN_APB38;
	}
}
