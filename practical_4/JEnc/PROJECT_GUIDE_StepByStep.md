# Practical 4: JPEG MPSoC Design — Step-by-Step Guide

This guide breaks down the project for someone with **basic Nios II and Quartus** experience. Work through it in order.

---

## Target Platform

| Property | Value | Notes |
|----------|-------|-------|
| **Board** | Terasic DE2-115 | |
| **FPGA** | Altera Cyclone IV E — **EP4CE115F29C7** | |
| **Logic Elements** | 114,480 LEs | Each Nios II/e ≈ 600–700 LEs; Nios II/f ≈ 1,800–3,500 LEs. Six Nios II/e cores (Web/Lite) ≈ 4,200 LEs; mixed /e+/f (paid) ≈ 8,800 LEs — both fit comfortably. |
| **Embedded Memory** | 3,888 Kbit (486 KB) of M9K blocks | Used for on-chip RAM, caches, and FIFOs. Each M9K block = 9,216 bits (1,152 bytes) |
| **DSP Blocks** | 266 (18×18 multipliers) | Useful if you add custom instructions for DCT butterfly multiply-accumulate |
| **External SDRAM** | 2 × 64 MB = **128 MB** (IS42S16320D, 32-bit bus at 100 MHz) | Main storage for the raw input image and the output JPEG buffer |
| **External SRAM** | 2 MB (IS61WV102416, 16-bit bus) | Fast, zero-wait-state; good for performance-critical scratch memory |
| **Flash** | 8 MB | Not needed for this design (used for non-volatile storage) |

---

## What You Are Building (Overview)

You will build a **6-stage pipelined multiprocessor system (MPSoC)** that encodes images to JPEG:

| Stage | Task | Reference code folder | Hot data type |
|-------|------|------------------------|---------------|
| **1** | Read macro-block from raw image, convert RGB → YCbCr | `jencoder1a` | UINT8 in, INT16 out |
| **2** | Level shifting (subtract 128 from each sample) | `jencoder1b` | INT16 in/out |
| **3** | 2D DCT (8×8 vertical + horizontal transform) | `jencoder1c` | INT16 in/out |
| **4** | Quantization (divide DCT coefficients by quant table) | `jencoder1d` | INT16 in/out |
| **5** | Huffman encoding (entropy coding + JPEG markers) | `jencoder1e` | INT16 in, UINT8 out |
| **6** | Write encoded byte stream to JPEG file | `jencoder1f` | UINT8 in |

**Data flow:** Raw image → Stage 1 → **FIFO** → Stage 2 → **FIFO** → … → **FIFO** → Stage 6 → JPEG file.

Each **macro-block** (MCU) contains **3 blocks** (Y, Cb, Cr), each **8×8 = 64 samples**. So one MCU produces **192 INT16 values** (384 bytes) flowing through the pipeline.

The reference design uses **Tensilica Xtensa** CPUs with custom **TIE port queues** (e.g. `SEND6()`, `RECV1()`). Your job is to implement the **same algorithm** on the FPGA using **Nios II** processors and **Altera On-Chip FIFO Memory** IP blocks between stages.

---

## Data Types Used Throughout

The reference code defines these in `datatype.h` — keep the same types on Nios II (they map to standard C types on a 32-bit processor):

| Typedef | C type | Size | Where used |
|---------|--------|------|------------|
| `INT8` | `char` | 1 byte | Filename characters (Stage 1→6) |
| `UINT8` | `unsigned char` | 1 byte | Raw pixel data, Huffman output bytes, quantization tables |
| `INT16` | `short` | 2 bytes | YCbCr samples, level-shifted values, DCT coefficients, quantized coefficients |
| `UINT16` | `unsigned short` | 2 bytes | Inverse quantization tables (ILqt, ICqt) |
| `INT32` | `int` | 4 bytes | Control parameters (MCU count, quality factor, image dimensions) |
| `UINT32` | `unsigned int` | 4 bytes | Control parameters |

**Key constant:** `BLOCK_SIZE = 64` (an 8×8 block). Arrays like `Y1[64]`, `CB[64]`, `CR[64]`, `Temp[64]`, `Lqt[64]`, `Cqt[64]` are all 64 elements.

---

## Part 0: Prerequisites and Reading

### 0.1 Tools
- **Quartus Prime** (with Cyclone IV E device support for EP4CE115F29C7)
- **Nios II Software Build Tools (SBT)** / Nios II EDS
- DE2-115 board with USB-Blaster cable

### 0.2 Concepts to be comfortable with
- Creating a **Qsys/Platform Designer** system with at least one Nios II
- Adding **Avalon-MM** components and connecting them
- Compiling the hardware and generating the BSP/application in Nios II SBT
- Running software on the Nios II (e.g. from host via JTAG UART or from on-chip memory)

### 0.3 Reading (before coding)
1. **Practical 4 PDF** — stages, pipeline, FIFOs, Part 2 improvement ideas.
2. **Onchip FIFO Memory Datasheet** (Altera) — understand depth, width, status registers, and single-clock vs dual-clock modes.
3. **Research paper** ("Synthesis of heterogeneous pipelined multiprocessor systems using ILP: JPEG case study"):
   - Section 3: system model, 6 processes, FIFO connectivity (Figure 1(b)).
   - Section 3.2: processor configurations (caches, custom instructions).
   - Section 7–8: how they measure runtime and optimize (for Part 2 ideas).

### 0.4 Quartus Edition and Nios II Core Availability

**If you use the free Quartus Web Edition or Quartus Prime Lite Edition:** only the **Nios II/e** (economy) core is included in the free license. **Nios II/f** (fast) and **Nios II/s** (standard) require a paid Quartus Prime Standard or Pro license.

- **Practical impact:** Use **Nios II/e for all 6 pipeline stages** when using the free edition. The design will work; stages 3 (DCT) and 5 (Huffman) will run slower because they use software multiply and no caches, but the algorithm is unchanged.
- **If you have a paid license:** You can use the optional configuration in this guide that puts Nios II/f (with caches and hardware multiply) on stages 3 and 5 for better performance.

---

## Part 1: Pipelined JPEG MPSoC (Main Deliverable)

Part 1 is split into **hardware** and **software**. Do hardware first so you have addresses for the FIFOs; then port the C code to use those FIFOs.

---

### Step 1: Map the Reference Pipeline to Nios II + FIFOs

The reference `jencoder1.c` defines the exact FIFO connectivity with `ADDFIFO()` calls. Here is the complete FIFO map with types, depths, and justifications:

#### 1.1 Complete FIFO Specification Table

