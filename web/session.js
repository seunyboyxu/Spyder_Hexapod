import { encodeCommand, parseLine, Snapshot } from "./protocol.js";
export class Session extends EventTarget {
  constructor({
    pollMs = 750,
    heartbeatMs = 250,
    timeoutMs = 2500,
    cleanupMs = 1000,
    clock = () => performance.now(),
  } = {}) {
    super();
    this.options = { pollMs, heartbeatMs, timeoutMs, cleanupMs };
    this.clock = clock;
    this.lastHeartbeat = -Infinity;
    this.safetyPending = 0;
    this.controlEpoch = 0;
    this.requestOrder = 0;
    this.stateOrder = 0;
    this.ready = false;
    this.state = null;
    this.pending = new Map();
    this.queue = [];
    this.nextId = 1;
    this.generation = 0;
    this.busy = false;
    this.timers = [];
  }
  notify(type, detail) {
    this.dispatchEvent(new CustomEvent(type, { detail }));
  }
  async connect(transport) {
    await this.disconnect();
    this.transport = transport;
    const generation = this.generation;
    const attempt = { generation, error: null };
    this.attempt = attempt;
    this.opening = true;
    try {
      await transport.open(
        (line) => {
          if (generation === this.generation) this.receive(line);
        },
        (error) => {
          if (generation === this.generation) this.fault(error);
        },
      );
      if (transport.settleMs)
        await new Promise((resolve) => setTimeout(resolve, transport.settleMs));
      if (generation !== this.generation)
        throw attempt.error || new Error("Connection cancelled");
      await this.request("HELLO");
      await this.request("STATUS");
      this.ready = true;
      this.opening = false;
      this.notify("status", this.state);
      const periodic = (ms, verb) => {
        if (ms)
          this.timers.push(
            setInterval(() => {
              if (
                this.ready &&
                !this.busy &&
                !this.pending.size &&
                !this.queue.length
              )
                this.enqueue(verb, [], false).catch(() => {});
            }, ms),
          );
      };
      periodic(this.options.heartbeatMs, "PING");
      periodic(this.options.pollMs, "STATUS");
    } catch (error) {
      // The original fault already invalidated this attempt. An old startup
      // must neither hide that diagnostic nor abort a newer connection.
      if (generation === this.generation) this.fault(error);
      throw error;
    }
  }
  fault(error) {
    if (this.attempt?.generation === this.generation)
      this.attempt.error = error;
    this.ready = false;
    this.opening = false;
    this.generation++;
    this.controlEpoch++;
    this.safetyPending = 0;
    this.busy = false;
    this.timers.forEach(clearInterval);
    this.timers = [];
    for (const p of this.pending.values()) {
      clearTimeout(p.timer);
      p.reject(error);
    }
    this.pending.clear();
    this.queue.splice(0).forEach((p) => p.reject(error));
    if (this.transport) {
      try {
        this.cleanupTask = Promise.resolve(
          this.transport.abort
            ? this.transport.abort()
            : this.transport.close(),
        ).catch(() => {});
      } catch {
        this.cleanupTask = Promise.resolve();
      }
    }
    this.notify("fault", error.message || String(error));
  }
  async disconnect() {
    this.fault(new Error("Disconnected"));
    const old = this.transport;
    this.transport = null;
    this.state = null;
    if (old) {
      let timer;
      try {
        await Promise.race([
          Promise.resolve().then(() => old.close()),
          new Promise((_, reject) => {
            timer = setTimeout(
              () =>
                reject(
                  new Error(
                    "Port cleanup timed out. Unplug the Uno or reload before reconnecting.",
                  ),
                ),
              this.options.cleanupMs,
            );
          }),
        ]);
      } finally {
        clearTimeout(timer);
      }
    }
  }
  receive(line) {
    try {
      const message = parseLine(line);
      if (message.type === "BOOT") {
        if (!this.opening)
          this.fault(new Error("Uno reset: reconnect and rearm deliberately"));
        return;
      }
      if (message.type === "SPYDER") {
        for (const p of this.pending.values())
          if (p.verb === "HELLO") p.version = true;
        return;
      }
      const p = this.pending.get(message.id);
      if (!p) return;
      if (message.type === "ERR") {
        this.pending.delete(message.id);
        clearTimeout(p.timer);
        p.reject(new Error(`Device: ${message.code}`));
        return;
      }
      if (["BEGIN", "JOINT", "END"].includes(message.type)) {
        if (p.verb !== "STATUS") throw new Error("Unexpected snapshot");
        const state = p.snapshot.accept(message);
        if (state) p.state = state;
        return;
      }
      if (message.type === "ACK") {
        if (
          (p.verb === "HELLO" && !p.version) ||
          (p.verb === "STATUS" && !p.state)
        )
          throw new Error("Incomplete response");
        this.pending.delete(message.id);
        clearTimeout(p.timer);
        if (p.verb === "PING" || p.verb === "RESUME")
          this.lastHeartbeat = this.clock();
        if (p.state && p.order > this.stateOrder) {
          this.stateOrder = p.order;
          this.state = p.state;
          this.notify("status", this.state);
        }
        p.resolve();
      }
    } catch (error) {
      this.fault(error);
    }
  }
  request(verb, args = [], urgent = false) {
    if (!this.transport || (!this.ready && !this.opening))
      return Promise.reject(new Error("Serial session is no longer active"));
    const generation = this.generation;
    const epoch = this.controlEpoch;
    const id = this.nextId++;
    if (this.nextId > 65535) this.nextId = 1;
    const line = encodeCommand(id, verb, args);
    return new Promise((resolve, reject) => {
      const p = {
        verb,
        resolve,
        reject,
        snapshot: new Snapshot(id),
        order: ++this.requestOrder,
      };
      p.timer = setTimeout(
        () =>
          this.fault(
            new Error("Serial acknowledgement timeout: device state unknown"),
          ),
        this.options.timeoutMs,
      );
      this.pending.set(id, p);
      Promise.resolve()
        .then(() => {
          if (generation !== this.generation || !this.pending.has(id)) return;
          if (!urgent && epoch !== this.controlEpoch) {
            this.pending.delete(id);
            clearTimeout(p.timer);
            p.reject(new Error("Cancelled by safety control"));
            return;
          }
          return this.transport.write(line);
        })
        .catch((error) => {
          if (generation === this.generation) this.fault(error);
        });
    });
  }
  command(verb, args = []) {
    if (!this.ready)
      return Promise.reject(new Error("Reconnect before commanding"));
    if (verb === "STOP") return this.stop();
    if (verb === "RELEASE") return this.release();
    if (this.safetyPending)
      return Promise.reject(
        new Error(
          "Wait for STOP/RELEASE acknowledgement before further commands",
        ),
      );
    return this.enqueue(verb, args, true);
  }
  enqueue(verb, args, readback, priority = false) {
    return new Promise((resolve, reject) => {
      const item = {
        verb,
        args,
        readback,
        resolve,
        reject,
        epoch: this.controlEpoch,
      };
      if (priority) this.queue.unshift(item);
      else this.queue.push(item);
      this.drain();
    });
  }
  async drain() {
    if (this.busy || this.safetyPending) return;
    this.busy = true;
    const generation = this.generation;
    try {
      while (
        this.queue.length &&
        this.ready &&
        !this.safetyPending &&
        generation === this.generation
      ) {
        const p = this.queue.shift();
        try {
          if (p.verb !== "PING") await this.heartbeatIfDue();
          if (p.epoch !== this.controlEpoch)
            throw new Error("Cancelled by safety control");
          await this.request(p.verb, p.args);
          if (p.epoch !== this.controlEpoch)
            throw new Error("Cancelled by safety control");
          if (p.readback && p.verb !== "STATUS") {
            await this.heartbeatIfDue();
            if (p.epoch !== this.controlEpoch)
              throw new Error("Cancelled by safety control");
            await this.request("STATUS");
          }
          if (p.epoch !== this.controlEpoch)
            throw new Error("Cancelled by safety control");
          p.resolve(this.state);
        } catch (error) {
          p.reject(error);
        }
      }
    } finally {
      if (generation === this.generation) {
        this.busy = false;
        if (this.queue.length && !this.safetyPending) this.drain();
      }
    }
  }
  async heartbeatIfDue() {
    if (
      this.options.heartbeatMs &&
      !this.safetyPending &&
      this.clock() - this.lastHeartbeat >= this.options.heartbeatMs
    ) {
      await this.request("PING");
    }
  }
  stop() {
    return this.interrupt("STOP");
  }
  release() {
    return this.interrupt("RELEASE");
  }
  interrupt(verb) {
    if (!this.ready)
      return Promise.reject(
        new Error(`Cannot confirm ${verb}: device state unknown`),
      );
    this.queue
      .splice(0)
      .forEach((p) => p.reject(new Error(`Cancelled by ${verb}`)));
    this.controlEpoch++;
    this.safetyPending++;
    this.notify("activity");
    const generation = this.generation;
    return (async () => {
      try {
        // Complete-line writes remain ordered by the transport, but a pending
        // response no longer prevents sending an urgent control command.
        await this.request(verb, [], true);
        await this.request("STATUS", [], true);
        return this.state;
      } finally {
        if (generation === this.generation) {
          this.safetyPending--;
          this.notify("activity");
          if (!this.busy && this.queue.length) this.drain();
        }
      }
    })();
  }
}
export class SerialTransport {
  constructor(port) {
    this.port = port;
    this.settleMs = 1800;
    this.closed = false;
  }
  async open(onLine, onError) {
    await this.port.open({ baudRate: 115200 });
    this.writer = this.port.writable.getWriter();
    this.reader = this.port.readable.getReader();
    this.readTask = (async () => {
      const decoder = new TextDecoder();
      let buffer = "";
      try {
        while (!this.closed) {
          const { value, done } = await this.reader.read();
          if (done) break;
          buffer += decoder.decode(value, { stream: true });
          if (buffer.length > 4096) throw new Error("Serial receive overflow");
          let split;
          while ((split = buffer.indexOf("\n")) >= 0) {
            const line = buffer.slice(0, split).replace(/\r$/, "");
            buffer = buffer.slice(split + 1);
            if (line) onLine(line);
          }
        }
        if (!this.closed) onError(new Error("Serial connection ended"));
      } catch (error) {
        if (!this.closed) onError(error);
      }
    })();
  }
  async write(line) {
    await this.writer.write(new TextEncoder().encode(line));
  }
  abort() {
    return this.close();
  }
  close() {
    if (!this.closeTask) this.closeTask = this.closePort();
    return this.closeTask;
  }
  async closePort() {
    this.closed = true;
    if (this.reader) {
      await this.reader.cancel().catch(() => {});
      await this.readTask;
      this.reader.releaseLock();
    }
    if (this.writer) {
      // Cancel queued output instead of draining stale motion commands.
      // Bytes already transmitted cannot be recalled; firmware lease/hold is authoritative.
      await this.writer.abort().catch(() => {});
      this.writer.releaseLock();
    }
    await this.port.close();
  }
}
