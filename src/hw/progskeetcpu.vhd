library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity progskeetcpu is
    Port (
		CLK, RST_LOCAL: in std_logic;
		P : inout std_logic_vector(49 downto 1); -- 50 pin connector on the board
		
		
		fifo_empty, fifo_full : in std_logic;
		fifo_re, fifo_we : out std_logic;
		fifo_wdata : out std_logic_vector(7 downto 0);
		fifo_rdata : in std_logic_vector(7 downto 0);
		
		switch : in std_logic;
		led : out std_logic;
		
		SPI_MISO : in std_logic;
		SPI_MOSI, SPI_SCK, SPI_CS: out std_logic
);

end progskeetcpu;


architecture behavioral of progskeetcpu is


type dataselect_t is (version_0, version_1, dq_0, dq_1, spi, timebase_0, timebase_1, temp_0, temp_1, nothing);
signal dataselect : dataselect_t;

type cmd_t is (get_version, set_counter, set_address, write_data, read_data, cmd_set_config, set_gpio, set_directions, nop, set_pinout_nor, set_pinout_nand, set_timeout, data_polling, spi_write, spi_read, wait_rdy_low, wait_rdy_high, precharge_rdy, read_timebase, end_of_packet, padding, unrecognized);
signal cmd : cmd_t;


constant version : std_logic_vector(15 downto 0) := X"0104";

signal address : std_logic_vector(31 downto 0);
signal inc_address, set_address_0, set_address_1, set_address_2, set_address_3 : std_logic;
signal delay, n_delay, delay_counter, cmd_reg, i_fifo_wdata : std_logic_vector(7 downto 0);
signal state, n_state : integer;

signal substate, restore_state, n_substate : std_logic_vector(15 downto 0);
signal n_spi_state, spi_state : integer range 0 to 15;


signal dec_cCount, set_cCount_0, set_cCount_1 : std_logic;
signal rst_delay_counter, done_delay_counter : std_logic;
signal cCount, output_data, i_GPIO, directions, n_directions,
inputdata, tgpio : std_logic_vector(15 downto 0);
signal set_output_data_0, set_output_data_1 : std_logic;
signal DQ, p_DQ, cnt_timeout, temp, n_temp : std_logic_vector(15 downto 0);


signal double, n_double, drive_enable, n_drive_enable, tristate, n_tristate,
i_OE, i_WE, waitrdy, n_waitrdy, byteswap, n_byteswap : std_logic;
signal rdy, done_timeout, rst_timeout : std_logic;
signal isnor, n_isnor : std_logic;

signal ref_timeout, timebase : std_logic_vector(15 downto 0);
signal set_timeout_0, set_timeout_1, set_toggle_mask, set_timeout_mask, set_timeout_value : std_logic;
signal toggle_mask, timeout_mask, timeout_value, spi_data : std_logic_vector(7 downto 0);

signal set_last_rdy, last_rdy, set_tgpio_0, set_tgpio_1, do_xor, set_cmd, inc_substate, rst_substate, set_config, set_directions_0, set_directions_1,
i_fifo_we, i_fifo_re, n_fifo_we, set_spi_data: std_logic;

 signal precharge : std_logic;

constant LEGACY_PIN_A22 : integer := 31;
constant LEGACY_PIN_A0 : integer := 9;
constant LEGACY_PIN_OE : integer := 8;
constant LEGACY_PIN_WE : integer := 7;
constant LEGACY_PIN_RDY : integer := 6;
constant LEGACY_PIN_GP0 : integer := 5;
constant LEGACY_PIN_GP5 : integer := 0;


signal i_SPI_MOSI, i_SPI_MISO, i_SPI_SCK, n_SPI_MOSI, n_SPI_SCK, i_led, n_led, done_spi, do_spi : std_logic;

-- GP0 - CLE
-- GP1 - ALE
-- GP2 - WP
-- GP3 - CE

-- should be
-- GPIO(7) - CLE	- i_GPIO(0)
-- GPIO(6) - ALE	- i_GPIO(1)
-- GPIO(5) - WE		- i_WE
-- GPIO(4) - RE		- i_OE
-- GPIO(3) - CE		- i_GPIO(3)
-- GPIO(2) - WP		- i_GPIO(2)
-- GPIO(1) - CE_B	- i_GPIO(4)
-- GPIO(0) - R/B 	- RDY