| FIFO name | Producer | Consumer | Data width | Depth (words) | Bytes | Purpose & Justification |
|-----------|----------|----------|------------|---------------|-------|------------------------|
| **fifo_1to2_Y** | Stage 1 (OUT1) | Stage 2 (IN1) | 32-bit | 64 | 256 B | Carries one full 8×8 Y (luminance) block. Depth = 64 so Stage 1 can push an entire block without stalling, even if Stage 2 hasn't started consuming yet. Each INT16 sample is zero/sign-extended to 32 bits for the write. |
| **fifo_1to2_Cb** | Stage 1 (OUT2) | Stage 2 (IN2) | 32-bit | 64 | 256 B | Same as above for Cb (chrominance blue). Separate FIFO because Stage 2 reads Y, Cb, Cr in sequence from different ports. |
| **fifo_1to2_Cr** | Stage 1 (OUT3) | Stage 2 (IN3) | 32-bit | 64 | 256 B | Same for Cr (chrominance red). Three FIFOs mirror the reference's three output ports (SEND1, SEND2, SEND3). |
| **fifo_2to3** | Stage 2 (OUT) | Stage 3 (IN) | 32-bit | 64 | 256 B | Level-shifted samples. Stage 2 outputs 3×64 = 192 values per MCU, but each block of 64 is processed sequentially. Depth of 64 allows one full block to be buffered. |
| **fifo_3to4** | Stage 3 (OUT) | Stage 4 (IN2) | 32-bit | 64 | 256 B | DCT coefficients. Same reasoning: one 8×8 block at a time. |
| **fifo_4to5** | Stage 4 (OUT) | Stage 5 (IN2) | 32-bit | 64 | 256 B | Quantized coefficients. One block buffered. |
| **fifo_5to6_data** | Stage 5 (OUT1) | Stage 6 (IN2) | 32-bit | 64 | 256 B | Huffman-encoded byte stream. Each UINT8 byte is zero-extended to 32 bits for the write. 64 words is more than enough to buffer one block's worth of encoded data (compressed, so always ≤ 64 bytes per block). |
| **fifo_5to6_done** | Stage 5 (OUT2) | Stage 6 (IN3) | 32-bit | 1 | 4 B | Single "done" token. Stage 5 writes a `1` (as a 32-bit word) when all MCUs are encoded. Stage 6 polls this to know when to close the file. Depth = 1 is sufficient (only one value ever sent). |
| **fifo_1to4_param** | Stage 1 (OUT4) | Stage 4 (IN1) | 32-bit | 2 | 8 B | Sends `num_mcus` (UINT32) and `quality_factor` (UINT32) — exactly 2 words. Depth = 2 holds both values. |
| **fifo_1to5_param** | Stage 1 (OUT5) | Stage 5 (IN1) | 32-bit | 3 | 12 B | Sends `image_width`, `image_height`, and `num_mcus` — exactly 3 UINT32 values. Depth = 3 holds all three. |
| **fifo_1to6_fname** | Stage 1 (OUT6) | Stage 6 (IN1) | 32-bit | 100 | 400 B | Output filename as a null-terminated string (up to 100 characters). Each INT8 character is zero-extended to 32 bits. Depth = 100 so the entire filename can be pushed without blocking. |

**Total FIFO count: 11 FIFOs.**

**Total embedded memory for FIFOs:** ≈ 2,204 bytes of data storage (all FIFOs are 32-bit wide). On the EP4CE115, each M9K block holds 1,152 bytes, so some FIFOs (depth 64 × 4 bytes = 256 B) fit in 1 M9K block each. With 11 FIFOs, you use ≈ 11 M9K blocks out of 432 available — only **2.5%** of the embedded memory budget. Using a uniform 32-bit width simplifies the Qsys design and FIFO driver at the cost of slightly more memory per FIFO for data that is logically 8-bit or 16-bit.

#### 1.2 Why Separate FIFOs (Not Shared Memory) for Data

- **Decoupling:** FIFOs provide natural back-pressure (full/empty flags). A producer blocks when the FIFO is full; a consumer blocks when empty. No explicit synchronization code needed.
- **Determinism:** Each FIFO is single-writer, single-reader (no bus arbitration contention on the data path).
- **Simplicity:** The reference design uses dedicated TIE port queues — one FIFO per port maps directly to the reference code, making porting straightforward.

#### 1.3 Why Shared Memory for Parameters (Alternative)

Instead of `fifo_1to4_param`, `fifo_1to5_param`, and `fifo_1to6_fname`, you may use a small **shared on-chip RAM** (e.g. 256 bytes) where Stage 1 writes parameters and Stages 4/5/6 read them. Reasons:

- **Fewer components:** Eliminates 3 FIFOs from the Qsys design.
- **Simpler reads:** Parameters are read only once during initialization; shared memory avoids the FIFO push/pop protocol for one-shot data.
- **Trade-off:** Requires a synchronization flag (e.g. a PIO register) so downstream stages know when Stage 1 has finished writing parameters. This adds a tiny bit of complexity.

**Recommendation:** Start with all 11 FIFOs (matches reference exactly). Switch to shared memory for parameters later if you want to simplify the Qsys design.

---

### Step 2: Create the Hardware in Qsys (Platform Designer)

Do this in small increments so you can compile and test.

#### 2.1 Single Nios II + one FIFO (sanity check)

1. Create a new Qsys system; add these components:

| Component | IP Name in Qsys | Configuration | Reason |
|-----------|-----------------|---------------|--------|
| **Nios II Processor** | `altera_nios2_gen2` | Select **Nios II/e** (economy). No caches, no multiplier. | Nios II/e uses ~600 LEs and is the only core included in Quartus Web/Lite Edition. Use /e for all stages in the free edition; with a paid Quartus license you can use Nios II/f on stages 3 and 5 for better performance. |
| **On-Chip RAM** | `altera_avalon_onchip_memory2` | **32 KB**, 32-bit data width, type = RAM | 32 KB holds the Nios II firmware for one stage (~8–16 KB code + stack + heap). 32-bit width matches the Nios II data bus. Each KB uses ~1 M9K block, so 32 KB uses ~28 M9K blocks. |
| **JTAG UART** | `altera_avalon_jtag_uart` | Default (write depth = 64, read depth = 64) | Required for `printf` debug output. Low resource cost (~200 LEs). |
| **System ID** | `altera_avalon_sysid_qsys` | Default | Ensures BSP matches hardware. Costs ~20 LEs. |
| **On-Chip FIFO Memory** | `altera_avalon_fifo` | Depth = **64**, Width = **32 bits**, Single-clock mode, Avalon-MM write slave + Avalon-MM read slave | Matches `fifo_1to2_Y` spec. Single-clock mode because all components share one clock domain (50 MHz from DE2-115 CLOCK_50). Avalon-MM on both sides so both Nios II cores access it via memory-mapped registers. All FIFOs use a uniform 32-bit width to match the Nios II 32-bit data bus and simplify the driver. |
| **Interval Timer** | `altera_avalon_timer` | Period = 1 ms, counter size = 32-bit | For measuring per-stage latency (cycles/µs). Optional for the first test, but add it now to avoid re-generating later. |

2. **Connect:**
   - Nios II **data_master** → On-Chip RAM slave, JTAG UART slave, System ID slave, FIFO write slave (or read slave depending on which side you test).
   - Nios II **instruction_master** → On-Chip RAM slave.
   - All components **clk** → system clock (50 MHz).
   - All components **reset** → system reset.

3. **Assign base addresses** (let Qsys auto-assign or pick your own). Record them. Set the Nios II **reset vector** and **exception vector** to point into the On-Chip RAM.

4. **Generate HDL**, add the Qsys system to your Quartus project, connect `CLOCK_50` and the reset button to the system's clock and reset. **Compile** and verify it fits on the FPGA.

#### 2.2 FIFO Register Map (How the Nios II Sees the FIFO)

The **Altera On-Chip FIFO Memory** (`altera_avalon_fifo`) exposes these Avalon-MM slave registers:

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| `+0x00` | **Data** | Write (producer) / Read (consumer) | Write = push one word into FIFO. Read = pop one word from FIFO. All FIFOs are configured with 32-bit width to match the Nios II data bus. Values that are logically 8-bit or 16-bit are zero/sign-extended to 32 bits on write and masked/cast on read. |
| `+0x04` | **Status** | Read-only | Bit 0: `empty` (1 = FIFO has no data). Bit 1: `full` (1 = FIFO cannot accept more data). Other bits: almost-empty, almost-full, overflow, underflow. |
| `+0x08` | **Event** | Read/Write-1-to-clear | Sticky event flags (overflow, underflow, etc.). Write a 1 to a bit to clear it. |
| `+0x0C` | **IE (Interrupt Enable)** | Read/Write | Enable interrupts for specific events. Not needed if you poll (spin-wait). |
| `+0x10` | **Almost-Full Threshold** | Read/Write | Number of words at which the "almost full" flag asserts. Only relevant if you use DMA or interrupts. |
| `+0x14` | **Almost-Empty Threshold** | Read/Write | Number of words at which "almost empty" asserts. |
| `+0x18` | **Fill Level** | Read-only | Current number of words stored in the FIFO (0 to depth). Useful for debugging. |

