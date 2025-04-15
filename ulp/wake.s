/* ULP Example: using ADC in deep sleep

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This file contains assembly code which runs on the ULP.

   ULP wakes up to run this code at a certain period, determined by the values
   in SENS_ULP_CP_SLEEP_CYCx_REG registers. On each wake up, the program
   measures input voltage on the given ADC channel 'adc_oversampling_factor'
   times. Measurements are accumulated and average value is calculated.
   Average value is compared to the two thresholds: 'low_threshold' and 'high_threshold'.
   If the value is less than 'low_threshold' or more than 'high_threshold', ULP wakes up
   the chip from deep sleep.
*/

/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files
*/
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

// use version number to know what we are running
.set version, 2

/* ADC1 channel 6, GPIO34 */
.set adc_channel, 6

/* Define variables, which go into .bss section (zero-initialized data) */
.bss

.global start_counter
start_counter:
.long 0

.global cur_version
cur_version:
.long 0

.global wakeup_counter
wakeup_counter:
.long 0

.global wakeup_counter_goal
wakeup_counter_goal:
.long 0

.global do_adc
do_adc:
.long 0

/* Code goes into .text section */
.text
.global entry
entry:

/* increment start counter */
move r3, start_counter
ld R2, r3, 0
add R2, R2, 1
st R2, r3, 0

// provide version to caller
move r3, cur_version // load address
move r2, version // store const
st R2, r3, 0 // write const into address

// implement manual timer
// load wakeup_counter and wakeup_counter_goal
// if at goal then wake, otherwise increment
move r3, wakeup_counter
ld R2, r3, 0
move r1, wakeup_counter_goal
ld R0, r1, 0
sub r1, r2, r0
jump wake_up, eq // jump if r2==r0
add r2, r2, 1 // inc
st r2, r3, 0 // store to memory

// perform adc instruction if option is set
move r3, do_adc
ld R0, r3, 0
jumpr exit, 0, eq // exit if do_adc == 0
adc r1, 0, adc_channel + 1

.global exit
exit:
halt

.global wake_up
wake_up:
/* Check if the system can be woken up */
READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
and r0, r0, 1
jump exit, eq

/* Wake up the SoC, end program */
wake
WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
jump exit