signal port_xor : std_logic_vector(2 downto 0);
signal xor_value : std_logic_vector(7 downto 0);

signal port_rdy : std_logic_vector(2 downto 0);
signal rdy_value, rdy_mask : std_logic_vector(7 downto 0);


begin


process(address, done_delay_counter, state, double, delay, cCount,
directions, tristate, waitrdy, byteswap,
p_dq, rdy, fifo_empty, fifo_full, i_fifo_we, done_timeout, temp, isnor, toggle_mask, timeout_value, timeout_mask,

done_spi,

last_rdy, drive_enable,

cmd, substate, i_led
)
begin
	
		n_temp <= temp;
		n_state <= state;
		n_double <= double;
		n_delay <= delay;
		n_drive_enable <= drive_enable;
		
		set_cCount_0 <= '0';
		set_cCount_1 <= '0';
		dec_cCount <= '0';
		
		set_tgpio_0 <= '0';
		set_tgpio_1 <= '0';
		
		n_directions <= directions;
		n_tristate <= tristate;
		n_waitrdy <= waitrdy;
		n_byteswap <= byteswap;
		set_address_0 <= '0';
		set_address_1 <= '0';
		set_address_2 <= '0';
		set_address_3 <= '0';
		inc_address <= '0';
		set_output_data_0 <= '0';
		set_output_data_1 <= '0';
		
		do_xor <= '0';
		i_fifo_re <= '0';
		n_fifo_we <= '0';
		rst_timeout <= '1';
		
		rst_delay_counter <= '1';
		n_isnor <= isnor;
		
		set_timeout_0 <= '0';
		set_timeout_1 <= '0';
		
		n_temp <= temp;
		
		set_toggle_mask <= '0';
		set_timeout_mask <= '0';
		set_timeout_value <= '0';
		

		
		precharge <= '0';
		
		set_last_rdy <= '0';
		
		
		inc_substate <= '0';
		rst_substate <= '0';
		
		set_directions_0 <= '0';
		set_directions_1 <= '0';
		
		set_config <= '0';
		set_cmd <= '0';
		n_led <= i_led;
		
		restore_state <= X"0001";
		do_spi <= '0';
		set_spi_data <= '0';
		dataselect <= nothing;
		
		case state is
			when 0 =>
				n_led <= '0';
				
				if fifo_empty /= '1' then
					i_fifo_re <= '1';
					n_state <= 1;
				end if;
			when 1 =>
				set_cmd <= '1';
				rst_substate <= '1'; 
				n_state <= 2;
			when 2 =>
				
				case cmd is
					when get_version => --X"00" => -- get version
						case substate(7 downto 0) is
							when X"01" =>
								dataselect <= version_0;
								if i_fifo_we = '0' then
									n_fifo_we <= not fifo_full;
								else
									inc_substate <= '1';
								end if;
							when X"02" =>
								dataselect <= version_1;
								if i_fifo_we = '0' then
									n_fifo_we <= not fifo_full;
								else
									n_state <= 0;
								end if;
								
								
							when others =>
								n_state <= 0;
						end case;
					
					when set_counter => --X"01" => -- set counter
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"02" =>
								set_cCount_0 <= '1';
								
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"04" =>
								set_cCount_1 <= '1';
								n_state <= 0;
							when others =>
								n_state <= 0;
						end case;
					
					when set_address => --X"02" => -- set address
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"02" =>
								set_address_0 <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"04" =>
								set_address_1 <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"08" =>
								set_address_2 <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"10" =>
								set_address_3 <= '1';
								n_state <= 0;
							when others =>
								n_state <= 0;
						end case;
					
					
					when write_data => --X"03" => -- write data
					n_drive_enable <= '1';
						case substate(7 downto 0) is
							when X"01" =>
								--n_drive_enable <= '1';
								rst_timeout <= '0';
								precharge <= '0';
								if (waitrdy = '1' and ((RDY = '1') or (done_timeout = '1'))) or (waitrdy = '0') then
									inc_substate <= '1';
								end if;
							when X"02" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"04" =>
								set_output_data_0 <= not byteswap;
								set_output_data_1 <= byteswap;
								if double = '1' then
									if fifo_empty = '0' then
										inc_substate <= '1';
										i_fifo_re <= '1';
									end if;
								else
									inc_substate <= '1';
								end if;
								
								
							when X"08" =>
								if double = '1' then
									set_output_data_0 <= byteswap;
									set_output_data_1 <= not byteswap;
								end if;
								inc_substate <= '1';
							when X"10" =>
								do_xor <= '1';
								inc_substate <= '1';
							when X"20" =>
								rst_delay_counter <= '0';
								if done_delay_counter = '1' then
									inc_substate <= '1';
								end if;
							when X"40" =>
								do_xor <= '1';
								inc_substate <= '1';
							when X"80" =>
								rst_delay_counter <= '0';
								if done_delay_counter = '1' then
									n_state <= 3;
									inc_substate <= '1';
								end if;
							when others =>
								n_state <= 0;
						end case;
					
					when read_data => --X"04" => -- read data
						n_drive_enable <= '0';
						case substate(7 downto 0) is
							when X"01" =>
								rst_timeout <= '0';
								precharge <= '0';
								if (waitrdy = '1' and ((RDY = '1') or (done_timeout = '1'))) or (waitrdy = '0') then
									inc_substate <= '1';
								end if;
							when X"02" =>
								do_xor <= '1';
								inc_substate <= '1';
							when X"04" =>
								rst_delay_counter <= '0';
								n_temp <= p_dq;
								if(done_delay_counter = '1') then
									inc_substate <= '1';
								end if;
							when X"08" =>
								if byteswap = '0' then
									dataselect <= temp_0;
								else
									dataselect <= temp_1;
								end if;
								
								if i_fifo_we = '0' then
									n_fifo_we <= not fifo_full;
								else
									inc_substate <= '1';
								end if;
							when X"10" =>
								if byteswap = '1' then
									dataselect <= temp_0;
								else
									dataselect <= temp_1;
								end if;
								
								if (double = '0') or (i_fifo_we = '1') then
									inc_substate <= '1';
								else
									n_fifo_we <= not fifo_full;
								end if;
								
							when X"20" =>
								do_xor <= '1';
								inc_substate <= '1';
							when X"40" =>
								rst_delay_counter <= '0';
								if done_delay_counter = '1' then
									n_state <= 3; -- state 3 decrements cCounter and re-executes command (if counter is not 0) and increment address
									inc_substate <= '1';
								end if;
							when others =>
								n_state <= 0;
							
						end case;
					
					when cmd_set_config => --X"05" => -- set config
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"02" =>
								set_config <= '1';
								n_state <= 0;
							when others =>
								n_state <= 0;
						end case;
						
					when set_gpio => --X"06" => -- set gpio
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"02" =>
								set_tgpio_0 <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"04" =>
								set_tgpio_1 <= '1';
								n_state <= 0;
							when others =>
								n_state <= 0;
							
						end case;
									
					when set_directions => --X"07" => -- set directions
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1'; 
								end if;
							when X"02" =>
								set_directions_0 <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"04" =>
								set_directions_1 <= '1';
								n_state <= 0;
							when others =>
								n_state <= 0;
							
						end case;
									
					when nop => --X"09" => -- NOP
						dec_cCount <= '1';
						if cCount = X"0000" then
							n_state <= 0;
						end if;
									
					when set_pinout_nor => --X"0A" => -- set nor flag
						n_state <= 0;
						n_isnor <= '0';
						
					when set_pinout_nand => --X"0B" =>
						n_state <= 0;
						n_isnor <= '1';
						
					when set_timeout => --X"0C" =>
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1'; 
								end if;
							when X"02" =>
								set_timeout_0 <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"04" =>
								set_timeout_1 <= '1';
								n_state <= 0;
							when others =>
								n_state <= 0;
						end case;
						
						
						
					when data_polling => --X"0D" =>
								n_drive_enable <= '0';
						case substate is
							when X"0001" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1'; --n_substate <= 1;
								end if;
							
							when X"0002" =>
								set_toggle_mask <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1'; --n_substate <= 2;
								end if;
							when X"0004" =>
								set_timeout_mask <= '1';
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1'; --n_substate <= 3;
								end if;
							when X"0008" =>
								set_timeout_value <= '1';
								inc_substate <= '1'; --n_substate <= 4;
						
								-- 4     5      6        7     8     9    10        11
								-- xor, wait, read+xor, wait, xor, wait, read+xor, wait
								-- 4,    5,     6,       5,    4,   5,      6,      5
								
								
							when X"0010" | X"0100"  =>
								do_xor <= '1';
								inc_substate <= '1'; --n_substate <= substate + 1;
							when X"0020" | X"0080" | X"0200" | X"0800" =>
								rst_delay_counter <= '0';
								if done_delay_counter = '1' then
									inc_substate <= '1'; --n_substate <= substate + 1;
								end if;
							when X"0040" | X"0400" =>
								if byteswap = '0' then
									n_temp <= temp(7 downto 0) & p_DQ(7 downto 0);
								else
									n_temp <= temp(7 downto 0) & p_DQ(15 downto 8);
								end if;
								do_xor <= '1';
								inc_substate <= '1'; --n_substate <= substate+1;
								
								
							when X"1000" =>
								restore_state <= X"0010";
								--if ((temp(15 downto 8) and toggle_mask) /= (temp(7 downto 0) and toggle_mask)) then
								if (((temp(15 downto 8) xor temp(7 downto 0)) and toggle_mask) = X"00") or
									( (temp(7 downto 0) and timeout_mask) = (timeout_value and timeout_mask) ) then
									n_state <= 0; --n_substate <= 4;
								else
									rst_substate <= '1'; --n_substate <= 0;
								end if;
							when others =>
								n_state <= 0;
						end case;
											

					when spi_write => --X"0E" => -- SPI WRITE
						case substate(7 downto 0) is
							when X"01" =>
								if fifo_empty = '0' then
									i_fifo_re <= '1';
									inc_substate <= '1';
								end if;
							when X"02" =>
								set_spi_data <= '1';
								do_spi <= '1';
								inc_substate <= '1';
							when X"04" =>
								if done_spi = '1' then
									n_state <= 3;
								end if;
							when others =>
								n_state <= 0;
						end case;
						
					when spi_read => --X"0F" => -- SPI READ
						case substate(7 downto 0) is
							when X"01" =>
								do_spi <= '1';
								if done_spi = '0' then
									inc_substate <= '1';
								end if;
							when X"02" =>
								if done_spi = '1' then
									inc_substate <= '1';
								end if;
							when X"04" =>
								dataselect <= spi;
								
								if i_fifo_we = '0' then
									n_fifo_we <= not fifo_full;
								else
									n_state <= 3;
								end if;
								
							when others =>
								n_state <= 0;
						end case;
					
					
					when wait_rdy_low => --X"10" => -- wait rdy low
						precharge <= '0';
						rst_timeout <= '0';
						--if last_rdy = '1' then
							if (rdy = '0') or (done_timeout = '1') then
								set_last_rdy <= '1';
								n_state <= 0;
							end if;
				
						
					when wait_rdy_high => --X"11" => -- wait rdy high
						precharge <= '0';
						rst_timeout <= '0';
						--if last_rdy = '0' then
							if (rdy = '1') or (done_timeout = '1') then
								set_last_rdy <= '1';
								n_state <= 0;
							end if;
						
						
					when precharge_rdy => --X"12" => -- precharge rdy
						precharge <= '1';
						rst_delay_counter <= '0';
						if done_delay_counter = '1' then
							n_state <= 0;
						end if;
						
					when read_timebase => --X"C4" => -- read timebase
						case substate(7 downto 0) is
							when X"01" =>
								dataselect <= timebase_0;
								if i_fifo_we = '0' then
									n_fifo_we <= not fifo_full;
								else
									inc_substate <= '1';
								end if;
							when X"02" =>
								dataselect <= timebase_1;
								if i_fifo_we = '0' then
									n_fifo_we <= not fifo_full;
								else
									n_state <= 0;
								end if;
							when others =>
								n_state <= 0;
						end case;
						
					
					when end_of_packet => --X"FE" => -- end of packet
						dataselect <= nothing;
						if i_fifo_we = '0' then
							n_fifo_we <= not fifo_full;
						else
							n_state <= 3;
						end if;
					when padding =>
						n_state <= 0;
						
					when others =>
						n_led <= '1';

				end case;

				
			when 3 =>
				if (cmd = write_data) or (cmd = read_data) then
					inc_address <= address(31);
				end if;
				
				dec_cCount <= '1';
				rst_substate <= '1';
				if cCount = X"0000" then
					n_state <= 0;
				else
					n_state <= 2;
				end if;
			
			when others =>
				n_state <= 0;
		end case;
