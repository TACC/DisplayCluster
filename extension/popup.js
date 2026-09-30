import { loadSettings, isConfigured, wallName, fitName, stateKey } from './settings.js';

const $ = (id) => document.getElementById(id);

const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
const settings = await loadSettings();
const key = stateKey(tab.id);

$('open-options').addEventListener('click', () => chrome.runtime.openOptionsPage());
$('options-link').addEventListener('click', (e) => { e.preventDefault(); chrome.runtime.openOptionsPage(); });

if (!isConfigured(settings)) {
  $('setup').hidden = false;
} else {
  $('controls').hidden = false;
  $('wall').textContent = wallName(settings.wall);
  $('wall').title = settings.wall;
  $('name').value = fitName(tab.title || '');

  const state = (await chrome.storage.session.get(key))[key] ?? null;
  if (state?.name) $('name').value = state.name;
  show(state);

  chrome.storage.onChanged.addListener((changes, area) => {
    if (area === 'session' && key in changes) show(changes[key].newValue ?? null);
  });

  $('start').addEventListener('click', start);
  $('name').addEventListener('keydown', (e) => { if (e.key === 'Enter' && !$('start').hidden) start(); });
  $('stop').addEventListener('click', () => {
    chrome.runtime.sendMessage({ target: 'background', type: 'stop', tabId: tab.id });
  });
}

async function start() {
  const name = fitName($('name').value) || fitName(tab.title || '') || 'Browser tab';
  $('start').disabled = true;
  try {
    // asked for here, since opening this popup is what lets the extension
    // capture this tab
    const streamId = await chrome.tabCapture.getMediaStreamId({ targetTabId: tab.id });
    await chrome.runtime.sendMessage({ target: 'background', type: 'start', tabId: tab.id, streamId, name });
  } catch (e) {
    show({ state: 'error', message: /chrome:|Chrome pages|cannot be captured/i.test(e.message)
      ? 'Chrome doesn\'t allow this page to be captured.' : e.message });
  }
}

function show(state) {
  const active = state && state.state !== 'error';
  $('start').hidden = active;
  $('start').disabled = false;
  $('stop').hidden = !active;
  $('name').disabled = active;

  const status = $('status');
  status.hidden = !state;
  if (!state) return;

  if (state.state === 'streaming') {
    status.className = 'status ok';
    status.textContent = `On the wall as “${state.name}”.`;
    $('name').value = state.name;
  } else if (state.state === 'starting') {
    status.className = 'status info';
    status.textContent = 'Connecting to the wall…';
  } else {
    status.className = 'status error';
    status.textContent = `Not streaming: ${state.message}`;
  }
}
