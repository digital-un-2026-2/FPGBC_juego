# Puente de lectura/quemado del cartucho para la FPGA GW2AR-18 de la FPGBC.
# Usar dentro del entorno pixi de FPGBC_digital_UN (ver README.md).

TOP      = cart_bridge
SRC      = $(wildcard rtl/datapath/*.v) rtl/uart_rx.v rtl/control_uart_rx.v rtl/uart_tx.v rtl/control_uart_tx.v \
           rtl/cart_bus.v rtl/control_cart_bus.v rtl/control_bridge.v rtl/cart_bridge.v
CST      = constraints/fpgbc_cart.cst
SDC      = constraints/fpgbc_cart.sdc
BUILD    = build
DEVICE   = GW2AR-LV18EQ144PC8/I7
FAMILY   = GW2A-18C

YOSYS    ?= $(shell command -v yosys || echo yowasp-yosys)
NEXTPNR  ?= $(shell command -v nextpnr-himbaechel-gowin || echo yowasp-nextpnr-himbaechel-gowin)
PACK     ?= gowin_pack
JTAG_FREQ ?= 1000000
CABLE    ?= --cable ft232RL --pins TXD:CTS:DTR:RXD

.PHONY: sim build load detect clean

# Simulacion con el modelo del cartucho (no necesita la consola)
sim:
	mkdir -p $(BUILD)
	iverilog -g2005 -DBENCH -o $(BUILD)/tb.vvp sim/tb_cart_bridge.v sim/cart_model.v $(SRC)
	cd $(BUILD) && vvp -n tb.vvp

# Sintesis, place & route y empaquetado del bitstream
build: $(BUILD)/$(TOP).fs

$(BUILD)/$(TOP).fs: $(SRC) $(CST) $(SDC)
	mkdir -p $(BUILD)
	$(YOSYS) -q -l $(BUILD)/synth.log -p 'read_verilog $(SRC); synth_gowin -family gw2a -top $(TOP) -json $(BUILD)/synth.json'
	$(NEXTPNR) --device $(DEVICE) --top $(TOP) --json $(BUILD)/synth.json --write $(BUILD)/pnr.json \
	    --sdc $(SDC) --freq 40 --vopt family=$(FAMILY) --vopt cst=$(CST) --log $(BUILD)/pnr.log
	$(PACK) -d $(FAMILY) -o $@ $(BUILD)/pnr.json

# Carga en SRAM (se pierde al apagar; no toca la flash de configuracion de la consola)
load: $(BUILD)/$(TOP).fs
	openFPGALoader $(CABLE) --freq $(JTAG_FREQ) --write-sram --bitstream $<

detect:
	openFPGALoader $(CABLE) --freq $(JTAG_FREQ) --detect -f

clean:
	rm -rf $(BUILD)
