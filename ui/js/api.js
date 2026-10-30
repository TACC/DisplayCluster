// Talks to the wall's REST API: the token, control of the wall, the event
// stream, and throttled updates for drags.

const TOKEN_KEY = 'dc.token';
const LEASE_KEY = 'dc.lease';

export class ApiError extends Error {
  constructor(status, body) {
    super((body && body.error) || `HTTP ${status}`);
    this.status = status;
    this.body = body || {};
  }
}

function storageGet(storage, key) {
  try { return storage.getItem(key); } catch (e) { return null; }
}

function storageSet(storage, key, value) {
  try {
    if (value === null) storage.removeItem(key);
    else storage.setItem(key, value);
  } catch (e) { /* storage blocked: we just won't remember */ }
}

export class Api {
  constructor() {
    this.token = storageGet(sessionStorage, TOKEN_KEY) || storageGet(localStorage, TOKEN_KEY);

    // control survives a page reload (for as long as the wall still honours
    // it), but not a new tab
    const lease = JSON.parse(storageGet(sessionStorage, LEASE_KEY) || 'null');
    this.lease = lease ? lease.lease : null;
    this.leaseId = lease ? lease.id : null;

    this.events = null;
    this.onSnapshot = () => {};
    this.onConnection = () => {};
    this.onUnauthorized = () => {};
    this.onControlRetaken = () => {};
    this.retaking = null;
  }

  setToken(token, remember) {
    this.token = token;
    storageSet(sessionStorage, TOKEN_KEY, token);
    storageSet(localStorage, TOKEN_KEY, remember ? token : null);
  }

  forgetToken() {
    this.token = null;
    storageSet(sessionStorage, TOKEN_KEY, null);
    storageSet(localStorage, TOKEN_KEY, null);
  }

  // A change refused because our control merely lapsed (this tab was
  // suspended, the computer slept) takes control back - only if no one else
  // has it - and goes again. So does one that raced such a retake and went
  // out with the old lease.
  async request(method, path, body, retried) {
    const lease = this.lease;
    try {
      return await this.send(method, path, body);
    } catch (e) {
      // not for taking control itself, nor the heartbeat: the app decides
      // whether to take control back when it merely checks (see app.js)
      if (retried || !(e instanceof ApiError) || e.status !== 423 || !lease ||
          path === '/control' || path === '/control/heartbeat') throw e;
      if (e.body.lapsed && lease === this.lease) {
        try {
          await this.retake();
        } catch (e2) {
          throw e2 instanceof ApiError && e2.status === 423 ? e2 : e;
        }
      } else if (!this.lease || lease === this.lease) {
        throw e;
      }
      return this.request(method, path, body, true);
    }
  }

  // one retake however many requests find the lease lapsed at once
  retake() {
    if (!this.retaking) {
      this.retaking = this.takeControl(false)
        .then((reply) => { this.onControlRetaken(); return reply; })
        .finally(() => { this.retaking = null; });
    }
    return this.retaking;
  }

  async send(method, path, body) {
    const headers = {};
    if (this.token) headers['Authorization'] = 'Bearer ' + this.token;
    if (this.lease) headers['X-DC-Lease'] = this.lease;
    if (body !== undefined) headers['Content-Type'] = 'application/json';

    let response;
    try {
      response = await fetch(path, { method, headers, body: body === undefined ? undefined : JSON.stringify(body) });
    } catch (e) {
      throw new ApiError(0, { error: "Couldn't reach the wall" });
    }

    let reply = null;
    try { reply = await response.json(); } catch (e) { /* no body */ }

    if (response.status === 401) this.onUnauthorized();
    if (!response.ok) throw new ApiError(response.status, reply);
    return reply;
  }

  get(path) { return this.request('GET', path); }

  // fetches an image with the token (which an <image> can't send) and returns
  // a local URL for it
  async imageUrl(path) {
    const headers = this.token ? { 'Authorization': 'Bearer ' + this.token } : {};
    const response = await fetch(path, { headers });
    if (!response.ok) throw new ApiError(response.status, null);
    return URL.createObjectURL(await response.blob());
  }

