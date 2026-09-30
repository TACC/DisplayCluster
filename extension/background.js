// The service worker: starts and stops streams, which run in an offscreen
// document (a service worker can't capture a tab or encode frames), and shows
// each tab's streaming state on the toolbar button. The state lives in
// chrome.storage.session, since this worker is stopped whenever it's idle.

import { loadSettings, isConfigured, fitName, stateKey } from './settings.js';

const OFFSCREEN_URL = 'offscreen.html';

let creatingOffscreen = null;

async function hasOffscreen() {
  const contexts = await chrome.runtime.getContexts({ contextTypes: ['OFFSCREEN_DOCUMENT'] });
  return contexts.length > 0;
}

async function ensureOffscreen() {
  if (await hasOffscreen()) return;
  creatingOffscreen ??= chrome.offscreen.createDocument({
    url: OFFSCREEN_URL,
    reasons: ['USER_MEDIA'],
    justification: 'Captures the tab being streamed to the wall and encodes its frames.',
  }).finally(() => { creatingOffscreen = null; });
  await creatingOffscreen;
}

async function getState(tabId) {
  const key = stateKey(tabId);
  return (await chrome.storage.session.get(key))[key] ?? null;
}

async function anyActive() {
  const all = await chrome.storage.session.get(null);
  return Object.entries(all).some(([key, state]) => key.startsWith('tab-') && state.state !== 'error');
}

async function setState(tabId, state) {
  const key = stateKey(tabId);
  if (state) {
    await chrome.storage.session.set({ [key]: state });
  } else {
    await chrome.storage.session.remove(key);
  }
  await showState(tabId, state);
}

// the toolbar button, for one tab
async function showState(tabId, state) {
  const badge = !state ? '' : state.state === 'error' ? '!' : 'ON';
  const color = state?.state === 'error' ? '#b3302f' : '#2f6fd0';
  const title = !state ? 'Stream this tab to the wall'
    : state.state === 'error' ? `Not streaming: ${state.message}`
    : `Streaming to the wall as "${state.name}"`;
  try {
    await chrome.action.setBadgeText({ tabId, text: badge });
    await chrome.action.setBadgeBackgroundColor({ tabId, color });
    await chrome.action.setTitle({ tabId, title });
  } catch {
    // the tab has closed
  }
}

async function startStreaming(tabId, streamId, name) {
  const settings = await loadSettings();
  if (!isConfigured(settings)) {
    await setState(tabId, { state: 'error', name, message: 'set the wall and its token in the options first' });
    return;
  }
  name = fitName(name) || 'Browser tab';
  await setState(tabId, { state: 'starting', name, message: '' });
  const tab = await chrome.tabs.get(tabId);
  await ensureOffscreen();
  await chrome.runtime.sendMessage({ target: 'offscreen', type: 'start', tabId, streamId, name, settings,
                                     tabSize: { width: tab.width, height: tab.height } });
}

async function stopStreaming(tabId) {
  if (await hasOffscreen()) {
    await chrome.runtime.sendMessage({ target: 'offscreen', type: 'stop', tabId });
  }
  await setState(tabId, null);
}

async function toggle(tab) {
  const state = await getState(tab.id);
  if (state && state.state !== 'error') {
    await stopStreaming(tab.id);
    return;
  }
  try {
    const streamId = await chrome.tabCapture.getMediaStreamId({ targetTabId: tab.id });
    await startStreaming(tab.id, streamId, tab.title || (tab.url ? new URL(tab.url).hostname : ''));
  } catch (e) {
    await setState(tab.id, { state: 'error', name: '', message: e.message });
  }
}

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.target !== 'background') return;

  (async () => {
    switch (message.type) {
      // from the popup, which asks for the stream ID itself since it's what
      // the user invoked
      case 'start':
        await startStreaming(message.tabId, message.streamId, message.name);
        break;
      case 'stop':
        await stopStreaming(message.tabId);
        break;
      // from the offscreen document
      case 'state':
        await setState(message.tabId, message.state);
        break;
      case 'idle':
        // unless a stream has started since
        if (await hasOffscreen() && !(await anyActive())) await chrome.offscreen.closeDocument();
        break;
    }
  })().then(() => sendResponse({ ok: true }), (e) => sendResponse({ ok: false, error: e.message }));

  return true;
});

chrome.commands.onCommand.addListener(async (command, tab) => {
  if (command === 'toggle-streaming' && tab) await toggle(tab);
});

chrome.tabs.onRemoved.addListener(async (tabId) => {
  if (await getState(tabId)) await stopStreaming(tabId);
});

// the offscreen document crops frames to the tab's shape, so it needs to hear
// when a window's resize reshapes a tab being streamed
chrome.windows.onBoundsChanged.addListener(async (window) => {
  if (!(await hasOffscreen())) return;
  for (const tab of await chrome.tabs.query({ windowId: window.id })) {
    if (await getState(tab.id)) {
      await chrome.runtime.sendMessage({ target: 'offscreen', type: 'resize', tabId: tab.id,
                                         tabSize: { width: tab.width, height: tab.height } });
    }
  }
});

// a tab's badge is reset when it navigates, but its stream carries on
chrome.tabs.onUpdated.addListener(async (tabId, change) => {
  if (change.status === 'loading') {
    const state = await getState(tabId);
    if (state) await showState(tabId, state);
  }
});

// states left over from an offscreen document that's gone (the extension was
// reloaded or updated) are stale
(async () => {
  if (!(await hasOffscreen())) {
    const all = await chrome.storage.session.get(null);
    for (const key of Object.keys(all)) {
      if (key.startsWith('tab-')) await setState(Number(key.slice(4)), null);
    }
  }
})();