**Important:** The FIFO exposes **two separate Avalon-MM slaves** — one for the **write side** (producer) and one for the **read side** (consumer). In Qsys you connect:
- The **producer CPU's data_master** → FIFO **in** (write slave) — this CPU writes to offset `+0x00` to push data, and reads status from its own status register.
- The **consumer CPU's data_master** → FIFO **out** (read slave) — this CPU reads from offset `+0x00` to pop data.

Each side has its own base address (e.g. `FIFO_1TO2_Y_IN_BASE` for the write side, `FIFO_1TO2_Y_OUT_BASE` for the read side). The CSR (control/status) port can be connected to either side or both.

#### 2.3 Scale to 6 Nios II + All FIFOs

Now build the full system. Here is the complete component list:

##### 2.3.1 Processors (×6)

**Quartus Web / Lite Edition (free):** Use **Nios II/e for all 6 cores** — the /e core is the only one included in the free license. The pipeline works; stages 3 (DCT) and 5 (Huffman) will be slower due to software multiply and no caches.

**Quartus Standard or Pro (paid):** You may use Nios II/f on stages 3 and 5 for better performance (see optional row notes below).

| Instance name | Variant | Instruction cache | Data cache | Multiplier | Reason |
|---------------|---------|-------------------|------------|------------|--------|
| `cpu_stage1` | **Nios II/e** | None | None | None | Stage 1 (RGB→YCbCr) is mostly memory reads and simple arithmetic. Economy core saves area. |
| `cpu_stage2` | **Nios II/e** | None | None | None | Stage 2 (level shift) is trivial: subtract 128 from each sample. Nios II/e is sufficient. |
| `cpu_stage3` | **Nios II/e** (free) or **Nios II/f** (paid) | None (e) or 4 KB (f) | None (e) or 2 KB (f) | None (e) or HW multiply (f) | Stage 3 (DCT) is the most compute-intensive — 8×8 matrix multiplications. With /e, multiplies are done in software (~32 cycles each). With /f (paid), hardware multiply and caches speed it up. **Bottleneck stage.** |
| `cpu_stage4` | **Nios II/e** | None | None | None | Stage 4 (quantization) is a simple divide/multiply loop over 64 elements. Economy core is fine. |
| `cpu_stage5` | **Nios II/e** (free) or **Nios II/f** (paid) | None (e) or 4 KB (f) | None (e) or 2 KB (f) | None (e) or HW multiply (f) | Stage 5 (Huffman) does bit manipulation and table lookups. /e works; /f (paid) improves table lookup and loop speed. |
| `cpu_stage6` | **Nios II/e** | None | None | None | Stage 6 (file write) just receives bytes and writes them out. Minimal computation. |

**Area estimate (all Nios II/e — Web/Lite Edition):**
- 6 × Nios II/e ≈ 6 × 700 = **4,200 LEs** — about **3.7%** of 114,480 LEs. No cache blocks.

**Area estimate (optional: 2× Nios II/f + 4× Nios II/e — paid Quartus only):**
- 4 × Nios II/e ≈ 2,800 LEs; 2 × Nios II/f ≈ 6,000 LEs → **Total ≈ 8,800 LEs**. Cache memory: 2 × (4 KB I$ + 2 KB D$) ≈ 12 M9K blocks.

##### 2.3.2 Per-CPU Private On-Chip RAM

Each Nios II needs its own instruction and data memory. Sizes depend on the stage's code complexity:

| Instance | Size | M9K blocks used | Reason |
|----------|------|-----------------|--------|
| `ram_stage1` | **64 KB** | ~56 | Stage 1 has the largest code: image reading, RGB→YCbCr conversion tables, and parameter distribution. It also allocates a buffer for input pixel data. 64 KB accommodates code (~12 KB) + stack (4 KB) + image buffer remainder. **Note:** If the raw image is large (e.g. 640×480 = 921 KB), store it in external SDRAM and give Stage 1 an SDRAM connection. |
| `ram_stage2` | **16 KB** | ~14 | Stage 2 code is very small (~2 KB). 16 KB provides comfortable headroom for stack and any local arrays. |
| `ram_stage3` | **32 KB** | ~28 | Stage 3 (DCT) uses several 64-element INT16 arrays for intermediate results, plus the DCT coefficient table (64 × INT16 = 128 bytes). Code is ~8 KB. |
| `ram_stage4` | **32 KB** | ~28 | Stage 4 has quantization tables (`Lqt[64]`, `Cqt[64]`, `ILqt[64]`, `ICqt[64]`) and code for building tables from quality factor. |
| `ram_stage5` | **48 KB** | ~42 | Stage 5 (Huffman) has the largest data footprint: Huffman tables (DC/AC for luminance and chrominance — 4 tables of 256 entries each = 4 KB), marker data arrays, bit-packing buffers. Code is ~10 KB. |
| `ram_stage6` | **16 KB** | ~14 | Stage 6 code is simple. 16 KB is more than enough for the write buffer and stack. |

**Total on-chip RAM ≈ 208 KB → ~182 M9K blocks.** The EP4CE115 has 432 M9K blocks (486 KB total embedded memory), so this uses **42%** of embedded memory. This is feasible.

**Alternative — Use External SDRAM:** If on-chip RAM is tight, move some or all instruction/data memory to the external 128 MB SDRAM via an SDRAM controller (`altera_avalon_new_sdram_controller`). Trade-off: SDRAM has higher latency (~10 ns access, vs ~1 cycle for on-chip RAM), but with instruction caches on the /f cores, this is manageable. You can also split: put code in SDRAM (cached) and use small on-chip RAM for stack/heap (fast access).

##### 2.3.3 FIFOs (×11)

Use the **Altera On-Chip FIFO Memory** (`altera_avalon_fifo`) for all FIFOs. Configuration for each:

