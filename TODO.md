# TODO

Deferred work items for signal_gen. For the public-facing summary, see the Future Work section in README.md.

---

## Pending

- [ ] **Si5351A MCLK** — populate Si5351A breakout, implement I2C driver (I2C1, GP14/GP15), configure CLK0 for 12.288MHz. Decision required on sync strategy: all three I2S clocks from Si5351 (PIO in slave mode) vs. MCLK only (PIO divides BCK/LRCK from MCLK input). The latter is simpler but requires PIO GPIO input handling.

- [ ] **Additional waveforms** — square wave is straightforward (binary phase accumulator). Triangle requires integration of a square wave or a separate LUT. Both need audio_engine changes; the UI will need a waveform selector on a button long-press or a third encoder.

- [ ] **Frequency arc display** — the logarithmic arc mapping was noted as potentially unintuitive in real use. Revisit after more bench time. Switching to linear is a one-line change in `_freq_to_arc()` in `src/ui/ui.c`.

- [ ] **Output stage** — current PCM5102A direct output is 2.2Vrms at 100% amplitude. Evaluate op-amp buffer with resistor-ratio attenuation for driving lower-impedance loads or producing a calibrated reference level (+4dBu = 1.228Vrms, 0dBV = 1.000Vrms).

---

*Last updated: April 2026*
