#include <zephyr/ipc/ipc_service.h>
#include <zephyr/drivers/misc/ppi/nrfx_dppi.h>
#include <zephyr/drivers/misc/ppi/nrfx_dppi_nrf54h.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dppi_local, 3);

static struct ipc_ept ep;
static struct k_sem sem;
static struct k_mutex alloc_lock;
static volatile uint32_t rsp_handle;
static volatile uint32_t result;

static void ep_bound(void *priv)
{
	LOG_INF("bounded");
	k_sem_give(&sem);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	const struct dppi_msg *msg = data;

	if ((len != DPPI_MSG_ALLOC_RSP_LEN) || (msg->type != DPPI_MSG_ALLOC_RSP)) {
		LOG_ERR("Unexpected length %d (exp:%d), type:%d",
				len, DPPI_MSG_ALLOC_RSP_LEN, msg->type);
		result = DPPI_RESULT_FAIL;
	} else {
		rsp_handle = msg->alloc_rsp.handle;
		result = msg->alloc_rsp.result;
	}

	k_sem_give(&sem);
}

static struct ipc_ept_cfg ep_cfg = {
	.name = "dppi",
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};


int nrf_dppi_service_alloc(uint32_t producer, uint32_t consumer, nrf_dppi_route_handle_t *handle)
{
	int ret;
	struct dppi_msg msg;

	msg.type = DPPI_MSG_ALLOC;
	msg.alloc.prod_id = producer;
	msg.alloc.cons_id = consumer;

	ret = k_mutex_lock(&alloc_lock, K_MSEC(1000));
	if (ret < 0) {
		return ret;
	}

	ret = ipc_service_send(&ep, &msg, DPPI_MSG_ALLOC_LEN);
	if (ret < 0) {
		/* TODO Repeat until successful. */
		LOG_ERR("Failed to send request");
		goto release_lock;
	}

	ret = k_sem_take(&sem, K_MSEC(1000));
	if (ret == 0) {
		if (result == DPPI_RESULT_OK) {
			*handle = rsp_handle;
			uint32_t dppi_cnt = rsp_handle >> 15 & 0x7;
			uint32_t dppi_ch = rsp_handle >> 18 & 0x1f;
			LOG_INF("Alloc prod:%d cons:%d handle:%08x, dppis involved:%d, chan:%d",
					producer, consumer, rsp_handle, dppi_cnt, dppi_ch);
		} else {
			ret = -EINVAL;
		}
	} else {
		LOG_ERR("Failed to get response");
	}

release_lock:
	k_mutex_unlock(&alloc_lock);

	return ret;
}

void nrf_dppi_service_free(nrf_dppi_route_handle_t handle)
{
	struct dppi_msg msg;
	int ret;

	msg.type = DPPI_MSG_FREE;
	msg.free.handle = handle;
	LOG_INF("Sending free handle:%08x", handle);
	ret = ipc_service_send(&ep, &msg, DPPI_MSG_FREE_LEN);
	if (ret < 0) {
		/* TODO Repeat until successful. */
		LOG_ERR("Failed to free handle");
	}
}

#if defined(CONFIG_SOC_NRF54H20_CPURAD)
int nrf_dppi_domain_local_alloc(uint32_t producer, uint32_t consumer, nrf_dppi_route_handle_t *handle);
void nrf_dppi_domain_local_free(nrf_dppi_route_handle_t handle);
#endif

int nrf_dppi_domain_conn_alloc(uint32_t producer, uint32_t consumer, nrf_dppi_route_handle_t *handle)
{
#if defined(CONFIG_SOC_NRF54H20_CPURAD)
#define NRF_DPPI_IS_RAD_DOMAIN(x) ((((uintptr_t)x >> 24) & 0xF) == 3)
	if (NRF_DPPI_IS_RAD_DOMAIN(producer) && NRF_DPPI_IS_RAD_DOMAIN(consumer)) {
		return nrf_dppi_domain_local_alloc(producer, consumer, handle);
	}
	if (!NRF_DPPI_IS_RAD_DOMAIN(producer) && !NRF_DPPI_IS_RAD_DOMAIN(consumer)) {
		return nrf_dppi_service_alloc(producer, consumer, handle);
	}

	return -EINVAL;
#else
	return nrf_dppi_service_alloc(producer, consumer, handle);
#endif
}

void nrf_dppi_domain_conn_free(nrf_dppi_route_handle_t handle)
{
#if defined(CONFIG_SOC_NRF54H20_CPURAD)
	if (handle & BIT(24)) {
		return nrf_dppi_domain_local_free(handle);
	} else {
		nrf_dppi_service_free(handle);
	}
#else
	nrf_dppi_service_free(handle);
#endif
}

static int dppi_service_init(void)
{
	const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(cpusec_cpuapp_ipc_b));
	int ret;

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	k_mutex_init(&alloc_lock);
	(void)k_sem_init(&sem, 0, 1);

	ret = ipc_service_register_endpoint(ipc_instance, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

	ret = k_sem_take(&sem, K_MSEC(10000));
	if (ret < 0) {
		LOG_ERR("Failed to bound");
	}

	return ret;
}

SYS_INIT(dppi_service_init, POST_KERNEL, UTIL_INC(CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY));
