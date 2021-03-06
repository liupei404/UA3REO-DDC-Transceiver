set_time_format -unit ns -decimal_places 3

create_clock -name "clock_crystal" -period 122.880MHz [get_ports {clk_sys}]
create_clock -name "clock_adc" -period 122.880MHz [get_ports {ADC_CLK}]

set_clock_groups -asynchronous -group { clock_adc clock_crystal }

derive_pll_clocks -create_base_clocks

derive_clock_uncertainty

set_output_delay -clock clock_crystal -max 36ps [get_ports {DAC_OUTPUT[*]}]
set_output_delay -clock clock_crystal -min 30ps [get_ports {DAC_OUTPUT[*]}]

set_input_delay -clock clock_adc -max 0ps [get_ports ADC_INPUT[*]]
set_input_delay -clock clock_adc -min -66ps [get_ports ADC_INPUT[*]]
set_input_delay -clock clock_adc -max 0ps [get_ports ADC_OTR]
set_input_delay -clock clock_adc -min -66ps [get_ports ADC_OTR]
