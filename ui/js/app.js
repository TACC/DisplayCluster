import { Api, ApiError, ThrottledPatch, windowPath } from './api.js';
import { WallView, baseName } from './wall.js';
import { FileBrowser, StateDialog, typeIcon, typeOfUri } from './browser.js';

const $ = (id) => document.getElementById(id);

const OPTION_LABELS = {
  constrainAspectRatio: 'Keep content aspect ratio',
  showWindowBorders: 'Show window borders',
  showContentLabels: 'Show content labels',
  showTestPattern: 'Show test pattern',
  enableMullionCompensation: 'Compensate for mullions',
  showZoomContext: 'Show zoom context',
  enableStreamingSynchronization: 'Synchronize streams',
  showStreamingSegments: 'Show stream segments',
  showStreamingStatistics: 'Show stream statistics',
};

const api = new Api();
let config = null;
let state = { status: { asleep: false, controller: null, idleTimeout: 0 }, windows: [], options: {} };
let hadControl = false;
let lastSelected = null;
let lastActivity = Date.now();
const patcher = new ThrottledPatch(api, 66, (e) => showError(e));

// ---- feedback ----

let toastTimer = null;

function toast(message) {
  const t = $('toast');
  t.textContent = message;
  t.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.hidden = true; }, 4000);
}

function showError(e) {
  if (e instanceof ApiError && e.status === 423) {
    const holder = e.body.controller;
    toast(holder ? `${holder.label} has control of the wall.` : 'Take control of the wall first.');
  } else {
    toast(e.message || String(e));
  }
}

async function call(method, path, body) {
  try {
    return await api.request(method, path, body);
  } catch (e) {
    showError(e);
    return null;
  }
}

// whether the wall has confirmed our lease since the last snapshot that
// disagreed; covers the moment after taking control, when a snapshot from
// just before can still arrive naming the previous holder
let controlConfirmed = false;
let verifying = null;

function canEdit() {
  return !!api.lease && (api.hasControl(state.status) || controlConfirmed);
}

// a snapshot says someone else has control: ask the wall whether that's so
function verifyControl() {
  if (verifying || !api.lease) return;
  controlConfirmed = false;
  verifying = api.request('POST', '/control/heartbeat')
    .then(() => { controlConfirmed = true; })
    .catch((e) => {
      if (!(e instanceof ApiError) || e.status !== 423) return;
      const holder = e.body.controller;
      api.setLease(null, null);
      toast(holder ? `${holder.label} took control of the wall.` : 'Your control of the wall lapsed.');
    })
    .finally(() => { verifying = null; onSnapshot(state); });
}

// ---- the wall view ----

const wall = new WallView($('wall'), {
  canEdit,
  onSelect: (name) => {
    // selecting another window puts a filled one back where it was
    const previous = lastSelected && state.windows.find((w) => w.name === lastSelected);
    if (previous && previous.filled && name !== previous.name && canEdit()) {
      call('PATCH', windowPath(previous.name), { filled: false });
    }
    lastSelected = name;
    renderList();
    renderInspector();
  },
  onFront: (name) => {
    const top = Math.max(...state.windows.map((w) => w.z));
    const w = state.windows.find((w) => w.name === name);
    if (w && w.z < top) call('PATCH', windowPath(name), { front: true });
  },
  onChange: (name, props) => { patcher.update(windowPath(name), props); renderInspector(); },
  onChangeDone: () => patcher.finish(),
  onClose: (name) => call('DELETE', windowPath(name)),
  onHint: (text) => { $('wall-hint').textContent = text; },
  thumbnail: (url) => thumbnail(url),
});

// thumbnails by URL: the image once loaded, null while loading or if there's
// none. URLs carry the file's version, so each is fetched once
const thumbnails = new Map();

function thumbnail(url) {
  if (!thumbnails.has(url)) {
    thumbnails.set(url, null);
    api.imageUrl(url)
      .then((src) => { thumbnails.set(url, src); wall.render(); })
      .catch(() => { /* no thumbnail: keep the placeholder */ });
  }
  return thumbnails.get(url);
}

// ---- rendering state ----

function onSnapshot(snapshot) {
  const before = state.status;
  state = snapshot;

  if (api.lease) {
    if (api.hasControl(state.status)) controlConfirmed = true;
    else if (!verifying && before.controller !== state.status.controller) verifyControl();
  }

  const has = canEdit();
  if (has && !hadControl) lastActivity = Date.now();
  if (has !== hadControl && wall.viewing) wall.setViewing(null);
  hadControl = has;

  if (before.asleep && !state.status.asleep) lastActivity = Date.now();

  wall.setState(state.windows, state.options);
  wall.updateHint();
  renderControl();
  renderList();
  renderInspector();
  renderOptions();
  $('asleep').hidden = !state.status.asleep;
}