| Instance | Width | Depth | Clock mode | Interface | M9K blocks |
|----------|-------|-------|------------|-----------|------------|
| `fifo_1to2_Y` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_1to2_Cb` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_1to2_Cr` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_2to3` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_3to4` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_4to5` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_5to6_data` | 32-bit | 64 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_5to6_done` | 32-bit | 1 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_1to4_param` | 32-bit | 2 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_1to5_param` | 32-bit | 3 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |
| `fifo_1to6_fname` | 32-bit | 100 | Single-clock | Avalon-MM ↔ Avalon-MM | 1 |

**Total FIFOs: 11 M9K blocks** (2.5% of available embedded memory).

**Why single-clock mode:** All 6 Nios II cores and all FIFOs run from the same 50 MHz clock (`CLOCK_50` on DE2-115). Single-clock FIFOs (SCFIFO) are simpler, use fewer resources, and have no clock-domain-crossing latency. Use dual-clock (DCFIFO) only if you later add components in a different clock domain (e.g. an SDRAM controller at 100 MHz).

**Why Avalon-MM on both sides:** Both the producer and consumer are Nios II processors with Avalon-MM data masters. Using Avalon-MM slaves on both FIFO ports allows direct memory-mapped access from C code (read/write to a base address). No DMA or streaming interface needed.

**Why uniform 32-bit width:** The Nios II data bus is 32 bits wide. Using 32-bit FIFOs for all channels — even those carrying logically 8-bit or 16-bit data — simplifies the hardware design (one uniform FIFO configuration in Qsys) and the software driver (a single `fifo_write_32`/`fifo_read_32` pair handles all FIFOs). The overhead is small: a 16-bit value occupies 4 bytes instead of 2 in the FIFO, but each FIFO still fits in a single M9K block. The caller simply casts to/from the appropriate C type (`short`, `unsigned char`, etc.) when writing/reading.

**Why these depths:** Each data FIFO depth = 64 matches one 8×8 block. This allows a producer to write an entire block before the consumer starts reading, which provides **one block of decoupling** between stages. For better throughput (double-buffering), you can increase depths to 128 (two blocks) so the producer can start the next block while the consumer processes the current one. Start with 64 and increase in Part 2 if profiling shows stalls.

##### 2.3.4 Shared Components

| Component | IP Name | Configuration | Connected to | Reason |
|-----------|---------|---------------|-------------|--------|
| **JTAG UART** (×1 or ×6) | `altera_avalon_jtag_uart` | Default (64-word write FIFO, 64-word read FIFO) | All 6 CPUs (shared) or one per CPU | For `printf` debugging. One shared JTAG UART saves area but interleaves output from all CPUs. Six separate ones give clean per-stage output but cost ~1,200 LEs. **Recommendation:** Start with 1 shared JTAG UART; add per-CPU UARTs if debugging is difficult. |
| **System ID** | `altera_avalon_sysid_qsys` | Default | All 6 CPUs | Ensures each BSP matches the hardware. One instance is enough (all CPUs can read it). |
| **Interval Timer** (×6) | `altera_avalon_timer` | 32-bit counter, 1 µs period | One per CPU | Each stage needs its own timer to measure per-block latency independently. Cost: ~200 LEs each, ~1,200 LEs total. |
| **SDRAM Controller** (optional) | `altera_avalon_new_sdram_controller` | 128 MB, 32-bit, CAS latency = 3 | Stage 1 data_master (for reading raw image), potentially all CPUs for instruction memory | Gives access to the board's 128 MB SDRAM. Stage 1 needs this if the raw image doesn't fit in on-chip RAM (images > 64 KB). Other CPUs need it only if you move instruction memory to SDRAM. |
| **PLL** (optional) | `altpll` | Input: 50 MHz (CLOCK_50), Output: 100 MHz for SDRAM, 50 MHz for system | Clock input to SDRAM controller and system | SDRAM on DE2-115 runs at 100 MHz. If you add an SDRAM controller, you need a PLL to generate the 100 MHz SDRAM clock from the 50 MHz board clock. The PLL also provides a clean, phase-aligned clock for the system. |

##### 2.3.5 Connectivity Matrix

Each CPU's `data_master` connects to specific FIFOs and memory. Here is the full wiring:

| CPU | Writes to (FIFO write-slave) | Reads from (FIFO read-slave) | Memory | Other |
|-----|------------------------------|------------------------------|--------|-------|
| `cpu_stage1` | `fifo_1to2_Y` (in), `fifo_1to2_Cb` (in), `fifo_1to2_Cr` (in), `fifo_1to4_param` (in), `fifo_1to5_param` (in), `fifo_1to6_fname` (in) | — | `ram_stage1`, SDRAM (for raw image) | JTAG UART, System ID, Timer 1 |
| `cpu_stage2` | `fifo_2to3` (in) | `fifo_1to2_Y` (out), `fifo_1to2_Cb` (out), `fifo_1to2_Cr` (out) | `ram_stage2` | JTAG UART, Timer 2 |
| `cpu_stage3` | `fifo_3to4` (in) | `fifo_2to3` (out) | `ram_stage3` | JTAG UART, Timer 3 |
| `cpu_stage4` | `fifo_4to5` (in) | `fifo_3to4` (out), `fifo_1to4_param` (out) | `ram_stage4` | JTAG UART, Timer 4 |
| `cpu_stage5` | `fifo_5to6_data` (in), `fifo_5to6_done` (in) | `fifo_4to5` (out), `fifo_1to5_param` (out) | `ram_stage5` | JTAG UART, Timer 5 |
| `cpu_stage6` | — | `fifo_5to6_data` (out), `fifo_5to6_done` (out), `fifo_1to6_fname` (out) | `ram_stage6`, SDRAM (for writing JPEG output) | JTAG UART, Timer 6 |

Each CPU's `instruction_master` connects **only** to its own private RAM (and optionally to SDRAM if you store code there).

**Why restrict connections:** Connecting a master only to the slaves it needs reduces Qsys interconnect complexity and avoids unnecessary arbitration, which saves LEs and improves timing.

##### 2.3.6 Reset and Clock

- **Clock:** All components share one clock domain (50 MHz from `CLOCK_50`). If using SDRAM, add a PLL to generate 100 MHz for the SDRAM clock pin and 50 MHz (phase-aligned) for the rest of the system.
- **Reset:** All Nios II cores and FIFOs share the same system reset signal (active-low from KEY[0] on DE2-115). This ensures the entire pipeline starts in a known state — all FIFOs empty, all CPUs at their reset vectors.
- **Boot order:** All 6 CPUs boot simultaneously from reset. Stage 2–6 will immediately block on their input FIFOs (waiting for data). Stage 1 begins producing data, and the pipeline naturally starts flowing. No explicit boot sequencing needed.

##### 2.3.7 Resource Budget Summary

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| Logic Elements | ~12,000 (CPUs + interconnect + peripherals) | 114,480 | ~10% |
| M9K Memory Blocks | ~200 (RAM + caches + FIFOs) | 432 | ~46% |
| Embedded Memory (KB) | ~230 KB | 486 KB | ~47% |
| DSP Blocks | 0 (all Nios II/e) or 2 (if using Nios II/f on stages 3 & 5) | 266 | <1% |
| User I/O Pins | ~5 (JTAG, clock, reset) | 528 | <1% |

This leaves substantial headroom for Part 2 optimizations (more caches, larger FIFOs, additional cores, custom instructions using DSP blocks).

4. **Generate HDL** from Qsys, add the generated system to your Quartus project, **assign pins** (CLOCK_50, KEY[0] for reset, JTAG pins are automatic), and **compile**. Verify timing is met at 50 MHz.

Keep a **memory map table** in a header file: for each Nios, list every FIFO base address it accesses.

---

### Step 3: Nios II Software — FIFO Driver (Abstraction)

The reference code uses `SEND*()` and `RECV*()`. You will replace these with a small driver that talks to your FIFO hardware.

#### 3.1 Register Definitions

For each FIFO, you need the **base address** (from Qsys, found in `system.h` after BSP generation). The registers are:

```c
/* FIFO register offsets (from Altera On-Chip FIFO Memory datasheet) */
#define FIFO_DATA_REG        0x00  /* Write = push, Read = pop */
#define FIFO_STATUS_REG      0x04  /* Bit 0: empty, Bit 1: full */
#define FIFO_EVENT_REG       0x08  /* Sticky event flags */
#define FIFO_IE_REG          0x0C  /* Interrupt enable */
#define FIFO_ALMOSTFULL_REG  0x10  /* Almost-full threshold */
#define FIFO_ALMOSTEMPTY_REG 0x14  /* Almost-empty threshold */
#define FIFO_FILL_REG        0x18  /* Current fill level (0..depth) */
```

#### 3.2 Blocking Read/Write Functions

```c
#include <io.h>  /* Nios II HAL: IORD_32DIRECT, IOWR_32DIRECT */

/* Status register bit masks */
#define FIFO_EMPTY_MASK  0x01
#define FIFO_FULL_MASK   0x02

