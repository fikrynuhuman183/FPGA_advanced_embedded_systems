# Practical 4: JPEG MPSoC (JEnc)

6-stage pipelined JPEG encoder on Terasic DE2-115 (Cyclone IV E) using Nios II processors and on-chip FIFOs.

## Quartus Web / Lite Edition (free)

**Nios II/f is not available in the free Quartus Web Edition or Quartus Prime Lite Edition.** The free license includes only the **Nios II/e** (economy) core.

- Use **Nios II/e for all 6 pipeline stages** when using the free edition.
- The design works; stages 3 (DCT) and 5 (Huffman) run slower without hardware multiply and caches, but the algorithm is unchanged.
- **Nios II/f** (and Nios II/s) require a **paid Quartus Prime Standard or Pro** license. With a paid license you may use Nios II/f on stages 3 and 5 for better performance.

## Documentation

- **[PROJECT_GUIDE_StepByStep.md](PROJECT_GUIDE_StepByStep.md)** — Full step-by-step guide (hardware in Qsys/Platform Designer, software per stage, FIFO map).
- **[REFERENCE_FIFO_MAP.md](REFERENCE_FIFO_MAP.md)** — FIFO connectivity reference.

## Tools

- Quartus Prime (Web/Lite or Standard) with Cyclone IV E support (EP4CE115F29C7)
- Nios II Software Build Tools (SBT) / Nios II EDS
- DE2-115 board and USB-Blaster