end process;


-- P1	GP6
-- ..
-- P7	GP0
-- P8	RDY
-- P9	WE
-- P10	OE
-- P11	ADR0
-- ..
-- P33	ADR22
-- P34	DQ0
-- ..
-- P49	DQ15


process(P, double, byteswap)
begin
		if double = '1' then
		DQ <= P(49 downto 34);
	else
		if byteswap = '1' then
			DQ <= P(49 downto 42) & X"00";
		else
			DQ <= X"00" & P(41 downto 34);
		end if;
	end if;
end process;


process(clk,rst_local)
begin
	if rst_local = '0' then
		P <= (others => 'Z');
		rdy <= '0';
		p_DQ <= X"0000";
	elsif rising_edge(clk) then
	
		p_DQ <= DQ;
	--rdy <= '0';
	P(49 downto 1) <= (others => 'Z');
		
		if (tristate = '0') then
			if (drive_enable = '1') then
					P(49 downto 34) <= output_data(15 downto 0);
			end if;
			
			if isnor = '1' then
				
				for i in 3 downto 0 loop
					P(7-i) <= i_GPIO(i);
				end loop;
				
				P(1) <= address(23);
				P(2) <= address(24);
				P(3) <= address(25);
				
				P(33 downto 11) <= address(22 downto 0);
				
				P(9) <= i_WE;
				P(10) <= i_OE;
				
					RDY <= P(8);
				
				if(precharge = '1') then
				 	P(8) <= 'H';
				else
					P(8) <= 'Z';
				end if;

			else
			
				if byteswap = '0' then
					RDY <= P(2);
					if(precharge = '1') then
						P(2) <= 'H';
					else
						P(2) <= 'Z';
					end if;
					P(6) <= i_OE;
					P(7) <= i_WE;
					P(30) <= '1';
					P(31) <= '1';
				else
					RDY <= P(26);
					P(30) <= i_OE;
					P(31) <= i_WE;
					P(6) <= '1';
					P(7) <= '1';
					if(precharge = '1') then
						P(26) <= 'H';
					else
						P(26) <= 'Z';
					end if;
				end if;
						
				
				P(3) <= i_GPIO(4); -- added 130504 / 1841
				P(4) <= i_GPIO(2);
				P(5) <= i_GPIO(3);
				P(8) <= i_GPIO(1); 
				P(9) <= i_GPIO(0); 
				P(27) <= i_GPIO(8);
				P(28) <= i_GPIO(10);
				P(29) <= i_GPIO(9);
				P(32) <= i_GPIO(11);
				P(33) <= i_GPIO(12);

			end if;
		else
			P(49 downto 1) <= (others => 'Z');
		end if;

	end if;