/**
 * fifo_write_32 — Push a 32-bit value into a FIFO.
 * Blocks (spins) if the FIFO is full, providing natural back-pressure
 * to the producer. This matches the TIE SEND() semantics from the
 * reference design where a send stalls the processor until the
 * downstream queue has space.
 *
 * All FIFOs use 32-bit width. For data that is logically 8-bit or
 * 16-bit, the caller casts/sign-extends to unsigned int before writing.
 * For example: fifo_write_32(base, (unsigned int)(short)value) for INT16,
 * or fifo_write_32(base, (unsigned int)byte) for UINT8.
 *
 * @param base  Write-side base address of the FIFO (from system.h)
 * @param value 32-bit value to push
 */
static inline void fifo_write_32(unsigned int base, unsigned int value) {
    /* Spin until FIFO is not full */
    while (IORD_32DIRECT(base + FIFO_STATUS_REG, 0) & FIFO_FULL_MASK) { }
    /* Push the value */
    IOWR_32DIRECT(base + FIFO_DATA_REG, 0, value);
}

/**
 * fifo_read_32 — Pop a 32-bit value from a FIFO.
 * Blocks (spins) if the FIFO is empty, providing natural synchronization
 * with the producer. This matches the TIE RECV() semantics.
 *
 * The caller casts the returned 32-bit value to the appropriate type:
 *   - For INT16 data:  (short)(fifo_read_32(base) & 0xFFFF)
 *   - For UINT8 data:  (unsigned char)(fifo_read_32(base) & 0xFF)
 *   - For UINT32 data: use the return value directly
 *
 * @param base  Read-side base address of the FIFO (from system.h)
 * @return      32-bit value popped from the FIFO
 */
static inline unsigned int fifo_read_32(unsigned int base) {
    /* Spin until FIFO is not empty */
    while (IORD_32DIRECT(base + FIFO_STATUS_REG, 0) & FIFO_EMPTY_MASK) { }
    /* Pop and return the value */
    return IORD_32DIRECT(base + FIFO_DATA_REG, 0);
}
```

**Why a single 32-bit driver:** All FIFOs are configured with a uniform 32-bit data width, matching the Nios II 32-bit data bus. This means only one pair of read/write functions is needed. Data that is logically 8-bit (UINT8, INT8) or 16-bit (INT16) is simply cast to/from `unsigned int` by the caller. On write, the value is zero/sign-extended to 32 bits; on read, the caller masks or casts the lower bits (e.g. `& 0xFFFF` for INT16, `& 0xFF` for UINT8). This simplifies both the hardware (one FIFO configuration) and the software (one driver function pair).

**Why `IORD_32DIRECT` / `IOWR_32DIRECT`:** These are Nios II HAL macros that generate uncached, volatile memory-mapped I/O accesses. They compile to single load/store instructions and bypass any data cache, which is critical for hardware registers that can change between reads (like FIFO status).

Put these in a header (e.g. `fifo_driver.h`) and use them everywhere instead of `SEND`/`RECV`.

#### 3.3 FIFO Address Map Header

Create `fifo_map.h` with base addresses from your Qsys design (these are examples — replace with your actual addresses after Qsys generation):

```c
#ifndef FIFO_MAP_H
#define FIFO_MAP_H

/*
 * These addresses come from system.h generated by Qsys.
 * Each FIFO has TWO base addresses:
 *   - _IN_BASE  = write-side slave (producer connects here)
 *   - _OUT_BASE = read-side slave  (consumer connects here)
 *   - _CSR_BASE = control/status registers (fill level, thresholds)
 *
 * ALL FIFOs are configured with 32-bit data width.
 * Use fifo_write_32() / fifo_read_32() for all FIFOs.
 * Cast to/from the logical data type (INT16, UINT8, etc.) in the caller.
 *
 * Example addresses (replace with actual values from your system.h):
 */

/* Stage 1 → Stage 2: Y, Cb, Cr blocks (32-bit, depth 64) */
#define FIFO_1TO2_Y_IN_BASE    0x00081000  /* Stage 1 writes Y here */
#define FIFO_1TO2_Y_OUT_BASE   0x00081020  /* Stage 2 reads Y here */
#define FIFO_1TO2_CB_IN_BASE   0x00082000
#define FIFO_1TO2_CB_OUT_BASE  0x00082020
#define FIFO_1TO2_CR_IN_BASE   0x00083000
#define FIFO_1TO2_CR_OUT_BASE  0x00083020

/* Stage 2 → Stage 3: level-shifted samples (32-bit, depth 64) */
#define FIFO_2TO3_IN_BASE      0x00084000
#define FIFO_2TO3_OUT_BASE     0x00084020

/* Stage 3 → Stage 4: DCT coefficients (32-bit, depth 64) */
#define FIFO_3TO4_IN_BASE      0x00085000
#define FIFO_3TO4_OUT_BASE     0x00085020

/* Stage 4 → Stage 5: quantized coefficients (32-bit, depth 64) */
#define FIFO_4TO5_IN_BASE      0x00086000
#define FIFO_4TO5_OUT_BASE     0x00086020

/* Stage 5 → Stage 6: encoded byte stream (32-bit, depth 64) */
#define FIFO_5TO6_DATA_IN_BASE  0x00087000
#define FIFO_5TO6_DATA_OUT_BASE 0x00087020

/* Stage 5 → Stage 6: done token (32-bit, depth 1) */
#define FIFO_5TO6_DONE_IN_BASE  0x00088000
#define FIFO_5TO6_DONE_OUT_BASE 0x00088020

/* Stage 1 → Stage 4: parameters (32-bit, depth 2) */
#define FIFO_1TO4_PARAM_IN_BASE  0x00089000
#define FIFO_1TO4_PARAM_OUT_BASE 0x00089020

/* Stage 1 → Stage 5: parameters (32-bit, depth 3) */
#define FIFO_1TO5_PARAM_IN_BASE  0x0008A000
#define FIFO_1TO5_PARAM_OUT_BASE 0x0008A020

/* Stage 1 → Stage 6: filename (32-bit, depth 100) */
#define FIFO_1TO6_FNAME_IN_BASE  0x0008B000
#define FIFO_1TO6_FNAME_OUT_BASE 0x0008B020

#endif /* FIFO_MAP_H */
```

Build **6 executables** (one per Nios), all `#include "fifo_map.h"`. Each stage only uses the `_IN_BASE` or `_OUT_BASE` addresses relevant to it.

---

### Step 4: Port the Reference C Code to Nios II (Per Stage)

Create **6 separate Nios II application projects** (one per CPU). Each project's BSP targets the **same hardware .sopcinfo** but selects a different CPU instance (e.g. `cpu_stage1`, `cpu_stage2`, etc.) in the BSP settings.

For each stage, do the following.

#### 4.1 Stage 1 (was `jencoder1a`) — RGB → YCbCr + Parameter Distribution

**Reference files:** `encoder.c`, `readYUV.c`, `main.c`

**What it does:**
1. Reads the raw image from memory (original reference allocates `2,500,000 × sizeof(UINT8)` = 2.5 MB for pixel data).
2. For each MCU: extracts an 8×8 block, converts RGB → YCbCr, producing `Y[64]`, `Cb[64]`, `Cr[64]` (all INT16).
3. Sends each block to Stage 2 via 3 FIFOs (SEND1=Y, SEND2=Cb, SEND3=Cr), 64 values each.
4. Sends control parameters once at startup:
   - `SEND4(num_mcus)` → `fifo_write_32(FIFO_1TO4_PARAM_IN_BASE, num_mcus)`
   - `SEND4(quality)` → `fifo_write_32(FIFO_1TO4_PARAM_IN_BASE, quality)`
   - `SEND5(width)` → `fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, width)`
   - `SEND5(height)` → `fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, height)`
   - `SEND5(num_mcus)` → `fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, num_mcus)`
   - `SEND6(filename_char)` × N → `fifo_write_32(FIFO_1TO6_FNAME_IN_BASE, (unsigned int)ch)` for each character, including `'\0'`

