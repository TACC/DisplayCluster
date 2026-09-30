// The offscreen document, where streams run: each captures its tab, encodes
// the frames as JPEG and sends them to the wall's /stream/{name} WebSocket.
//
// Frames go one at a time: the next is sent when the wall acks the last, so
// a slow network or wall drops frames here rather than queueing them. A tab
// that isn't changing produces no frames, and sends nothing.
//
// Chrome captures a tab at a fixed size, fitting the page into it with black
// bars when their shapes differ. So the capture starts at the tab's shape, and
// when the tab is resized after, each frame is cropped back to its new shape.

const streams = new Map();   // tab ID -> Stream

// how many names to try, "Name", "Name (2)" ... when one is taken
const MAX_NAME_TRIES = 9;

function report(tabId, state) {
  chrome.runtime.sendMessage({ target: 'background', type: 'state', tabId, state }).catch(() => {});
}

// the part of a frame showing the page, without Chrome's bars, for a tab shaped
// width x height
export function pageRect(frameWidth, frameHeight, width, height) {
  const aspect = width / height;
  const frameAspect = frameWidth / frameHeight;

  if (!(aspect > 0) || Math.abs(frameAspect - aspect) / aspect < 0.01) {
    return { x: 0, y: 0, width: frameWidth, height: frameHeight };
  }

  // a pixel in from the bars, which Chrome's scaling blurs into the page
  if (frameAspect > aspect) {
    const w = Math.round(frameHeight * aspect) - 2;
    return { x: Math.round((frameWidth - w) / 2), y: 0, width: w, height: frameHeight };
  }
  const h = Math.round(frameWidth / aspect) - 2;
  return { x: 0, y: Math.round((frameHeight - h) / 2), width: frameWidth, height: h };
}

class Stream {
  constructor(tabId, name, settings, tabSize) {
    this.tabId = tabId;
    this.baseName = name;
    this.settings = settings;
    this.tabSize = tabSize;  // { width, height } in CSS pixels
    this.tries = 1;
    this.acked = 0;
    this.ws = null;
    this.latest = null;      // the newest frame not yet sent (a VideoFrame)
    this.lastJpeg = null;    // the last frame sent, to send again on reconnecting
    this.inFlight = false;
    this.lastSent = 0;
    this.timer = null;
    this.stopped = false;
    this.canvas = new OffscreenCanvas(1, 1);
    this.context = this.canvas.getContext('2d', { alpha: false });
  }

  get name() {
    return this.tries > 1 ? `${this.baseName} (${this.tries})` : this.baseName;
  }

  async start(streamId) {
    const { maxSize, fps } = this.settings;

    // the tab's size in screen pixels, scaled down to fit maxSize
    const width = this.tabSize.width * devicePixelRatio;
    const height = this.tabSize.height * devicePixelRatio;
    const scale = Math.min(1, maxSize / Math.max(width, height));

    try {
      this.media = await navigator.mediaDevices.getUserMedia({
        audio: false,
        video: { mandatory: {
          chromeMediaSource: 'tab', chromeMediaSourceId: streamId,
          maxWidth: Math.round(width * scale), maxHeight: Math.round(height * scale), maxFrameRate: fps,
        } },
      });
    } catch (e) {
      this.fail(`couldn't capture the tab (${e.message})`);
      return;
    }

    const track = this.media.getVideoTracks()[0];
    // the tab closed, or Chrome stopped the capture
    track.addEventListener('ended', () => this.stop());

    this.reader = new MediaStreamTrackProcessor({ track }).readable.getReader();
    this.readFrames();
    this.connect();
  }

  async readFrames() {
    try {
      for (;;) {
        const { value, done } = await this.reader.read();
        if (done) break;
        // only the newest frame is worth sending; frames must be closed promptly
        // or the capture stalls
        this.latest?.close();
        this.latest = value;
        this.pump();
      }
    } catch {
      // cancelled by stop()
    }
  }