end process;
	

process(RST_LOCAL, CLK)
begin
	if rst_local = '0' then
		address <= (others => '0');
		delay_counter <= (others => '0');

		drive_enable <= '0';
		cCount <= (others => '0');
		output_data <= (others => '0');
		delay <= (others => '0');
		directions <= (others => '0');
		double <= '0';
		tristate <= '1';
		waitrdy <= '0';
		byteswap <= '0';
		tgpio <= (others => '1');
		temp <= (others => '0');
		isnor <= '0';
		ref_timeout <= X"0FFF";
		timeout_mask <= X"00";
		timeout_value <= X"00";
		toggle_mask <= X"00";
		temp <= X"0000";
		i_SPI_MOSI <= '0'; i_SPI_SCK <= '1';
		last_rdy <= '1';
		timebase <= X"0000";
		i_OE <= '1';
		i_WE <= '1';
		cmd_reg <= X"00";
		i_led <= '1';
		led <= '0';
		spi_state <= 0;
		spi_data <= X"00";
		i_SPI_MOSI <= '0';
		i_SPI_SCK <= '0';
		
		done_delay_counter <= '0';
	elsif rising_edge(CLK) then
	
	
		spi_state <= n_spi_state;
		
		timebase <= timebase + X"0001";
	
		if set_last_rdy = '1' then
			last_rdy <= rdy;
		end if;
		

		
		i_SPI_SCK <= n_SPI_SCK;
		
	
		if set_toggle_mask = '1' then
			toggle_mask <= fifo_rdata;
		end if;
		
		if set_timeout_mask = '1' then
			timeout_mask <= fifo_rdata;
		end if;
		
		if set_timeout_value = '1' then
			timeout_value <= fifo_rdata;
		end if;
	
		if set_timeout_0 = '1' then
			ref_timeout(7 downto 0) <= fifo_rdata;
		end if;
		
		if set_timeout_1 = '1' then
			ref_timeout(15 downto 8) <= fifo_rdata;
		end if;
	
	
		isnor <= n_isnor;
		if set_address_0 = '1' then
			address(7 downto 0) <= fifo_rdata;
		end if;
	
		if set_address_1 = '1' then
			address(15 downto 8) <= fifo_rdata;
		end if;
		
		if set_address_2 = '1' then
			address(23 downto 16) <= fifo_rdata;
		end if;
		
		if set_address_3 = '1' then
			address(31 downto 24) <= fifo_rdata;
		end if;
		
		
		if inc_address = '1' then
			address <= address + X"00000001";
		end if;
		
		if set_cCount_0 = '1' then
			cCount(7 downto 0) <= fifo_rdata;
		end if;
		
		if set_cCount_1 = '1' then
			cCount(15 downto 8) <= fifo_rdata;
		end if;
		
		if (dec_cCount = '1') and (cCount /= X"0000") then
			cCount <= cCount - X"0001";
		end if;
		
		if (rst_delay_counter = '1') or ((i_OE = '0') and (DQ /= p_DQ)) then
			delay_counter <= delay;
		else
			if delay_counter /= X"00" then
				delay_counter <= delay_counter - X"01";
			else
				done_delay_counter <= '1';
			end if;
		end if;
		

		drive_enable <= n_drive_enable;
		if set_output_data_0 = '1' then
			output_data(7 downto 0) <= fifo_rdata;
		end if;
		
		if set_output_data_1 = '1' then
			output_data(15 downto 8) <= fifo_rdata;
		end if;
		
		delay <= n_delay;
		
		if set_directions_0 = '1' then
			directions(7 downto 0) <= fifo_rdata;
		end if;
		
		if set_directions_1 = '1' then
			directions(15 downto 8) <= fifo_rdata;
		end if;
		
		
		if set_config = '1' then
				delay <= X"0" & fifo_rdata(3 downto 0);
				double <= fifo_rdata(4);
				tristate <= fifo_rdata(5);
				waitrdy <= fifo_rdata(6);
				byteswap <= fifo_rdata(7);
		end if;
		
		
		if set_tgpio_0 = '1' then
			tgpio(7 downto 0) <= fifo_rdata;
		end if;
		
		if set_tgpio_1 = '1' then
			tgpio(15 downto 8) <= fifo_rdata;
		end if;
		
		if do_xor = '1' then
		
			if cmd = write_data then
				i_WE <= i_WE xor '1';
			end if;
			
			if cmd = read_data then
				i_OE <= i_OE xor '1';
			end if;
			
		end if;
		
		
		
		if set_cmd = '1' then
			cmd_reg <= fifo_rdata;
		end if;
		
		i_led <= n_led;
		led <= n_led;

		temp <= n_temp;
		
		
		if set_spi_data = '1' then
			spi_data <= fifo_rdata;
		elsif (i_spi_sck = '0') and (n_spi_sck = '1') then
			spi_data <= spi_data(6 downto 0) & SPI_MISO;
		end if;
		
		
		i_SPI_MOSI <= spi_data(7);
		
	end if;
