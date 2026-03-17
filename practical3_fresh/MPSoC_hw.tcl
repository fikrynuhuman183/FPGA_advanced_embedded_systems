package require -exact qsys 13.1

# module properties
set_module_property NAME {MPSoC_export}
set_module_property DISPLAY_NAME {MPSoC_export_display}

# default module properties
set_module_property VERSION {1.0}
set_module_property GROUP {default group}
set_module_property DESCRIPTION {default description}
set_module_property AUTHOR {author}

set_module_property COMPOSITION_CALLBACK compose
set_module_property opaque_address_map false

proc compose { } {
    # Instances and instance parameters
    # (disabled instances are intentionally culled)
    add_instance clk_0 clock_source 13.1
    set_instance_parameter_value clk_0 {clockFrequency} {50000000.0}
    set_instance_parameter_value clk_0 {clockFrequencyKnown} {1}
    set_instance_parameter_value clk_0 {resetSynchronousEdges} {NONE}

    add_instance cpu1 altera_nios2_qsys 13.1
    set_instance_parameter_value cpu1 {setting_showUnpublishedSettings} {0}
    set_instance_parameter_value cpu1 {setting_showInternalSettings} {0}
    set_instance_parameter_value cpu1 {setting_preciseSlaveAccessErrorException} {0}
    set_instance_parameter_value cpu1 {setting_preciseIllegalMemAccessException} {0}
    set_instance_parameter_value cpu1 {setting_preciseDivisionErrorException} {0}
    set_instance_parameter_value cpu1 {setting_performanceCounter} {0}
    set_instance_parameter_value cpu1 {setting_illegalMemAccessDetection} {0}
    set_instance_parameter_value cpu1 {setting_illegalInstructionsTrap} {0}
    set_instance_parameter_value cpu1 {setting_fullWaveformSignals} {0}
    set_instance_parameter_value cpu1 {setting_extraExceptionInfo} {0}
    set_instance_parameter_value cpu1 {setting_exportPCB} {0}
    set_instance_parameter_value cpu1 {setting_debugSimGen} {0}
    set_instance_parameter_value cpu1 {setting_clearXBitsLDNonBypass} {1}
    set_instance_parameter_value cpu1 {setting_bit31BypassDCache} {1}
    set_instance_parameter_value cpu1 {setting_bigEndian} {0}
    set_instance_parameter_value cpu1 {setting_export_large_RAMs} {0}
    set_instance_parameter_value cpu1 {setting_asic_enabled} {0}
    set_instance_parameter_value cpu1 {setting_asic_synopsys_translate_on_off} {0}
    set_instance_parameter_value cpu1 {setting_oci_export_jtag_signals} {0}
    set_instance_parameter_value cpu1 {setting_bhtIndexPcOnly} {0}
    set_instance_parameter_value cpu1 {setting_avalonDebugPortPresent} {0}
    set_instance_parameter_value cpu1 {setting_alwaysEncrypt} {1}
    set_instance_parameter_value cpu1 {setting_allowFullAddressRange} {0}
    set_instance_parameter_value cpu1 {setting_activateTrace} {1}
    set_instance_parameter_value cpu1 {setting_activateTrace_user} {0}
    set_instance_parameter_value cpu1 {setting_activateTestEndChecker} {0}
    set_instance_parameter_value cpu1 {setting_ecc_sim_test_ports} {0}
    set_instance_parameter_value cpu1 {setting_activateMonitors} {1}
    set_instance_parameter_value cpu1 {setting_activateModelChecker} {0}
    set_instance_parameter_value cpu1 {setting_HDLSimCachesCleared} {1}
    set_instance_parameter_value cpu1 {setting_HBreakTest} {0}
    set_instance_parameter_value cpu1 {setting_breakslaveoveride} {0}
    set_instance_parameter_value cpu1 {muldiv_divider} {0}
    set_instance_parameter_value cpu1 {mpu_useLimit} {0}
    set_instance_parameter_value cpu1 {mpu_enabled} {0}
    set_instance_parameter_value cpu1 {mmu_enabled} {0}
    set_instance_parameter_value cpu1 {mmu_autoAssignTlbPtrSz} {1}
    set_instance_parameter_value cpu1 {manuallyAssignCpuID} {1}
    set_instance_parameter_value cpu1 {debug_triggerArming} {1}
    set_instance_parameter_value cpu1 {debug_embeddedPLL} {1}
    set_instance_parameter_value cpu1 {debug_debugReqSignals} {0}
    set_instance_parameter_value cpu1 {debug_assignJtagInstanceID} {0}
    set_instance_parameter_value cpu1 {dcache_omitDataMaster} {0}
    set_instance_parameter_value cpu1 {cpuReset} {0}
    set_instance_parameter_value cpu1 {is_hardcopy_compatible} {0}
    set_instance_parameter_value cpu1 {setting_shadowRegisterSets} {0}
    set_instance_parameter_value cpu1 {mpu_numOfInstRegion} {8}
    set_instance_parameter_value cpu1 {mpu_numOfDataRegion} {8}
    set_instance_parameter_value cpu1 {mmu_TLBMissExcOffset} {0}
    set_instance_parameter_value cpu1 {debug_jtagInstanceID} {0}
    set_instance_parameter_value cpu1 {resetOffset} {0}
    set_instance_parameter_value cpu1 {exceptionOffset} {32}
    set_instance_parameter_value cpu1 {cpuID} {0}
    set_instance_parameter_value cpu1 {cpuID_stored} {0}
    set_instance_parameter_value cpu1 {breakOffset} {32}
    set_instance_parameter_value cpu1 {userDefinedSettings} {}
    set_instance_parameter_value cpu1 {resetSlave} {onchip_cpu1.s1}
    set_instance_parameter_value cpu1 {mmu_TLBMissExcSlave} {None}
    set_instance_parameter_value cpu1 {exceptionSlave} {onchip_cpu1.s1}
    set_instance_parameter_value cpu1 {breakSlave} {cpu1.jtag_debug_module}
    set_instance_parameter_value cpu1 {setting_perfCounterWidth} {32}
    set_instance_parameter_value cpu1 {setting_interruptControllerType} {Internal}
    set_instance_parameter_value cpu1 {setting_branchPredictionType} {Automatic}
    set_instance_parameter_value cpu1 {setting_bhtPtrSz} {8}
    set_instance_parameter_value cpu1 {muldiv_multiplierType} {EmbeddedMulFast}
    set_instance_parameter_value cpu1 {mpu_minInstRegionSize} {12}
    set_instance_parameter_value cpu1 {mpu_minDataRegionSize} {12}
    set_instance_parameter_value cpu1 {mmu_uitlbNumEntries} {4}
    set_instance_parameter_value cpu1 {mmu_udtlbNumEntries} {6}
    set_instance_parameter_value cpu1 {mmu_tlbPtrSz} {7}
    set_instance_parameter_value cpu1 {mmu_tlbNumWays} {16}
    set_instance_parameter_value cpu1 {mmu_processIDNumBits} {8}
    set_instance_parameter_value cpu1 {impl} {Tiny}
    set_instance_parameter_value cpu1 {icache_size} {4096}
    set_instance_parameter_value cpu1 {icache_tagramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {icache_ramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {icache_numTCIM} {0}
    set_instance_parameter_value cpu1 {icache_burstType} {None}
    set_instance_parameter_value cpu1 {dcache_bursts} {false}
    set_instance_parameter_value cpu1 {dcache_victim_buf_impl} {ram}
    set_instance_parameter_value cpu1 {debug_level} {Level1}
    set_instance_parameter_value cpu1 {debug_OCIOnchipTrace} {_128}
    set_instance_parameter_value cpu1 {dcache_size} {2048}
    set_instance_parameter_value cpu1 {dcache_tagramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {dcache_ramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {dcache_numTCDM} {0}
    set_instance_parameter_value cpu1 {dcache_lineSize} {32}
    set_instance_parameter_value cpu1 {setting_exportvectors} {0}
    set_instance_parameter_value cpu1 {setting_ecc_present} {0}
    set_instance_parameter_value cpu1 {setting_ic_ecc_present} {1}
    set_instance_parameter_value cpu1 {setting_rf_ecc_present} {1}
    set_instance_parameter_value cpu1 {setting_mmu_ecc_present} {1}
    set_instance_parameter_value cpu1 {setting_dc_ecc_present} {0}
    set_instance_parameter_value cpu1 {setting_itcm_ecc_present} {0}
    set_instance_parameter_value cpu1 {setting_dtcm_ecc_present} {0}
    set_instance_parameter_value cpu1 {regfile_ramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {ocimem_ramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {mmu_ramBlockType} {Automatic}
    set_instance_parameter_value cpu1 {bht_ramBlockType} {Automatic}

    add_instance cpu0 altera_nios2_qsys 13.1
    set_instance_parameter_value cpu0 {setting_showUnpublishedSettings} {0}
    set_instance_parameter_value cpu0 {setting_showInternalSettings} {0}
    set_instance_parameter_value cpu0 {setting_preciseSlaveAccessErrorException} {0}
    set_instance_parameter_value cpu0 {setting_preciseIllegalMemAccessException} {0}
    set_instance_parameter_value cpu0 {setting_preciseDivisionErrorException} {0}
    set_instance_parameter_value cpu0 {setting_performanceCounter} {0}
    set_instance_parameter_value cpu0 {setting_illegalMemAccessDetection} {0}
    set_instance_parameter_value cpu0 {setting_illegalInstructionsTrap} {0}
    set_instance_parameter_value cpu0 {setting_fullWaveformSignals} {0}
    set_instance_parameter_value cpu0 {setting_extraExceptionInfo} {0}
    set_instance_parameter_value cpu0 {setting_exportPCB} {0}
    set_instance_parameter_value cpu0 {setting_debugSimGen} {0}
    set_instance_parameter_value cpu0 {setting_clearXBitsLDNonBypass} {1}
    set_instance_parameter_value cpu0 {setting_bit31BypassDCache} {1}
    set_instance_parameter_value cpu0 {setting_bigEndian} {0}
    set_instance_parameter_value cpu0 {setting_export_large_RAMs} {0}
    set_instance_parameter_value cpu0 {setting_asic_enabled} {0}
    set_instance_parameter_value cpu0 {setting_asic_synopsys_translate_on_off} {0}
    set_instance_parameter_value cpu0 {setting_oci_export_jtag_signals} {0}
    set_instance_parameter_value cpu0 {setting_bhtIndexPcOnly} {0}
    set_instance_parameter_value cpu0 {setting_avalonDebugPortPresent} {0}
    set_instance_parameter_value cpu0 {setting_alwaysEncrypt} {1}
    set_instance_parameter_value cpu0 {setting_allowFullAddressRange} {0}
    set_instance_parameter_value cpu0 {setting_activateTrace} {1}
    set_instance_parameter_value cpu0 {setting_activateTrace_user} {0}
    set_instance_parameter_value cpu0 {setting_activateTestEndChecker} {0}
    set_instance_parameter_value cpu0 {setting_ecc_sim_test_ports} {0}
    set_instance_parameter_value cpu0 {setting_activateMonitors} {1}
    set_instance_parameter_value cpu0 {setting_activateModelChecker} {0}
    set_instance_parameter_value cpu0 {setting_HDLSimCachesCleared} {1}
    set_instance_parameter_value cpu0 {setting_HBreakTest} {0}
    set_instance_parameter_value cpu0 {setting_breakslaveoveride} {0}
    set_instance_parameter_value cpu0 {muldiv_divider} {0}
    set_instance_parameter_value cpu0 {mpu_useLimit} {0}
    set_instance_parameter_value cpu0 {mpu_enabled} {0}
    set_instance_parameter_value cpu0 {mmu_enabled} {0}
    set_instance_parameter_value cpu0 {mmu_autoAssignTlbPtrSz} {1}
    set_instance_parameter_value cpu0 {manuallyAssignCpuID} {1}
    set_instance_parameter_value cpu0 {debug_triggerArming} {1}
    set_instance_parameter_value cpu0 {debug_embeddedPLL} {1}
    set_instance_parameter_value cpu0 {debug_debugReqSignals} {0}
    set_instance_parameter_value cpu0 {debug_assignJtagInstanceID} {0}
    set_instance_parameter_value cpu0 {dcache_omitDataMaster} {0}
    set_instance_parameter_value cpu0 {cpuReset} {0}
    set_instance_parameter_value cpu0 {is_hardcopy_compatible} {0}
    set_instance_parameter_value cpu0 {setting_shadowRegisterSets} {0}
    set_instance_parameter_value cpu0 {mpu_numOfInstRegion} {8}
    set_instance_parameter_value cpu0 {mpu_numOfDataRegion} {8}
    set_instance_parameter_value cpu0 {mmu_TLBMissExcOffset} {0}
    set_instance_parameter_value cpu0 {debug_jtagInstanceID} {0}
    set_instance_parameter_value cpu0 {resetOffset} {0}
    set_instance_parameter_value cpu0 {exceptionOffset} {32}
    set_instance_parameter_value cpu0 {cpuID} {0}
    set_instance_parameter_value cpu0 {cpuID_stored} {0}
    set_instance_parameter_value cpu0 {breakOffset} {32}
    set_instance_parameter_value cpu0 {userDefinedSettings} {}
    set_instance_parameter_value cpu0 {resetSlave} {onchip_cpu0.s1}
    set_instance_parameter_value cpu0 {mmu_TLBMissExcSlave} {None}
    set_instance_parameter_value cpu0 {exceptionSlave} {onchip_cpu0.s1}
    set_instance_parameter_value cpu0 {breakSlave} {cpu0.jtag_debug_module}
    set_instance_parameter_value cpu0 {setting_perfCounterWidth} {32}
    set_instance_parameter_value cpu0 {setting_interruptControllerType} {Internal}
    set_instance_parameter_value cpu0 {setting_branchPredictionType} {Automatic}
    set_instance_parameter_value cpu0 {setting_bhtPtrSz} {8}
    set_instance_parameter_value cpu0 {muldiv_multiplierType} {EmbeddedMulFast}
    set_instance_parameter_value cpu0 {mpu_minInstRegionSize} {12}
    set_instance_parameter_value cpu0 {mpu_minDataRegionSize} {12}
    set_instance_parameter_value cpu0 {mmu_uitlbNumEntries} {4}
    set_instance_parameter_value cpu0 {mmu_udtlbNumEntries} {6}
    set_instance_parameter_value cpu0 {mmu_tlbPtrSz} {7}
    set_instance_parameter_value cpu0 {mmu_tlbNumWays} {16}
    set_instance_parameter_value cpu0 {mmu_processIDNumBits} {8}
    set_instance_parameter_value cpu0 {impl} {Tiny}
    set_instance_parameter_value cpu0 {icache_size} {4096}
    set_instance_parameter_value cpu0 {icache_tagramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {icache_ramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {icache_numTCIM} {0}
    set_instance_parameter_value cpu0 {icache_burstType} {None}
    set_instance_parameter_value cpu0 {dcache_bursts} {false}
    set_instance_parameter_value cpu0 {dcache_victim_buf_impl} {ram}
    set_instance_parameter_value cpu0 {debug_level} {Level1}
    set_instance_parameter_value cpu0 {debug_OCIOnchipTrace} {_128}
    set_instance_parameter_value cpu0 {dcache_size} {2048}
    set_instance_parameter_value cpu0 {dcache_tagramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {dcache_ramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {dcache_numTCDM} {0}
    set_instance_parameter_value cpu0 {dcache_lineSize} {32}
    set_instance_parameter_value cpu0 {setting_exportvectors} {0}
    set_instance_parameter_value cpu0 {setting_ecc_present} {0}
    set_instance_parameter_value cpu0 {setting_ic_ecc_present} {1}
    set_instance_parameter_value cpu0 {setting_rf_ecc_present} {1}
    set_instance_parameter_value cpu0 {setting_mmu_ecc_present} {1}
    set_instance_parameter_value cpu0 {setting_dc_ecc_present} {0}
    set_instance_parameter_value cpu0 {setting_itcm_ecc_present} {0}
    set_instance_parameter_value cpu0 {setting_dtcm_ecc_present} {0}
    set_instance_parameter_value cpu0 {regfile_ramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {ocimem_ramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {mmu_ramBlockType} {Automatic}
    set_instance_parameter_value cpu0 {bht_ramBlockType} {Automatic}

    add_instance timer0 altera_avalon_timer 13.1
    set_instance_parameter_value timer0 {alwaysRun} {0}
    set_instance_parameter_value timer0 {counterSize} {32}
    set_instance_parameter_value timer0 {fixedPeriod} {0}
    set_instance_parameter_value timer0 {period} {1}
    set_instance_parameter_value timer0 {periodUnits} {MSEC}
    set_instance_parameter_value timer0 {resetOutput} {0}
    set_instance_parameter_value timer0 {snapshot} {1}
    set_instance_parameter_value timer0 {timeoutPulseOutput} {0}

    add_instance timer1 altera_avalon_timer 13.1
    set_instance_parameter_value timer1 {alwaysRun} {0}
    set_instance_parameter_value timer1 {counterSize} {32}
    set_instance_parameter_value timer1 {fixedPeriod} {0}
    set_instance_parameter_value timer1 {period} {1}
    set_instance_parameter_value timer1 {periodUnits} {MSEC}
    set_instance_parameter_value timer1 {resetOutput} {0}
    set_instance_parameter_value timer1 {snapshot} {1}
    set_instance_parameter_value timer1 {timeoutPulseOutput} {0}

    add_instance sysid0 altera_avalon_sysid_qsys 13.1
    set_instance_parameter_value sysid0 {id} {0}

    add_instance sysid1 altera_avalon_sysid_qsys 13.1
    set_instance_parameter_value sysid1 {id} {1}

    add_instance jtag0 altera_avalon_jtag_uart 13.1
    set_instance_parameter_value jtag0 {allowMultipleConnections} {0}
    set_instance_parameter_value jtag0 {hubInstanceID} {0}
    set_instance_parameter_value jtag0 {readBufferDepth} {64}
    set_instance_parameter_value jtag0 {readIRQThreshold} {8}
    set_instance_parameter_value jtag0 {simInputCharacterStream} {}
    set_instance_parameter_value jtag0 {simInteractiveOptions} {NO_INTERACTIVE_WINDOWS}
    set_instance_parameter_value jtag0 {useRegistersForReadBuffer} {0}
    set_instance_parameter_value jtag0 {useRegistersForWriteBuffer} {0}
    set_instance_parameter_value jtag0 {useRelativePathForSimFile} {0}
    set_instance_parameter_value jtag0 {writeBufferDepth} {64}
    set_instance_parameter_value jtag0 {writeIRQThreshold} {8}

    add_instance jtag1 altera_avalon_jtag_uart 13.1
    set_instance_parameter_value jtag1 {allowMultipleConnections} {0}
    set_instance_parameter_value jtag1 {hubInstanceID} {0}
    set_instance_parameter_value jtag1 {readBufferDepth} {64}
    set_instance_parameter_value jtag1 {readIRQThreshold} {8}
    set_instance_parameter_value jtag1 {simInputCharacterStream} {}
    set_instance_parameter_value jtag1 {simInteractiveOptions} {NO_INTERACTIVE_WINDOWS}
    set_instance_parameter_value jtag1 {useRegistersForReadBuffer} {0}
    set_instance_parameter_value jtag1 {useRegistersForWriteBuffer} {0}
    set_instance_parameter_value jtag1 {useRelativePathForSimFile} {0}
    set_instance_parameter_value jtag1 {writeBufferDepth} {64}
    set_instance_parameter_value jtag1 {writeIRQThreshold} {8}

    add_instance onchip_cpu0 altera_avalon_onchip_memory2 13.1
    set_instance_parameter_value onchip_cpu0 {allowInSystemMemoryContentEditor} {0}
    set_instance_parameter_value onchip_cpu0 {blockType} {AUTO}
    set_instance_parameter_value onchip_cpu0 {dataWidth} {32}
    set_instance_parameter_value onchip_cpu0 {dualPort} {0}
    set_instance_parameter_value onchip_cpu0 {initMemContent} {1}
    set_instance_parameter_value onchip_cpu0 {initializationFileName} {onchip_mem.hex}
    set_instance_parameter_value onchip_cpu0 {instanceID} {NONE}
    set_instance_parameter_value onchip_cpu0 {memorySize} {40000.0}
    set_instance_parameter_value onchip_cpu0 {readDuringWriteMode} {DONT_CARE}
    set_instance_parameter_value onchip_cpu0 {simAllowMRAMContentsFile} {0}
    set_instance_parameter_value onchip_cpu0 {simMemInitOnlyFilename} {0}
    set_instance_parameter_value onchip_cpu0 {singleClockOperation} {0}
    set_instance_parameter_value onchip_cpu0 {slave1Latency} {1}
    set_instance_parameter_value onchip_cpu0 {slave2Latency} {1}
    set_instance_parameter_value onchip_cpu0 {useNonDefaultInitFile} {0}
    set_instance_parameter_value onchip_cpu0 {useShallowMemBlocks} {0}
    set_instance_parameter_value onchip_cpu0 {writable} {1}
    set_instance_parameter_value onchip_cpu0 {ecc_enabled} {0}

    add_instance onchip_cpu1 altera_avalon_onchip_memory2 13.1
    set_instance_parameter_value onchip_cpu1 {allowInSystemMemoryContentEditor} {0}
    set_instance_parameter_value onchip_cpu1 {blockType} {AUTO}
    set_instance_parameter_value onchip_cpu1 {dataWidth} {32}
    set_instance_parameter_value onchip_cpu1 {dualPort} {0}
    set_instance_parameter_value onchip_cpu1 {initMemContent} {1}
    set_instance_parameter_value onchip_cpu1 {initializationFileName} {onchip_mem.hex}
    set_instance_parameter_value onchip_cpu1 {instanceID} {NONE}
    set_instance_parameter_value onchip_cpu1 {memorySize} {40000.0}
    set_instance_parameter_value onchip_cpu1 {readDuringWriteMode} {DONT_CARE}
    set_instance_parameter_value onchip_cpu1 {simAllowMRAMContentsFile} {0}
    set_instance_parameter_value onchip_cpu1 {simMemInitOnlyFilename} {0}
    set_instance_parameter_value onchip_cpu1 {singleClockOperation} {0}
    set_instance_parameter_value onchip_cpu1 {slave1Latency} {1}
    set_instance_parameter_value onchip_cpu1 {slave2Latency} {1}
    set_instance_parameter_value onchip_cpu1 {useNonDefaultInitFile} {0}
    set_instance_parameter_value onchip_cpu1 {useShallowMemBlocks} {0}
    set_instance_parameter_value onchip_cpu1 {writable} {1}
    set_instance_parameter_value onchip_cpu1 {ecc_enabled} {0}

    add_instance shared_mem altera_avalon_onchip_memory2 13.1
    set_instance_parameter_value shared_mem {allowInSystemMemoryContentEditor} {0}
    set_instance_parameter_value shared_mem {blockType} {AUTO}
    set_instance_parameter_value shared_mem {dataWidth} {32}
    set_instance_parameter_value shared_mem {dualPort} {0}
    set_instance_parameter_value shared_mem {initMemContent} {1}
    set_instance_parameter_value shared_mem {initializationFileName} {onchip_mem.hex}
    set_instance_parameter_value shared_mem {instanceID} {NONE}
    set_instance_parameter_value shared_mem {memorySize} {40000.0}
    set_instance_parameter_value shared_mem {readDuringWriteMode} {DONT_CARE}
    set_instance_parameter_value shared_mem {simAllowMRAMContentsFile} {0}
    set_instance_parameter_value shared_mem {simMemInitOnlyFilename} {0}
    set_instance_parameter_value shared_mem {singleClockOperation} {0}
    set_instance_parameter_value shared_mem {slave1Latency} {1}
    set_instance_parameter_value shared_mem {slave2Latency} {1}
    set_instance_parameter_value shared_mem {useNonDefaultInitFile} {0}
    set_instance_parameter_value shared_mem {useShallowMemBlocks} {0}
    set_instance_parameter_value shared_mem {writable} {1}
    set_instance_parameter_value shared_mem {ecc_enabled} {0}

    # connections and connection parameters
    add_connection cpu1.instruction_master cpu1.jtag_debug_module avalon
    set_connection_parameter_value cpu1.instruction_master/cpu1.jtag_debug_module arbitrationPriority {1}
    set_connection_parameter_value cpu1.instruction_master/cpu1.jtag_debug_module baseAddress {0x0800}
    set_connection_parameter_value cpu1.instruction_master/cpu1.jtag_debug_module defaultConnection {0}

    add_connection cpu1.data_master cpu1.jtag_debug_module avalon
    set_connection_parameter_value cpu1.data_master/cpu1.jtag_debug_module arbitrationPriority {1}
    set_connection_parameter_value cpu1.data_master/cpu1.jtag_debug_module baseAddress {0x0800}
    set_connection_parameter_value cpu1.data_master/cpu1.jtag_debug_module defaultConnection {0}

    add_connection cpu0.instruction_master cpu0.jtag_debug_module avalon
    set_connection_parameter_value cpu0.instruction_master/cpu0.jtag_debug_module arbitrationPriority {1}
    set_connection_parameter_value cpu0.instruction_master/cpu0.jtag_debug_module baseAddress {0x0800}
    set_connection_parameter_value cpu0.instruction_master/cpu0.jtag_debug_module defaultConnection {0}

    add_connection cpu0.data_master cpu0.jtag_debug_module avalon
    set_connection_parameter_value cpu0.data_master/cpu0.jtag_debug_module arbitrationPriority {1}
    set_connection_parameter_value cpu0.data_master/cpu0.jtag_debug_module baseAddress {0x0800}
    set_connection_parameter_value cpu0.data_master/cpu0.jtag_debug_module defaultConnection {0}

    add_connection clk_0.clk timer0.clk clock

    add_connection clk_0.clk cpu0.clk clock

    add_connection clk_0.clk cpu1.clk clock

    add_connection clk_0.clk_reset cpu1.reset_n reset

    add_connection clk_0.clk_reset cpu0.reset_n reset

    add_connection clk_0.clk_reset timer0.reset reset

    add_connection cpu0.jtag_debug_module_reset timer0.reset reset

    add_connection cpu0.data_master timer0.s1 avalon
    set_connection_parameter_value cpu0.data_master/timer0.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu0.data_master/timer0.s1 baseAddress {0x1000}
    set_connection_parameter_value cpu0.data_master/timer0.s1 defaultConnection {0}

    add_connection clk_0.clk timer1.clk clock

    add_connection clk_0.clk_reset timer1.reset reset

    add_connection cpu1.jtag_debug_module_reset timer1.reset reset

    add_connection cpu1.data_master timer1.s1 avalon
    set_connection_parameter_value cpu1.data_master/timer1.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu1.data_master/timer1.s1 baseAddress {0x1000}
    set_connection_parameter_value cpu1.data_master/timer1.s1 defaultConnection {0}

    add_connection clk_0.clk sysid0.clk clock

    add_connection clk_0.clk_reset sysid0.reset reset

    add_connection cpu0.data_master sysid0.control_slave avalon
    set_connection_parameter_value cpu0.data_master/sysid0.control_slave arbitrationPriority {1}
    set_connection_parameter_value cpu0.data_master/sysid0.control_slave baseAddress {0x1028}
    set_connection_parameter_value cpu0.data_master/sysid0.control_slave defaultConnection {0}

    add_connection clk_0.clk sysid1.clk clock

    add_connection clk_0.clk_reset sysid1.reset reset

    add_connection cpu1.jtag_debug_module_reset sysid1.reset reset

    add_connection cpu0.jtag_debug_module_reset sysid0.reset reset

    add_connection clk_0.clk jtag0.clk clock

    add_connection cpu0.jtag_debug_module_reset jtag0.reset reset

    add_connection clk_0.clk_reset jtag0.reset reset

    add_connection cpu0.data_master jtag0.avalon_jtag_slave avalon
    set_connection_parameter_value cpu0.data_master/jtag0.avalon_jtag_slave arbitrationPriority {1}
    set_connection_parameter_value cpu0.data_master/jtag0.avalon_jtag_slave baseAddress {0x1020}
    set_connection_parameter_value cpu0.data_master/jtag0.avalon_jtag_slave defaultConnection {0}

    add_connection clk_0.clk jtag1.clk clock

    add_connection clk_0.clk_reset jtag1.reset reset

    add_connection cpu1.jtag_debug_module_reset jtag1.reset reset

    add_connection cpu1.data_master jtag1.avalon_jtag_slave avalon
    set_connection_parameter_value cpu1.data_master/jtag1.avalon_jtag_slave arbitrationPriority {1}
    set_connection_parameter_value cpu1.data_master/jtag1.avalon_jtag_slave baseAddress {0x1028}
    set_connection_parameter_value cpu1.data_master/jtag1.avalon_jtag_slave defaultConnection {0}

    add_connection clk_0.clk onchip_cpu0.clk1 clock

    add_connection clk_0.clk_reset onchip_cpu0.reset1 reset

    add_connection cpu0.instruction_master onchip_cpu0.s1 avalon
    set_connection_parameter_value cpu0.instruction_master/onchip_cpu0.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu0.instruction_master/onchip_cpu0.s1 baseAddress {0x10000000}
    set_connection_parameter_value cpu0.instruction_master/onchip_cpu0.s1 defaultConnection {0}

    add_connection clk_0.clk onchip_cpu1.clk1 clock

    add_connection clk_0.clk_reset onchip_cpu1.reset1 reset

    add_connection cpu1.jtag_debug_module_reset onchip_cpu1.reset1 reset

    add_connection cpu0.jtag_debug_module_reset onchip_cpu0.reset1 reset

    add_connection cpu1.instruction_master onchip_cpu1.s1 avalon
    set_connection_parameter_value cpu1.instruction_master/onchip_cpu1.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu1.instruction_master/onchip_cpu1.s1 baseAddress {0x20000000}
    set_connection_parameter_value cpu1.instruction_master/onchip_cpu1.s1 defaultConnection {0}

    add_connection clk_0.clk shared_mem.clk1 clock

    add_connection clk_0.clk_reset shared_mem.reset1 reset

    add_connection cpu1.data_master shared_mem.s1 avalon
    set_connection_parameter_value cpu1.data_master/shared_mem.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu1.data_master/shared_mem.s1 baseAddress {0x01000000}
    set_connection_parameter_value cpu1.data_master/shared_mem.s1 defaultConnection {0}

    add_connection cpu0.data_master shared_mem.s1 avalon
    set_connection_parameter_value cpu0.data_master/shared_mem.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu0.data_master/shared_mem.s1 baseAddress {0x01000000}
    set_connection_parameter_value cpu0.data_master/shared_mem.s1 defaultConnection {0}

    add_connection cpu1.data_master sysid1.control_slave avalon
    set_connection_parameter_value cpu1.data_master/sysid1.control_slave arbitrationPriority {1}
    set_connection_parameter_value cpu1.data_master/sysid1.control_slave baseAddress {0x1020}
    set_connection_parameter_value cpu1.data_master/sysid1.control_slave defaultConnection {0}

    add_connection cpu0.d_irq timer0.irq interrupt
    set_connection_parameter_value cpu0.d_irq/timer0.irq irqNumber {1}

    add_connection cpu1.d_irq timer1.irq interrupt
    set_connection_parameter_value cpu1.d_irq/timer1.irq irqNumber {1}

    add_connection cpu0.d_irq jtag0.irq interrupt
    set_connection_parameter_value cpu0.d_irq/jtag0.irq irqNumber {16}

    add_connection cpu1.d_irq jtag1.irq interrupt
    set_connection_parameter_value cpu1.d_irq/jtag1.irq irqNumber {16}

    add_connection cpu0.data_master onchip_cpu0.s1 avalon
    set_connection_parameter_value cpu0.data_master/onchip_cpu0.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu0.data_master/onchip_cpu0.s1 baseAddress {0x10000000}
    set_connection_parameter_value cpu0.data_master/onchip_cpu0.s1 defaultConnection {0}

    add_connection cpu1.data_master onchip_cpu1.s1 avalon
    set_connection_parameter_value cpu1.data_master/onchip_cpu1.s1 arbitrationPriority {1}
    set_connection_parameter_value cpu1.data_master/onchip_cpu1.s1 baseAddress {0x20000000}
    set_connection_parameter_value cpu1.data_master/onchip_cpu1.s1 defaultConnection {0}

    # exported interfaces
    add_interface clk clock sink
    set_interface_property clk EXPORT_OF clk_0.clk_in
    add_interface reset reset sink
    set_interface_property reset EXPORT_OF clk_0.clk_in_reset
}
