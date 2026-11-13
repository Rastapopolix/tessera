// SPDX-License-Identifier: GPL-3.0-only
// Andamento's virtual prototype in an AudioWorklet: andamento.wasm (web/andamento/sim.c) renders 128 frames a
// render quantum at 48 kHz; the page's controls go in as messages; about 30 times a second the screen and the LEDs
// come out. Key changes are held 24 ms of device time each, so a quick click still sounds (as web/sim/worklet.js).
const HOLD_MS = 24;
class Andamento extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.last = 0;
    this.keys = [];
    this.nextOk = 0;
    this.port.onmessage = (e) => this.onMessage(e.data);
  }
  async onMessage(m) {
    if (m.type === "load") {
      try {
        const { instance } = await WebAssembly.instantiate(m.wasm, {});
        const ex = instance.exports;
        ex._initialize();
        ex.web_volume(m.volume ?? 700);
        ex.web_boot();
        this.ex = ex;
        this.port.postMessage({ type: "ready", rate: ex.web_sample_rate() });
      } catch (err) {
        this.port.postMessage({ type: "error", message: String((err && err.message) || err) });
      }
      return;
    }
    const ex = this.ex;
    if (!ex) return;
    if (m.type === "key") this.keys.push(m);
    else if (m.type === "enc") ex.web_enc(m.i, m.n | 0);
    else if (m.type === "push") ex.web_push(m.i);
    else if (m.type === "button") ex.web_button(m.b, m.down ? 1 : 0);
    else if (m.type === "volume") ex.web_volume(m.value | 0);
    else if (m.type === "demo") ex.web_demo(m.slot | 0);
    else if (m.type === "frame") this.publish();
  }
  publish() {
    const ex = this.ex;
    const fb = new Uint8Array(ex.memory.buffer, ex.web_screen(), 128 * 64).slice();
    const keys = new Uint8Array(ex.memory.buffer, ex.web_led_keys(), 16 * 3).slice();   // (16 x RGB)
    this.port.postMessage({ type: "frame", fb, buttons: ex.web_led_buttons(), keys, playing: ex.web_playing() },
                          [fb.buffer, keys.buffer]);
  }
  process(inputs, outputs) {
    const ex = this.ex;
    if (!ex) return true;
    const now = ex.web_now_ms();
    while (this.keys.length && now - this.nextOk >= 0) {
      const k = this.keys.shift();
      ex.web_key(k.k, k.down ? 1 : 0);
      this.nextOk = now + HOLD_MS;
    }
    const out = outputs[0], n = out[0].length;
    ex.web_render(n);
    out[0].set(new Float32Array(ex.memory.buffer, ex.web_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(ex.memory.buffer, ex.web_out_r(), n));
    if (currentTime - this.last >= 1 / 30) {
      this.last = currentTime;
      this.publish();
    }
    return true;
  }
}
registerProcessor("andamento", Andamento);