end process;


		fifo_re <= i_fifo_re;

process(rst_local,clk)
begin
	if rst_local = '0' then
		i_fifo_we <= '0';
		fifo_we <= '0';
	elsif rising_edge(clk) then
		fifo_we <= n_fifo_we;
		i_fifo_we <= n_fifo_we;
	end if;
end process;
		fifo_wdata <= i_fifo_wdata;



process(tgpio, directions, i_OE, i_WE, byteswap, tristate)
begin

	if tristate = '0' then
		if (byteswap = '1') then
			i_GPIO(15) <= i_OE;
			i_GPIO(14) <= i_WE;
		else
			if directions(15) = '1' then
				i_GPIO(15) <= tgpio(15);
			else
				i_GPIO(15) <= 'Z';
			end if;
			
			if directions(14) = '1' then
				i_GPIO(14) <= tgpio(14);
			else
				i_GPIO(14) <= 'Z';
			end if;
		end if;
		  
		for i in 0 to 13 loop
			if directions(i) = '1' then
				i_GPIO(i) <= tgpio(i);
			else
				i_GPIO(i) <= 'Z';
			end if;
		end loop;

	else
		i_GPIO <= (others => 'Z');
	end if;
end process;

process(clk)
begin
	if rising_edge(clk) then
		if rst_timeout = '1' then
			done_timeout <= '0';
			cnt_timeout <= ref_timeout;
		else 
			if cnt_timeout /= X"0000" then
				cnt_timeout <= cnt_timeout - X"0001";
			else
				done_timeout <= '1';
			end if;
		end if;
	end if;