function renderControl() {
  const pill = $('control-pill');
  const button = $('control-button');
  const holder = state.status.controller;

  if (canEdit()) {
    pill.textContent = 'You have control';
    pill.className = 'pill ok';
    button.textContent = 'Release';
  } else if (holder) {
    pill.textContent = `Controlled by ${holder.label}`;
    pill.className = 'pill warn';
    button.textContent = 'Take control';
  } else {
    pill.textContent = 'No one has control';
    pill.className = 'pill';
    button.textContent = 'Take control';
  }

  $('sleep-button').disabled = !canEdit() || state.status.asleep;
  $('open-button').disabled = !canEdit();
  $('tab-open').disabled = !canEdit();
  $('load-state-button').disabled = !canEdit();
  $('clear-button').disabled = !canEdit() || !state.windows.length;
}

function renderList() {
  const list = $('window-list');
  const windows = [...state.windows].sort((a, b) => b.z - a.z);
  $('window-list-empty').hidden = windows.length > 0;

  list.replaceChildren(...windows.map((w) => {
    const item = document.createElement('li');
    item.className = (w.name === wall.selected ? 'selected' : '') + (w.hidden ? ' is-hidden' : '');

    const icon = document.createElement('span');
    icon.className = 'type';
    icon.textContent = typeIcon(typeOfUri(w.uri));
    icon.setAttribute('aria-hidden', 'true');

    const label = document.createElement('span');
    label.className = 'window-label';
    label.textContent = baseName(w.name);
    label.title = w.name;

    const eye = document.createElement('button');
    eye.className = 'eye';
    eye.textContent = w.hidden ? '◌' : '◉';
    eye.title = w.hidden ? 'Show' : 'Hide';
    eye.setAttribute('aria-label', (w.hidden ? 'Show ' : 'Hide ') + baseName(w.name));
    eye.disabled = !canEdit();
    eye.addEventListener('click', (e) => {
      e.stopPropagation();
      call('PATCH', windowPath(w.name), { hidden: !w.hidden });
    });

    item.append(icon, label, eye);
    item.addEventListener('click', () => wall.select(w.name));
    return item;
  }));
}

function fmt(n) {
  return Number.isFinite(n) ? String(Math.round(n * 100) / 100) : '';
}

function renderInspector() {
  const w = wall.selected ? wall.window(wall.selected) : null;
  $('inspector-empty').hidden = !!w;
  $('inspector-body').hidden = !w;
  if (!w) return;

  const edit = canEdit();
  const setField = (id, value) => {
    const input = $(id);
    if (document.activeElement !== input) input.value = value;
    input.disabled = !edit;
  };

  setField('insp-name', w.name);
  // a name that's a long path: show its end, the file name
  if (document.activeElement !== $('insp-name')) $('insp-name').scrollLeft = $('insp-name').scrollWidth;
  setField('insp-x', fmt(w.x));
  setField('insp-y', fmt(w.y));
  setField('insp-w', fmt(w.w));
  setField('insp-h', fmt(w.h));
  setField('insp-zoom', fmt(w.zoom));

  $('insp-uri').textContent = w.uri === w.name ? '' : w.uri;
  $('insp-source').textContent = w.contentWidth ? `Source ${w.contentWidth} × ${w.contentHeight}` : '';
  $('insp-hide').textContent = w.hidden ? 'Show' : 'Hide';
  $('insp-fill').textContent = w.filled ? 'Restore' : 'Fill wall';
  $('insp-fill').setAttribute('aria-pressed', String(!!w.filled));
  $('insp-fill').title = w.filled ? 'Put the window back where it was' : 'Fill the wall, remembering where the window was';
  $('insp-view').textContent = wall.viewing === w.name ? 'Done' : 'Adjust view';

  for (const id of ['insp-fill', 'insp-front', 'insp-hide', 'insp-view', 'insp-reset-view', 'insp-close']) {
    $(id).disabled = !edit;
  }
}