  connect() {
    const wall = this.settings.wall.replace(/^http/, 'ws');
    const url = `${wall}/stream/${encodeURIComponent(this.name)}?access_token=${encodeURIComponent(this.settings.token)}`;
    const ws = new WebSocket(url);
    this.ws = ws;
    this.inFlight = false;

    ws.addEventListener('open', () => {
      // a page that isn't changing has no new frame to open the window with
      if (!this.latest && this.lastJpeg) {
        this.inFlight = true;
        ws.send(this.lastJpeg);
      } else {
        this.pump();
      }
    });

    ws.addEventListener('message', (event) => {
      if (event.data !== 'ack') return;
      if (this.acked++ === 0) {
        report(this.tabId, { state: 'streaming', name: this.name, message: '' });
      }
      this.inFlight = false;
      this.pump();
    });

    ws.addEventListener('close', async (event) => {
      if (this.ws !== ws || this.stopped) return;
      this.ws = null;

      // the name is taken: try the next one
      if (event.code === 1008 && this.acked === 0 && this.tries < MAX_NAME_TRIES && /already/.test(event.reason)) {
        this.tries++;
        this.connect();
        return;
      }

      this.fail(await this.explain(event));
    });
  }

  // why a connection ended, in words
  async explain(event) {
    if (event.code === 1007) return 'the wall couldn\'t read a frame';
    if (event.reason) return event.reason;
    if (this.acked > 0) return 'the wall closed the stream, or the connection to it was lost';

    // a refused handshake looks the same as an unreachable wall to a
    // WebSocket (1006, no reason); asking over HTTP tells them apart. With
    // access to the wall's address (granted in the options) the answer is
    // readable; without, only whether there was one.
    try {
      const response = await fetch(`${this.settings.wall}/status`, {
        headers: { Authorization: `Bearer ${this.settings.token}` },
      });
      if (response.status === 401) return 'the wall didn\'t accept the token';
      return `the wall refused the stream (HTTP ${response.status})`;
    } catch {
      try {
        await fetch(`${this.settings.wall}/status`, { mode: 'no-cors' });
        return 'the wall refused the stream; check the token in the options';
      } catch {
        return `couldn't reach the wall at ${this.settings.wall}`;
      }
    }
  }

  pump() {
    if (this.stopped || this.inFlight || !this.latest || this.ws?.readyState !== WebSocket.OPEN) return;

    // at most settings.fps frames a second
    const wait = this.lastSent + 1000 / this.settings.fps - performance.now();
    if (wait > 0) {
      this.timer ??= setTimeout(() => { this.timer = null; this.pump(); }, wait);
      return;
    }

    const frame = this.latest;
    this.latest = null;
    this.inFlight = true;
    this.lastSent = performance.now();
    this.send(frame);
  }

  async send(frame) {
    const ws = this.ws;
    try {
      const page = pageRect(frame.displayWidth, frame.displayHeight, this.tabSize.width, this.tabSize.height);
      if (this.canvas.width !== page.width || this.canvas.height !== page.height) {
        this.canvas.width = page.width;
        this.canvas.height = page.height;
      }
      this.context.drawImage(frame, page.x, page.y, page.width, page.height, 0, 0, page.width, page.height);
    } finally {
      frame.close();
    }

    const jpeg = await this.canvas.convertToBlob({ type: 'image/jpeg', quality: this.settings.quality });
    this.lastJpeg = jpeg;

    if (ws === this.ws && ws?.readyState === WebSocket.OPEN) {
      ws.send(jpeg);
    } else {
      this.inFlight = false;
    }
  }

  fail(message) {
    this.stop({ state: 'error', name: this.name, message });
  }

  // ends the stream, closing its window on the wall; state is what the
  // toolbar button shows after (null: nothing); silent leaves it alone
  stop(state = null, { silent = false } = {}) {
    if (this.stopped) return;
    this.stopped = true;

    clearTimeout(this.timer);
    this.ws?.close(1000);
    this.ws = null;
    this.reader?.cancel().catch(() => {});
    this.media?.getTracks().forEach((track) => track.stop());
    this.latest?.close();
    this.latest = null;

    if (streams.get(this.tabId) === this) streams.delete(this.tabId);
    if (!silent) report(this.tabId, state);

    if (streams.size === 0) {
      chrome.runtime.sendMessage({ target: 'background', type: 'idle' }).catch(() => {});
    }
  }
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.target !== 'offscreen') return;

  if (message.type === 'start') {
    streams.get(message.tabId)?.stop(null, { silent: true });
    const stream = new Stream(message.tabId, message.name, message.settings, message.tabSize);
    streams.set(message.tabId, stream);
    stream.start(message.streamId);
  } else if (message.type === 'stop') {
    streams.get(message.tabId)?.stop();
  } else if (message.type === 'resize') {
    const stream = streams.get(message.tabId);
    if (stream) stream.tabSize = message.tabSize;
  }

  sendResponse({ ok: true });
});
