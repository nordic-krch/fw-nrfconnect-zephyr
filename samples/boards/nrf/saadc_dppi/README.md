# SAADC maxmimum performance {#saadc_maximum_performance}

This sample is a port to nrf54h20dk from sample that can be found in
``modules/hal/nordic/nrfx/samples/src/nrfx_saadc/maximum_performance``.
The sample demonstrates an advanced functionality of the nrfx_saadc driver operating at its peak performance.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf54h20dk_nrf54h20 |     Yes     |

## Overview

Application initializes the nrfx_saadc driver and starts operating in the non-blocking mode.
Sampling is performed at the highest supported frequency.
In the sample @p m_single_channel is configured, and the SAADC driver is set to the advanced mode.
To achieve the maximum performance, do the following:
- Provide an external timer in order to perform sampling at @p MAX_SAADC_SAMPLE_FREQUENCY.
  You can do this by setting up endpoints of the channel @p m_gppi_channels [ @p gppi_channels_purpose_t::SAADC_SAMPLING ] to trigger the SAADC sample task ( @p nrf_saadc_task_t::NRF_SAADC_TASK_SAMPLE ) on the TIMER COMPARE event.
- Provide hardware start-on-end.
  You can do this by setting up endpoints of the channel @p m_gppi_channels [ @p gppi_channels_purpose_t::SAADC_START_ON_END ] to trigger SAADC task start ( @p nrf_saadc_task_t::NRF_SAADC_TASK_START ) on the SAADC event end ( @p nrf_saadc_event_t::NRF_SAADC_EVENT_END ).

@p nrfx_saadc_offset_calibrate triggers calibration in a non-blocking manner.
Then, sampling is initiated at @p NRFX_SAADC_EVT_CALIBRATEDONE event in @p saadc_handler() by calling @p nrfx_saadc_mode_trigger() function.
Consecutive sample tasks are triggered by the external timer at the sample rate specified in @p SAADC_SAMPLE_FREQUENCY symbol.

> For more information, see **SAADC driver - nrfx documentation**.

## Considerations

1. When nrfx drivers are used directly then peripherals must be assigned to cpuapp by
  SDFW. As it happens in devicetree we must reserve those nodes (status = "reserved").
  Nodes shall not be enabled (status = "okay") because that might enable and initialize
  zephyr driver. Additionally, interrupt must be manually connected (``IRQ_CONNECT``).

2. SAADC buffers must be located in special RAM memory (RAM3). Buffers are declared in
  a dedicated memory section and devicetree ``adc`` node has that information thus
  a macro ``DMM_MEMORY_SECTION`` is used to pick the that section.

3. DPPI connections which are used by the sample must be preallocated in the devicetree
  so that those channels are assigned to the cpuapp by SDFW during boot. That is because
  DPPI channels are shared resource. Overlay file has comments which explain why
  given channels are needed.

4. In the example 8 sample buffers are used and next buffer must be provided to the
  SAADC on time since DPPI drives sampling and buffer switching. When SAADC collects
  data to one buffer there is a buffer request event to provide next buffer. Due
  to high frequency of sample there is ~40us to provide next buffer which does not
  leave much room for maximum latency. To increase the accepted latency buffer
  size must be increased.

## Wiring

To run the sample correctly, connect pins as follows:
- nrf54h20dk Pin 1.0 (Analog pin 0) with Pin 9.0 (LED0 pin).

You should see the following output:

```
*** Booting nRF Connect SDK v2.7.99-858a047e118a ***
*** Using Zephyr OS v3.6.99-ac074c99ecb3 ***
[00:00:00.189,381] <inf> app: Starting nrfx_saadc maximum performance example.
[00:00:00.189,540] <inf> app: SAADC event: CALIBRATEDONE
[00:00:00.189,550] <inf> app: SAADC event: READY
[00:00:00.189,562] <inf> app: SAADC event: BUF_REQ diff:189558
[00:00:00.189,601] <inf> app: SAADC event: DONE
[00:00:00.189,610] <inf> app: Sample buffer address == 0x2fc12ea8
[00:00:00.189,622] <inf> app: SAADC event: BUF_REQ diff:21
[00:00:00.189,640] <inf> app: SAADC event: DONE
[00:00:00.189,646] <inf> app: Sample buffer address == 0x2fc12eb8
[00:00:00.189,657] <inf> app: SAADC event: BUF_REQ diff:17
[00:00:00.189,682] <inf> app: SAADC event: DONE
[00:00:00.189,688] <inf> app: Sample buffer address == 0x2fc12ea8
[00:00:00.189,697] <inf> app: FINISHED
[00:00:00.289,599] <inf> app: [Sample 0.0]: 0
[00:00:00.289,605] <inf> app: [Sample 0.1]: -1
[00:00:00.289,611] <inf> app: [Sample 0.2]: -7
[00:00:00.289,617] <inf> app: [Sample 0.3]: -1
[00:00:00.289,623] <inf> app: [Sample 0.4]: -4
[00:00:00.289,631] <inf> app: [Sample 0.5]: -1
[00:00:00.289,637] <inf> app: [Sample 0.6]: -1
[00:00:00.289,643] <inf> app: [Sample 0.7]: 0
[00:00:00.289,648] <inf> app: [Sample 1.0]: -1
[00:00:00.289,654] <inf> app: [Sample 1.1]: -1
[00:00:00.289,660] <inf> app: [Sample 1.2]: 0
[00:00:00.289,665] <inf> app: [Sample 1.3]: -1
[00:00:00.289,671] <inf> app: [Sample 1.4]: -1
[00:00:00.289,677] <inf> app: [Sample 1.5]: 1023
[00:00:00.289,683] <inf> app: [Sample 1.6]: 1023
[00:00:00.289,689] <inf> app: [Sample 1.7]: 1023
[00:00:00.289,695] <inf> app: [Sample 2.0]: 1023
[00:00:00.289,700] <inf> app: [Sample 2.1]: 1023
[00:00:00.289,706] <inf> app: [Sample 2.2]: 1023
[00:00:00.289,712] <inf> app: [Sample 2.3]: 1023
[00:00:00.289,718] <inf> app: [Sample 2.4]: 1023
[00:00:00.289,723] <inf> app: [Sample 2.5]: 1023
[00:00:00.289,729] <inf> app: [Sample 2.6]: 1023
[00:00:00.289,735] <inf> app: [Sample 2.7]: 1023

```