function renderOptions() {
  const list = $('options-list');
  const edit = canEdit();

  list.replaceChildren(...Object.keys(OPTION_LABELS).filter((k) => k in state.options).map((key) => {
    const label = document.createElement('label');
    label.className = 'check';
    const box = document.createElement('input');
    box.type = 'checkbox';
    box.checked = !!state.options[key];
    box.disabled = !edit;
    box.addEventListener('change', () => call('PATCH', '/options', { [key]: box.checked }));
    label.append(box, ' ' + OPTION_LABELS[key]);
    return label;
  }));
}

// ---- actions ----

async function takeOrReleaseControl() {
  if (canEdit()) {
    await api.releaseControl();
    wall.setViewing(null);
    onSnapshot(state);
    return;
  }

  const holder = state.status.controller;
  if (holder && !confirm(`${holder.label} has control of the wall. Take it anyway?`)) return;

  try {
    await api.takeControl(!!holder);
    controlConfirmed = true;
    onSnapshot(state);
  } catch (e) {
    showError(e);
  }
}

async function openFile(uri) {
  const window = await call('POST', '/windows', { uri });
  if (window) wall.select(window.name);
}

function inspectorChange(field, parse) {
  const input = $(field.id);
  input.addEventListener('change', () => {
    const w = wall.selected && wall.window(wall.selected);
    if (!w) return;
    const value = parse(input.value);
    if (value === null) { renderInspector(); return; }
    call('PATCH', windowPath(w.name), { [field.key]: value });
  });
}

function number(min) {
  return (text) => {
    const v = parseFloat(text);
    return Number.isFinite(v) && (min === undefined || v >= min) ? v : null;
  };
}

function wireInspector() {
  inspectorChange({ id: 'insp-x', key: 'x' }, number());
  inspectorChange({ id: 'insp-y', key: 'y' }, number());
  inspectorChange({ id: 'insp-w', key: 'w' }, number(0.01));
  inspectorChange({ id: 'insp-h', key: 'h' }, number(0.01));
  inspectorChange({ id: 'insp-zoom', key: 'zoom' }, number(1));

  const name = $('insp-name');
  name.addEventListener('change', async () => {
    const old = wall.selected;
    const value = name.value.trim();
    if (!old || !value || value === old) { renderInspector(); return; }
    const result = await call('PATCH', windowPath(old), { name: value });
    if (result) wall.select(result.name);
    else renderInspector();
  });
  name.addEventListener('keydown', (e) => { if (e.key === 'Enter') name.blur(); });

  const selected = () => wall.selected && wall.window(wall.selected);

  $('insp-fill').addEventListener('click', () => {
    const w = selected();
    if (w) call('PATCH', windowPath(w.name), { filled: !w.filled });
  });
  $('insp-front').addEventListener('click', () => { const w = selected(); if (w) call('PATCH', windowPath(w.name), { front: true }); });
  $('insp-hide').addEventListener('click', () => { const w = selected(); if (w) call('PATCH', windowPath(w.name), { hidden: !w.hidden }); });
  $('insp-view').addEventListener('click', () => { const w = selected(); if (w) { wall.setViewing(wall.viewing === w.name ? null : w.name); renderInspector(); } });
  $('insp-reset-view').addEventListener('click', () => { const w = selected(); if (w) call('PATCH', windowPath(w.name), { zoom: 1, centerX: 0.5, centerY: 0.5 }); });
  $('insp-close').addEventListener('click', () => { const w = selected(); if (w) call('DELETE', windowPath(w.name)); });
}

// ---- sleeping and waking ----

function wireSleep() {
  $('sleep-button').addEventListener('click', () => call('POST', '/sleep'));

  const wake = (e) => {
    e.preventDefault();
    e.stopPropagation();
    call('POST', '/wake');
  };
  $('asleep').addEventListener('pointerdown', wake);
  document.addEventListener('keydown', (e) => {
    if (state.status.asleep && !document.querySelector('dialog[open]') && !$('login').contains(e.target)) wake(e);
  }, true);

  // while we control the wall, we decide when it sleeps: after the wall's own
  // idle timeout without input here
  for (const type of ['pointerdown', 'keydown', 'wheel']) {
    document.addEventListener(type, () => { lastActivity = Date.now(); }, { capture: true, passive: true });
  }
  setInterval(() => {
    const timeout = state.status.idleTimeout * 1000;
    if (canEdit() && !state.status.asleep && timeout > 0 && Date.now() - lastActivity > timeout) {
      lastActivity = Date.now();
      call('POST', '/sleep');
    }
  }, 5000);
}

// ---- panels, menus, dialogs ----