**Port steps:**
- Replace `#include <xtensa/tie/portqueue6o.h>` with `#include "fifo_driver.h"` and `#include "fifo_map.h"`.
- Replace every `SEND1(x)` → `fifo_write_32(FIFO_1TO2_Y_IN_BASE, (unsigned int)(short)x)`
- Replace every `SEND2(x)` → `fifo_write_32(FIFO_1TO2_CB_IN_BASE, (unsigned int)(short)x)`
- Replace every `SEND3(x)` → `fifo_write_32(FIFO_1TO2_CR_IN_BASE, (unsigned int)(short)x)`
- Replace every `SEND4(x)` → `fifo_write_32(FIFO_1TO4_PARAM_IN_BASE, x)`
- Replace every `SEND5(x)` → `fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, x)`
- Replace every `SEND6(x)` → `fifo_write_32(FIFO_1TO6_FNAME_IN_BASE, (unsigned int)(unsigned char)x)`
- Keep the rest of the algorithm (read macro-block, RGB→YCbCr) unchanged. Ensure data types match (`INT16`, `UINT8`, etc.).

**Input image:** For a first test, embed a small raw image (e.g. 16×16 pixels = 768 bytes for RGB) as a C array in Stage 1's code. For larger images, store the raw data in SDRAM (128 MB available) and have Stage 1 read from SDRAM via a pointer.

#### 4.2 Stage 2 (was `jencoder1b`) — Level Shift

**Reference file:** `main.c`

**What it does:** For each of 3 blocks per MCU: reads 64 INT16 values, subtracts 128 from each, writes 64 INT16 values.

**RECV/SEND mapping:**
- `RECV1()` → `(short)(fifo_read_32(FIFO_1TO2_Y_OUT_BASE) & 0xFFFF)` — 64 times (Y block)
- `RECV2()` → `(short)(fifo_read_32(FIFO_1TO2_CB_OUT_BASE) & 0xFFFF)` — 64 times (Cb block)
- `RECV3()` → `(short)(fifo_read_32(FIFO_1TO2_CR_OUT_BASE) & 0xFFFF)` — 64 times (Cr block)
- `SEND(value)` → `fifo_write_32(FIFO_2TO3_IN_BASE, (unsigned int)(short)value)` — 64 times per block, 192 total per MCU

**Data flow per MCU:** 3 × 64 INT16 in → subtract 128 → 3 × 64 INT16 out = 192 values (384 bytes).

#### 4.3 Stage 3 (was `jencoder1c`) — 2D DCT

**Reference file:** `dct.c`

**What it does:** For each of 3 blocks per MCU: reads 64 INT16 values, performs 8×8 2D DCT (first all rows, then all columns, or using a combined approach), writes 64 INT16 DCT coefficients.

**RECV/SEND mapping:**
- `RECV()` → `(short)(fifo_read_32(FIFO_2TO3_OUT_BASE) & 0xFFFF)` — 64 times per block
- `SEND(data[i])` → `fifo_write_32(FIFO_3TO4_IN_BASE, (unsigned int)(short)data[i])` — 64 times per block

**Data flow per MCU:** 3 × 64 INT16 in → DCT → 3 × 64 INT16 out = 192 values (384 bytes).

**Performance note:** DCT involves many multiply-accumulate operations (each 1D 8-point DCT requires ~48 multiplications). With 64 elements processed as 8 rows + 8 columns × 3 blocks per MCU, this totals ~2,304 multiplications per MCU. On Nios II/e (free edition), each multiply takes ~32 cycles in software. With Nios II/f (paid Quartus), hardware multiply reduces this to 1–3 cycles per multiply.

#### 4.4 Stage 4 (was `jencoder1d`) — Quantization

**Reference file:** `quant.c`

**What it does:**
1. **Initialization:** Reads quality factor from parameter FIFO → `fifo_read_32(FIFO_1TO4_PARAM_OUT_BASE)` for `num_mcus` and `quality_factor`. Builds quantization tables `Lqt[64]` (luminance) and `Cqt[64]` (chrominance) from standard JPEG tables scaled by quality.
2. **Per block:** Reads 64 INT16 DCT coefficients, divides each by the corresponding quantization table entry, writes 64 INT16 quantized values.
3. Also sends `Lqt[64]` and `Cqt[64]` (as UINT8) downstream to Stage 5, because the Huffman encoder needs them for the JPEG DQT marker.

**RECV/SEND mapping:**
- `RECV1()` → `fifo_read_32(FIFO_1TO4_PARAM_OUT_BASE)` — 2 values (initialization)
- `RECV2()` → `(short)(fifo_read_32(FIFO_3TO4_OUT_BASE) & 0xFFFF)` — 64 per block
- `SEND(Lqt[i])` → `fifo_write_32(FIFO_4TO5_IN_BASE, (unsigned int)(short)Lqt[i])` — 64 values (init)
- `SEND(Cqt[i])` → `fifo_write_32(FIFO_4TO5_IN_BASE, (unsigned int)(short)Cqt[i])` — 64 values (init)
- `SEND(Temp[i])` → `fifo_write_32(FIFO_4TO5_IN_BASE, (unsigned int)(short)Temp[i])` — 64 per block

#### 4.5 Stage 5 (was `jencoder1e`) — Huffman Encoding

**Reference file:** `huffman.c`

**What it does:**
1. **Initialization:** Reads 3 UINT32 parameters (width, height, count) from `fifo_1to5_param`. Reads `Lqt[64]` and `Cqt[64]` (128 values) from `fifo_4to5`. Writes JPEG header markers (SOI, APP0, DQT, SOF, DHT, SOS) to the output FIFO.
2. **Per block:** Reads 64 INT16 quantized coefficients, Huffman-encodes them (DC differential coding + AC run-length coding), writes variable-length UINT8 bytes to output.
3. **Termination:** After all MCUs, writes EOI marker and sends a done token.

**RECV/SEND mapping:**
- `RECV1()` → `fifo_read_32(FIFO_1TO5_PARAM_OUT_BASE)` — 3 values (init)
- `RECV2()` → `(short)(fifo_read_32(FIFO_4TO5_OUT_BASE) & 0xFFFF)` — 64 per block (plus 128 for tables at init)
- `SEND1(byte)` → `fifo_write_32(FIFO_5TO6_DATA_IN_BASE, (unsigned int)(unsigned char)byte)` — variable count
- `SEND2(1)` → `fifo_write_32(FIFO_5TO6_DONE_IN_BASE, 1)` — 1 value (done token)

**Memory note:** Huffman tables are large: 4 tables (DC luminance, DC chrominance, AC luminance, AC chrominance), each up to 256 entries of 2 bytes = ~2 KB total. Plus marker data arrays (~1 KB). Use `ram_stage5` = 48 KB. With Nios II/e (free edition) there are no caches; with Nios II/f (paid) you can add 4 KB instruction and 2 KB data cache to speed up table lookups.

#### 4.6 Stage 6 (was `jencoder1f`) — File Output

**Reference file:** `main.c`

**What it does:**
1. Reads filename from `fifo_1to6_fname` (character by character until `'\0'`).
2. Opens output file.
3. Reads bytes from `fifo_5to6_data` and writes them to the file.
4. Polls `fifo_5to6_done` for the done token, then closes the file.

**RECV mapping:**
- `RECV1()` → `(char)(fifo_read_32(FIFO_1TO6_FNAME_OUT_BASE) & 0xFF)` — variable (filename)
- `RECV2()` → `(unsigned char)(fifo_read_32(FIFO_5TO6_DATA_OUT_BASE) & 0xFF)` — variable (JPEG bytes)
- `RECV3()` → `(unsigned char)(fifo_read_32(FIFO_5TO6_DONE_OUT_BASE) & 0xFF)` — 1 value (done token)