end process;


process(rst_local, clk)
begin
	if rst_local = '0' then
		substate <= X"0000";
	elsif rising_edge(clk) then
		if rst_substate = '1' then
				substate <= restore_state;
		elsif inc_substate = '1' then
				substate <= n_substate;
		end if;
	end if;
end process;

n_substate <= substate(14 downto 0) & '0';

process(rst_local, clk)
begin
	if rst_local = '0' then
		state <= 0;
	elsif rising_edge(clk) then
		state <= n_state;
	end if;
end process;



process(i_spi_sck, spi_state, do_spi)
begin

	done_spi <= '0';
	n_spi_state <= spi_state;
	n_SPI_SCK <= i_SPI_SCK;
	case spi_state is
		when 0 =>
			if (do_spi = '1') then
				n_spi_state <= spi_state + 1;
			end if;
			n_SPI_SCK <= '0';
		when 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 =>
			n_SPI_SCK <= not i_SPI_SCK;
			if i_SPI_SCK = '1' then
				n_spi_state <= spi_state + 1;
			end if;
		
		when 9 =>
			done_spi <= '1';
			n_spi_state <= 0;
		when others => 
			n_spi_state <= 0;
	end case;
end process;

process(p_dq, spi_data, timebase, dataselect, temp)
begin
		case dataselect is
			when version_0 =>
				i_fifo_wdata <= version(7 downto 0);
			when version_1 =>
				i_fifo_wdata <= version(15 downto 8);
			when dq_0 =>
				i_fifo_wdata <= p_dq(7 downto 0);
			when dq_1 =>
				i_fifo_wdata <= p_dq(15 downto 8);
			when spi =>
				i_fifo_wdata <= spi_data;
			when timebase_0 =>
				i_fifo_wdata <= timebase(7 downto 0);
			when timebase_1 =>
				i_fifo_wdata <= timebase(15 downto 8);
			when temp_0 =>
				i_fifo_wdata <= temp(7 downto 0);
			when temp_1 =>
				i_fifo_wdata <= temp(15 downto 8);
			when nothing =>
				i_fifo_wdata <= X"00";
		end case;