function wireChrome() {
  $('control-button').addEventListener('click', takeOrReleaseControl);

  const menu = $('options-menu');
  const menuButton = $('options-button');
  menuButton.addEventListener('click', (e) => {
    e.stopPropagation();
    menu.hidden = !menu.hidden;
    menuButton.setAttribute('aria-expanded', String(!menu.hidden));
  });
  document.addEventListener('click', (e) => {
    if (!menu.hidden && !menu.contains(e.target)) {
      menu.hidden = true;
      menuButton.setAttribute('aria-expanded', 'false');
    }
  });

  $('forget-token').addEventListener('click', async () => {
    await api.releaseControl();
    api.forgetToken();
    location.reload();
  });

  // phones: side panels are sheets
  for (const button of document.querySelectorAll('.tabbar [data-sheet]')) {
    button.addEventListener('click', () => {
      const panel = $(button.dataset.sheet);
      const open = !panel.classList.contains('open');
      document.querySelectorAll('.panel.open').forEach((p) => p.classList.remove('open'));
      panel.classList.toggle('open', open);
    });
  }
  for (const close of document.querySelectorAll('.sheet-close')) {
    close.addEventListener('click', () => close.closest('.panel').classList.remove('open'));
  }

  const browser = new FileBrowser(api, {
    open: openFile,
    openGrid: (root, dir) => call('POST', '/windows/directory', { root, dir }),
    error: showError,
  });
  const openBrowser = () => {
    document.querySelectorAll('.panel.open').forEach((p) => p.classList.remove('open'));
    browser.show(canEdit());
  };
  $('open-button').addEventListener('click', openBrowser);
  $('tab-open').addEventListener('click', openBrowser);

  const states = new StateDialog(api, {
    load: (file) => call('POST', '/state/load', { file }),
    save: async (file) => { if (await call('POST', '/state/save', { file })) toast(`Saved ${file}.`); },
  });
  $('clear-button').addEventListener('click', () => {
    const n = state.windows.length;
    if (n && confirm(`Close all ${n} window${n === 1 ? '' : 's'} on the wall?`)) call('DELETE', '/windows');
  });
  $('load-state-button').addEventListener('click', () => states.show('load'));
  $('save-state-button').addEventListener('click', () => states.show('save'));

  document.addEventListener('keydown', (e) => {
    if (document.querySelector('dialog[open]') || e.target.matches('input, select, textarea')) return;
    wall.keyDown(e);
  });
}

// ---- connecting ----

function showLogin(message) {
  api.disconnectEvents();
  $('app').hidden = true;
  $('login').hidden = false;
  $('login-error').hidden = !message;
  $('login-error').textContent = message || '';
  $('token-input').focus();
}

async function start() {
  try {
    config = await api.get('/config');
  } catch (e) {
    if (e.status === 401) showLogin(api.token ? "That token didn't work." : '');
    else showLogin(e.message);
    return;
  }

  $('login').hidden = true;
  $('app').hidden = false;
  $('wall-host').textContent = location.hostname;
  $('wall-size').textContent = `${config.tilesWide} × ${config.tilesHigh} tiles`;
  document.title = `${location.hostname} · DisplayCluster`;

  wall.setConfig(config);
  wall.updateHint();
  api.onSnapshot = onSnapshot;
  api.onConnection = (up) => { $('connection').hidden = up; };
  api.connectEvents();
}

// iOS suspends a home-screen app in the background and drops its event
// stream; reconnect as soon as it's back rather than waiting on the retry
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'visible' && config && api.events && api.events.readyState !== EventSource.OPEN) {
    api.connectEvents();
  }
});

// the name a home-screen app gets: the wall's host name up to the first dot
// ("rattler" for rattler.tacc.utexas.edu), or a whole IP address. The wall
// names its manifest the same way (RestServer::mountUi())
export function appName(hostname) {
  if (/^[\d.]+$/.test(hostname) || hostname.includes(':')) return hostname;
  return hostname.split('.')[0] || 'Wall';
}

function init() {
  $('app-title').setAttribute('content', appName(location.hostname));

  // a link can carry the token: #token=... (e.g. from a QR code)
  const hash = new URLSearchParams(location.hash.slice(1));
  if (hash.get('token')) {
    api.setToken(hash.get('token'), true);
    history.replaceState(null, '', location.pathname + location.search);
  }

  api.onUnauthorized = () => showLogin("That token didn't work.");

  $('login-form').addEventListener('submit', (e) => {
    e.preventDefault();
    api.setToken($('token-input').value.trim(), $('token-remember').checked);
    start();
  });

  wireChrome();
  wireInspector();
  wireSleep();
  start();
}

init();