**File I/O on Nios II:** Use the Nios II HAL (`fopen`/`fwrite`) to write to a host-mounted filesystem (via JTAG UART or Altera host filesystem). For a first milestone, skip actual file I/O: just count received bytes and print the count via JTAG UART. Later, write to an SDRAM buffer and transfer to host.

---

### Step 5: Synchronization and Correctness

- **Blocking:** Your `fifo_write`/`fifo_read` already block on full/empty, so producers and consumers naturally synchronize. No need for extra locks because each FIFO is **single-writer, single-reader** (SWSR). The Avalon interconnect guarantees atomic 32-bit accesses.
- **Data ordering:** FIFOs are first-in-first-out by definition. The reference code sends data in a fixed order (Y block, then Cb, then Cr for each MCU), and the consumer reads in the same order. As long as you preserve this order in your port, data integrity is guaranteed.
- **End of stream:** Stage 1 must signal "no more macro-blocks". In the reference, Stage 1 sends all MCU data and then terminates. Stages 2–5 loop for `num_mcus` iterations (they know the count from the parameter channel). Stage 6 watches for the done token on `fifo_5to6_done`. Reuse the same convention.
- **First run:** Use a **tiny image** (e.g. 8×8 pixels = 1 MCU = 192 bytes of raw RGB data) so you can manually trace all 192 values through each FIFO. Compare the output byte stream with a reference JPEG encoder (e.g. run the same image through the reference C code compiled for x86 and compare byte-by-byte).

---

### Step 6: Throughput and Per-Stage Timing (Measurements)

#### 6.1 How to Measure

Each Nios II has an **Interval Timer** component. Use it to measure wall-clock time for processing one MCU:

```c
#include "sys/alt_timestamp.h"

/* At the start of your stage's main loop: */
alt_timestamp_start();              /* reset and start the timer */
unsigned int t_start = alt_timestamp();

/* ... process one MCU ... */

unsigned int t_end = alt_timestamp();
unsigned int cycles = t_end - t_start;
float us = (float)cycles / (float)alt_timestamp_freq() * 1e6;
printf("Stage X: %u cycles (%.1f us) per MCU\n", cycles, us);
```

At 50 MHz, 1 cycle = 20 ns. So 1,000 cycles = 20 µs.

#### 6.2 What to Record

| Metric | How to compute | Example |
|--------|---------------|---------|
| **Per-stage latency** | Timer difference for one MCU (as above) | Stage 3: 15,000 cycles (300 µs) |
| **Pipeline throughput** | 1 / (max stage latency) | If Stage 3 = 300 µs, throughput = 3,333 MCUs/sec |
| **Frame throughput** | Pipeline throughput / MCUs per frame | For 640×480 (4:2:0): 2,400 MCUs → 3,333/2,400 ≈ 1.4 frames/sec |
| **Bottleneck** | Stage with the highest per-MCU latency | Stage 3 (DCT) — most multiplications |

**Expected bottleneck:** Stage 3 (DCT) and Stage 5 (Huffman) are typically the slowest because they involve the most computation. The paper confirms this.

---

## Part 2: Improving Performance

The practical asks you to **improve throughput** and document how. Here are ideas with specific implementation details:

### 1. Processor Customizations

| Optimization | What to change | Expected improvement | Reason |
|-------------|----------------|---------------------|--------|
| **Hardware multiply** on Stage 3 | Requires **Nios II/f** (paid Quartus). Enable "Hardware Multiply" in processor settings. Uses 2 DSP blocks. | 10–30× faster multiply (1–3 cycles vs ~32 cycles in software) | DCT performs ~2,304 multiplies per MCU. At 32 cycles each = 73,728 cycles; with hardware multiply = ~6,912 cycles. Not available in Web/Lite (use Nios II/e and accept slower DCT). |
| **Instruction cache** on Stage 3 | Enable 4 KB I-cache (4 M9K blocks, direct-mapped). | Reduces instruction fetch latency from ~2 cycles (on-chip RAM) to 1 cycle (cache hit). | DCT inner loops are small (~200 bytes) and fit entirely in 4 KB cache. High hit rate expected (>95%). |
| **Data cache** on Stage 3 | Enable 2 KB D-cache (2 M9K blocks). | Speeds up repeated access to coefficient arrays and intermediate buffers. | The DCT coefficient table (64 × INT16 = 128 bytes) and working arrays (64 × INT16 = 128 bytes) fit in 2 KB cache. |
| **Custom instruction** for DCT butterfly | Implement a single custom instruction that computes `a*cos + b*sin` in one cycle using a DSP block. | Reduces butterfly from ~6 instructions to 1. | Each 8-point DCT has 12 butterfly operations × 16 per block × 3 blocks = 576 per MCU. |

### 2. Extend the Pipeline

- Split Stage 3 (DCT) into **Stage 3a (row DCT)** and **Stage 3b (column DCT)**, giving a 7-stage pipeline.
- This halves Stage 3's latency (each sub-stage processes 8 rows or 8 columns instead of both).
- Cost: 1 additional Nios II (~700–3,000 LEs), 1 additional FIFO (1 M9K), ~32 KB additional RAM (~28 M9K blocks).
- **Reason:** If DCT is the bottleneck, splitting it nearly doubles throughput at modest area cost.

### 3. Superscalar / Parallel Stage

- Duplicate the bottleneck stage: e.g. **2 DCT cores** processing alternate MCUs (even MCUs → core A, odd MCUs → core B).
- Requires a splitter (demux) before and a merger (mux) after, each implemented as small FIFOs with arbitration logic.
- **Reason:** Doubles the throughput of the bottleneck stage. Good if the bottleneck is much slower than others (>2× the second-slowest stage).

### 4. FIFO Sizing

| Change | Why |
|--------|-----|
| Increase `fifo_2to3` depth from 64 → **128** (2 blocks) | Allows double-buffering: Stage 2 writes block N+1 while Stage 3 processes block N. Reduces stalls when stages have slightly different processing times. Cost: 1 extra M9K block. |
| Increase `fifo_3to4` depth from 64 → **128** | Same double-buffering benefit around the DCT bottleneck. |
| Increase all data FIFOs to **256** (4 blocks) | Maximum decoupling. Useful if stage latencies vary significantly between MCUs (e.g. Huffman output size varies). Cost: ~4 M9K blocks per FIFO, 28 M9K total — still well within budget. |

### 5. Algorithmic Optimizations

- **Loop unrolling** in DCT: manually unroll the 8-iteration inner loop to reduce loop overhead and help the compiler schedule instructions.
- **Fixed-point tuning:** Ensure all intermediate DCT values use INT16 (not INT32) to halve memory bandwidth and keep values in registers.
- **Lookup table for quantization:** Pre-compute `(1 << 15) / Qval` and use multiply instead of divide (divide takes ~36 cycles on Nios II/e; multiply is faster and on Nios II/f with hardware multiply is ~3 cycles).