  // ---- control ----

  hasControl(status) {
    return !!(this.lease && status && status.controller && status.controller.id === this.leaseId);
  }

  async takeControl(force) {
    const label = this.clientLabel();
    const reply = await this.request('POST', '/control', { label, force: !!force });
    this.setLease(reply.lease, reply.id);
    return reply;
  }

  async releaseControl() {
    if (!this.lease) return;
    try { await this.request('DELETE', '/control'); } catch (e) { /* lapsed already */ }
    this.setLease(null, null);
  }

  setLease(lease, id) {
    const changed = lease !== this.lease;
    this.lease = lease;
    this.leaseId = id;
    storageSet(sessionStorage, LEASE_KEY, lease ? JSON.stringify({ lease, id }) : null);
    // the event stream carries the lease, which keeps it alive
    if (changed && this.events) this.connectEvents();
  }

  clientLabel() {
    const ua = navigator.userAgent;
    const device = /iPad/.test(ua) ? 'iPad' : /iPhone/.test(ua) ? 'iPhone' : /Android/.test(ua) ? 'Android' :
                   /Mac/.test(ua) ? 'Mac' : /Windows/.test(ua) ? 'Windows' : /Linux/.test(ua) ? 'Linux' : 'browser';
    return `Web UI on ${device}`;
  }

  // ---- events ----

  connectEvents() {
    if (this.events) this.events.close();

    const params = new URLSearchParams();
    if (this.token) params.set('access_token', this.token);
    if (this.lease) params.set('lease', this.lease);

    const events = new EventSource('/events?' + params.toString());
    this.events = events;

    events.onopen = () => this.onConnection(true);
    events.onmessage = (e) => {
      try { this.onSnapshot(JSON.parse(e.data)); } catch (err) { console.error(err); }
    };
    events.onerror = () => {
      if (this.events !== events) return;
      this.onConnection(false);
      // EventSource retries by itself, except after an HTTP error like 401;
      // check which it was
      if (events.readyState === EventSource.CLOSED) {
        this.get('/status').then(() => setTimeout(() => this.events === events && this.connectEvents(), 2000))
          .catch(() => setTimeout(() => this.events === events && this.connectEvents(), 5000));
      }
    };
  }

  disconnectEvents() {
    if (this.events) this.events.close();
    this.events = null;
  }
}

// Sends a stream of updates (e.g. from a drag) to one window, at most every
// `interval` ms and one request at a time, always sending the latest values;
// finish() sends whatever's left and resolves when it's done.
export class ThrottledPatch {
  constructor(api, interval, onError) {
    this.api = api;
    this.interval = interval;
    this.onError = onError;
    this.pending = null;
    this.inFlight = null;
    this.lastSent = 0;
    this.timer = null;
  }

  update(path, params) {
    if (this.pending && this.pending.path === path) Object.assign(this.pending.params, params);
    else this.pending = { path, params: { ...params } };
    this.schedule();
  }

  schedule() {
    if (this.inFlight || this.timer || !this.pending) return;
    const wait = Math.max(0, this.lastSent + this.interval - Date.now());
    this.timer = setTimeout(() => { this.timer = null; this.send(); }, wait);
  }

  send() {
    if (!this.pending || this.inFlight) return this.inFlight;
    const { path, params } = this.pending;
    this.pending = null;
    this.lastSent = Date.now();
    this.inFlight = this.api.request('PATCH', path, params)
      .catch((e) => this.onError(e))
      .finally(() => { this.inFlight = null; this.schedule(); });
    return this.inFlight;
  }

  async finish() {
    if (this.timer) { clearTimeout(this.timer); this.timer = null; }
    while (this.inFlight || this.pending) {
      if (this.inFlight) await this.inFlight;
      else await this.send();
    }
  }
}

export function windowPath(name) {
  return '/windows/' + encodeURIComponent(name);
}
