// Settings, kept in chrome.storage.local (not sync: the token stays on this
// machine), and helpers shared by the extension's pages.

export const DEFAULTS = {
  wall: '',          // e.g. http://rattler.tacc.utexas.edu:1910
  token: '',
  fps: 15,           // most frames a second sent
  quality: 0.8,      // JPEG quality, 0-1
  maxSize: 3840,     // longest side of a frame, in pixels
};

export async function loadSettings() {
  return { ...DEFAULTS, ...(await chrome.storage.local.get(Object.keys(DEFAULTS))) };
}

export function isConfigured(settings) {
  return settings.wall !== '' && settings.token !== '';
}

// the wall address as typed, tidied: a scheme if missing, no trailing slash
// or path; null if it isn't an address
export function normalizeWall(text) {
  text = text.trim();
  if (text === '') return '';
  if (!/^[a-z]+:\/\//i.test(text)) text = 'http://' + text;
  try {
    const url = new URL(text);
    if (url.protocol !== 'http:' && url.protocol !== 'https:') return null;
    // DC's port unless one was given (URL drops a scheme's default port, so
    // look at the text, not url.port)
    const explicitPort = /^[a-z]+:\/\/(\[[^\]]*\]|[^/:?#]*):\d+/i.test(text);
    return explicitPort ? url.origin : `${url.protocol}//${url.host}:1910`;
  } catch {
    return null;
  }
}

// the wall's host name up to its first dot ("rattler"), or a whole IP address
export function wallName(wall) {
  try {
    const host = new URL(wall).hostname;
    return /^[\d.]+$/.test(host) || host.includes(':') ? host : host.split('.')[0];
  } catch {
    return wall;
  }
}

// window names travel to the wall in 63 bytes; leave room for a " (2)" suffix
export function fitName(name, maxBytes = 56) {
  name = name.trim().replace(/\s+/g, ' ');
  const encoder = new TextEncoder();
  while (encoder.encode(name).length > maxBytes) name = name.slice(0, -1);
  return name.trim();
}

// the per-tab streaming state, as the background worker records it:
// { state: 'starting' | 'streaming' | 'error', name, message }
export function stateKey(tabId) {
  return `tab-${tabId}`;
}