Pick **one or two** techniques, implement them, **remeasure** throughput and per-stage latency, and write a short report: what you changed, before/after numbers, and why the bottleneck moved (or didn't).

---

## Suggested Order of Work (Checklist)

- [ ] **0.** Read Practical 4 PDF, Onchip FIFO Memory Datasheet, and paper Sections 3, 7, 8.
- [ ] **1.** Draw your 6-stage + 11-FIFO block diagram with data types and depths on each link.
- [ ] **2.** Implement one Nios II/e + one 32-bit depth-64 FIFO in Qsys; run a trivial producer/consumer test in C; confirm the FIFO driver works (write 64 values, read 64 values, verify match).
- [ ] **3.** Add the other 5 Nios II and all 11 FIFOs. **Web/Lite Edition:** use Nios II/e for all 6. **Paid Quartus:** you may use Nios II/f on stages 3 and 5. Generate and compile hardware; verify resource utilization fits. Document the memory map.
- [ ] **4.** Implement `fifo_driver.h` (with `fifo_write_32` / `fifo_read_32` — one pair for all FIFOs since all use 32-bit width) and `fifo_map.h` (with actual base addresses from `system.h`).
- [ ] **5.** Port Stage 2 first (simplest: only RECV/SEND, subtract 128); test with a harness that feeds fixed data from Stage 1 side.
- [ ] **6.** Port Stage 3 (DCT); then Stage 4, 5, 6; then Stage 1. Get one full pipeline run with a tiny 8×8 image (1 MCU).
- [ ] **7.** Add end-of-stream handling and parameter passing; verify correct JPEG output byte-by-byte against the reference encoder.
- [ ] **8.** Add interval timers; measure throughput and per-stage latency; identify the bottleneck stage.
- [ ] **9.** Part 2: apply one or two optimizations (e.g. hardware multiply + instruction cache on Stage 3, increase FIFO depth to 128); remeasure and write the comparison report.

---

## File and Folder Layout Suggestion

```
JEnc/
├── PROJECT_GUIDE_StepByStep.md   (this file)
├── hw/
│   ├── jpeg_mpsoc.qsys           (your 6-Nios + 11-FIFO system)
│   ├── jpeg_mpsoc.qpf            (Quartus project file)
│   ├── jpeg_mpsoc.qsf            (pin assignments, device = EP4CE115F29C7)
│   └── output_files/             (compiled .sof file)
├── sw/
│   ├── common/
│   │   ├── fifo_driver.h         (fifo_write_32, fifo_read_32 — uniform 32-bit driver)
│   │   ├── fifo_map.h            (base addresses for all 11 FIFOs)
│   │   ├── datatype.h            (INT8, UINT8, INT16, UINT16, INT32, UINT32)
│   │   └── config.h              (BLOCK_SIZE=64, image dimensions)
│   ├── stage1/                   (BSP + app for cpu_stage1; Nios II/e, 64 KB RAM)
│   ├── stage2/                   (BSP + app for cpu_stage2; Nios II/e, 16 KB RAM)
│   ├── stage3/                   (BSP + app for cpu_stage3; Nios II/e or /f, 32 KB RAM; /f adds 4KB I$, 2KB D$)
│   ├── stage4/                   (BSP + app for cpu_stage4; Nios II/e, 32 KB RAM)
│   ├── stage5/                   (BSP + app for cpu_stage5; Nios II/e or /f, 48 KB RAM; /f adds 4KB I$, 2KB D$)
│   └── stage6/                   (BSP + app for cpu_stage6; Nios II/e, 16 KB RAM)
└── JEnc_vlog_Final-2012-09-20/   (reference Tensilica code; do not modify, only copy from)
```

---

## Quick Reference: TIE Ports → Your FIFOs (Complete Mapping)

| Reference (Tensilica) | Data type | Width | Your FIFO | Your function call |
|-----------------------|-----------|-------|-----------|-------------------|
| `SEND1(x)` (Stage 1) | INT16 | 32-bit | `fifo_1to2_Y` | `fifo_write_32(FIFO_1TO2_Y_IN_BASE, (unsigned int)(short)x)` |
| `SEND2(x)` (Stage 1) | INT16 | 32-bit | `fifo_1to2_Cb` | `fifo_write_32(FIFO_1TO2_CB_IN_BASE, (unsigned int)(short)x)` |
| `SEND3(x)` (Stage 1) | INT16 | 32-bit | `fifo_1to2_Cr` | `fifo_write_32(FIFO_1TO2_CR_IN_BASE, (unsigned int)(short)x)` |
| `SEND4(x)` (Stage 1) | UINT32 | 32-bit | `fifo_1to4_param` | `fifo_write_32(FIFO_1TO4_PARAM_IN_BASE, x)` |
| `SEND5(x)` (Stage 1) | UINT32 | 32-bit | `fifo_1to5_param` | `fifo_write_32(FIFO_1TO5_PARAM_IN_BASE, x)` |
| `SEND6(x)` (Stage 1) | INT8 | 32-bit | `fifo_1to6_fname` | `fifo_write_32(FIFO_1TO6_FNAME_IN_BASE, (unsigned int)(unsigned char)x)` |
| `RECV1()` (Stage 2) | INT16 | 32-bit | `fifo_1to2_Y` | `(short)(fifo_read_32(FIFO_1TO2_Y_OUT_BASE) & 0xFFFF)` |
| `RECV2()` (Stage 2) | INT16 | 32-bit | `fifo_1to2_Cb` | `(short)(fifo_read_32(FIFO_1TO2_CB_OUT_BASE) & 0xFFFF)` |
| `RECV3()` (Stage 2) | INT16 | 32-bit | `fifo_1to2_Cr` | `(short)(fifo_read_32(FIFO_1TO2_CR_OUT_BASE) & 0xFFFF)` |
| `SEND(i)` (Stage 2) | INT16 | 32-bit | `fifo_2to3` | `fifo_write_32(FIFO_2TO3_IN_BASE, (unsigned int)(short)i)` |
| `RECV()` (Stage 3) | INT16 | 32-bit | `fifo_2to3` | `(short)(fifo_read_32(FIFO_2TO3_OUT_BASE) & 0xFFFF)` |
| `SEND(data[i])` (Stage 3) | INT16 | 32-bit | `fifo_3to4` | `fifo_write_32(FIFO_3TO4_IN_BASE, (unsigned int)(short)data[i])` |
| `RECV1()` (Stage 4, init) | UINT32 | 32-bit | `fifo_1to4_param` | `fifo_read_32(FIFO_1TO4_PARAM_OUT_BASE)` |
| `RECV2()` (Stage 4, data) | INT16 | 32-bit | `fifo_3to4` | `(short)(fifo_read_32(FIFO_3TO4_OUT_BASE) & 0xFFFF)` |
| `SEND()` (Stage 4) | INT16 | 32-bit | `fifo_4to5` | `fifo_write_32(FIFO_4TO5_IN_BASE, (unsigned int)(short)x)` |
| `RECV1()` (Stage 5, init) | UINT32 | 32-bit | `fifo_1to5_param` | `fifo_read_32(FIFO_1TO5_PARAM_OUT_BASE)` |
| `RECV2()` (Stage 5, data) | INT16 | 32-bit | `fifo_4to5` | `(short)(fifo_read_32(FIFO_4TO5_OUT_BASE) & 0xFFFF)` |
| `SEND1(out)` (Stage 5) | UINT8 | 32-bit | `fifo_5to6_data` | `fifo_write_32(FIFO_5TO6_DATA_IN_BASE, (unsigned int)(unsigned char)out)` |
| `SEND2(1)` (Stage 5) | UINT8 | 32-bit | `fifo_5to6_done` | `fifo_write_32(FIFO_5TO6_DONE_IN_BASE, 1)` |
| `RECV1()` (Stage 6, fname) | INT8 | 32-bit | `fifo_1to6_fname` | `(char)(fifo_read_32(FIFO_1TO6_FNAME_OUT_BASE) & 0xFF)` |
| `RECV2()` (Stage 6, data) | UINT8 | 32-bit | `fifo_5to6_data` | `(unsigned char)(fifo_read_32(FIFO_5TO6_DATA_OUT_BASE) & 0xFF)` |
| `RECV3()` (Stage 6, done) | UINT8 | 32-bit | `fifo_5to6_done` | `(unsigned char)(fifo_read_32(FIFO_5TO6_DONE_OUT_BASE) & 0xFF)` |

---

If you paste your current Qsys design or C code for any stage, I can help you wire the next step or debug specific issues.
