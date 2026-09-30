import { loadSettings, normalizeWall, wallName } from './settings.js';

const $ = (id) => document.getElementById(id);

const settings = await loadSettings();
$('wall').value = settings.wall;
$('token').value = settings.token;
$('fps').value = String(settings.fps);
$('maxSize').value = String(settings.maxSize);
$('quality').value = String(Math.round(settings.quality * 100));
showQuality();

$('quality').addEventListener('input', showQuality);

function showQuality() {
  $('quality-value').textContent = $('quality').value + '%';
}

function status(kind, text) {
  $('status').hidden = false;
  $('status').className = `status ${kind}`;
  $('status').textContent = text;
}

// the form's settings, or null (having said why) if they aren't usable
function readForm() {
  const wall = normalizeWall($('wall').value);
  if (!wall) {
    status('error', 'That isn\'t a wall address. Give its host name, like rattler.tacc.utexas.edu.');
    return null;
  }
  return {
    wall,
    token: $('token').value.trim(),
    fps: Number($('fps').value),
    maxSize: Number($('maxSize').value),
    quality: Number($('quality').value) / 100,
  };
}

// lets the extension ask the wall's API over HTTP, which Test and explaining a
// failed stream need; streaming itself doesn't. Must run in a click handler.
async function requestAccess(wall) {
  try {
    return await chrome.permissions.request({ origins: [`${wall}/*`] });
  } catch {
    return false;
  }
}

$('form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const values = readForm();
  if (!values) return;
  const access = values.wall ? requestAccess(values.wall) : Promise.resolve(true);
  await chrome.storage.local.set(values);
  $('wall').value = values.wall;
  status('ok', values.wall && values.token ? `Saved. Streams go to ${wallName(values.wall)}.` : 'Saved.');
  await access;
});

$('test').addEventListener('click', async () => {
  const values = readForm();
  if (!values) return;
  if (!values.wall || !values.token) {
    status('error', 'Give the wall\'s address and token first.');
    return;
  }
  if (!(await requestAccess(values.wall))) {
    status('error', 'The extension needs your permission to contact the wall to test it.');
    return;
  }

  status('info', `Contacting ${values.wall}…`);
  try {
    const response = await fetch(`${values.wall}/status`, {
      headers: { Authorization: `Bearer ${values.token}` },
      signal: AbortSignal.timeout(8000),
    });
    if (response.status === 401) {
      status('error', 'The wall answered, but didn\'t accept the token.');
    } else if (!response.ok) {
      status('error', `The wall answered with an error (HTTP ${response.status}).`);
    } else {
      const body = await response.json();
      const asleep = body.asleep ? ' It\'s asleep; a stream will wake it.' : '';
      status('ok', `Connected to ${wallName(values.wall)}, and the token works.${asleep}`);
    }
  } catch (err) {
    status('error', err.name === 'TimeoutError'
      ? `No answer from ${values.wall}. Check the address, that you're on a network that can reach it, and that its firewall allows port 1910.`
      : `Couldn't reach ${values.wall}. Check the address, that you're on a network that can reach it, and that its firewall allows port 1910.`);
  }
});