end process;
			
process(cmd_reg)
begin
	case cmd_reg is
		when X"00" =>
			cmd <= get_version;
		when X"01" =>
			cmd <= set_counter;
		when X"02" =>
			cmd <= set_address;
		when X"03" =>
			cmd <= write_data;
		when X"04" =>
			cmd <= read_data;
		when X"05" =>
			cmd <= cmd_set_config;
		when X"06" =>
			cmd <= set_gpio;
		when X"07" =>
			cmd <= set_directions;
		when X"09" =>
			cmd <= nop;
		when X"0A" =>
			cmd <= set_pinout_nor;
		when X"0B" =>
			cmd <= set_pinout_nand;
		when X"0C" =>
			cmd <= set_timeout;
		when X"0D" =>
			cmd <= data_polling;
		when X"0E" =>
			cmd <= spi_write;
		when X"0F" =>
			cmd <= spi_read;
		when X"10" =>
			cmd <= wait_rdy_low;
		when X"11" =>
			cmd <= wait_rdy_high;
		when X"12" =>
			cmd <= precharge_rdy;
		when X"C4" =>
			cmd <= read_timebase;
		when X"FE" =>
			cmd <= end_of_packet;
		when X"FF" =>
			cmd <= padding;
		when others =>
			cmd <= unrecognized;
	end case;
end process;


-- Note: SPI looks good in testbench, doesn't work in implementation
-- SCK		0101010101010101
-- n_SCK	1010101010101010
-- MOSI		X7
-- n_MOSI	7

 
 SPI_SCK <= i_SPI_SCK when tristate = '0' else 'Z';
 SPI_CS <= i_GPIO(7);
 SPI_MOSI <= i_SPI_MOSI when tristate = '0' else 'Z';

end behavioral;