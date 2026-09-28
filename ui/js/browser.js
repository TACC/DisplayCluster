// The file browser (browsing the wall host's media roots) and the load/save
// state dialog.

const PAGE = 200;
const ICONS = { directory: '▸', image: '▣', movie: '▶', svg: '◇', pyramid: '▦', other: '·' };

export function typeIcon(type) {
  return ICONS[type] || ICONS.other;
}

export function typeOfUri(uri) {
  const ext = (uri.split('.').pop() || '').toLowerCase();
  if (['mov', 'avi', 'mp4', 'mkv', 'mpg', 'flv', 'wmv'].includes(ext)) return 'movie';
  if (ext === 'svg') return 'svg';
  if (ext === 'pyr') return 'pyramid';
  return 'image';
}

function formatSize(bytes) {
  if (bytes === undefined) return '';
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let i = 0;
  while (bytes >= 1024 && i < units.length - 1) { bytes /= 1024; i++; }
  return `${bytes < 10 && i > 0 ? bytes.toFixed(1) : Math.round(bytes)} ${units[i]}`;
}

export function formatDate(iso) {
  const d = new Date(iso);
  if (isNaN(d)) return '';
  const now = new Date();
  if (d.toDateString() === now.toDateString()) {
    return 'today ' + d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  }
  const opts = { month: 'short', day: 'numeric' };
  if (d.getFullYear() !== now.getFullYear()) opts.year = 'numeric';
  return d.toLocaleDateString([], opts);
}

function li(icon, name, meta) {
  const item = document.createElement('li');
  const i = document.createElement('span'); i.className = 'type'; i.textContent = icon; i.setAttribute('aria-hidden', 'true');
  const n = document.createElement('span'); n.className = 'name'; n.textContent = name; n.title = name;
  const m = document.createElement('span'); m.className = 'meta'; m.textContent = meta || '';
  item.append(i, n, m);
  return item;
}

export class FileBrowser {
  // callbacks: open(uri), openGrid(root, dir), error(e)
  constructor(api, callbacks) {
    this.api = api;
    this.cb = callbacks;
    this.dialog = document.getElementById('browser');
    this.rootsEl = document.getElementById('browser-roots');
    this.crumbsEl = document.getElementById('browser-crumbs');
    this.listEl = document.getElementById('browser-list');
    this.moreEl = document.getElementById('browser-more');
    this.sortEl = document.getElementById('browser-sort');
    this.statusEl = document.getElementById('browser-status');
    this.openEl = document.getElementById('browser-open');
    this.gridEl = document.getElementById('browser-grid');

    this.roots = [];
    this.root = null;
    this.dir = '';
    this.entries = [];
    this.total = 0;
    this.selected = null;

    this.sortEl.addEventListener('change', () => this.load());
    this.moreEl.addEventListener('click', () => this.load(true));
    this.openEl.addEventListener('click', () => this.openSelected());
    this.gridEl.addEventListener('click', () => {
      this.dialog.close();
      this.cb.openGrid(this.root, this.dir);
    });
  }

  async show(canEdit) {
    this.openEl.hidden = this.gridEl.hidden = !canEdit;
    this.dialog.showModal();

    if (!this.roots.length) {
      try {
        this.roots = await this.api.get('/media');
      } catch (e) {
        this.cb.error(e);
        return;
      }
      if (this.roots.length) this.navigate(this.roots[0].name, '');
      else this.statusEl.textContent = 'No media directories are configured on the wall.';
    }
    this.drawRoots();
  }

  drawRoots() {
    this.rootsEl.replaceChildren(...this.roots.map((r) => {
      const b = document.createElement('button');
      b.textContent = r.name;
      b.title = r.path;
      b.className = r.name === this.root ? 'current' : '';
      b.addEventListener('click', () => this.navigate(r.name, ''));
      return b;
    }));
  }

  navigate(root, dir) {
    this.root = root;
    this.dir = dir;
    this.drawRoots();
    this.drawCrumbs();
    this.load();
  }

  drawCrumbs() {
    const parts = this.dir ? this.dir.split('/') : [];
    const crumbs = [];
    const add = (label, dir) => {
      const b = document.createElement('button');
      b.textContent = label;
      b.addEventListener('click', () => this.navigate(this.root, dir));
      crumbs.push(b);
    };
    add(this.root, '');
    parts.forEach((part, i) => {
      const sep = document.createElement('span'); sep.className = 'sep'; sep.textContent = '/';
      crumbs.push(sep);
      add(part, parts.slice(0, i + 1).join('/'));
    });
    this.crumbsEl.replaceChildren(...crumbs);
  }

  async load(more) {
    const [sort, order] = this.sortEl.value.split(':');
    const offset = more ? this.entries.length : 0;
    const params = new URLSearchParams({ dir: this.dir, sort, order, offset, limit: PAGE });

    if (!more) {
      this.entries = [];
      this.select(null);
      this.listEl.replaceChildren(this.row('Loading…'));
    }

    let result;
    try {
      result = await this.api.get(`/media/${encodeURIComponent(this.root)}?${params}`);
    } catch (e) {
      this.listEl.replaceChildren(this.row(e.message));
      return;
    }

    this.entries = more ? this.entries.concat(result.entries) : result.entries;
    this.total = result.total;
    this.draw();
  }

  row(text) {
    const item = document.createElement('li');
    item.className = 'empty-row';
    item.textContent = text;
    return item;
  }

  draw() {
    const items = this.entries.map((e) => {
      const meta = e.type === 'directory' ? formatDate(e.modified) : `${formatSize(e.size)} · ${formatDate(e.modified)}`;
      const item = li(typeIcon(e.type), e.name, meta);
      if (this.selected === e) item.classList.add('selected');
      item.addEventListener('click', () => {
        if (e.type === 'directory') this.navigate(this.root, e.dir);
        else this.select(e);
      });
      item.addEventListener('dblclick', () => {
        if (e.type !== 'directory') { this.select(e); this.openSelected(); }
      });
      return item;
    });

    if (!items.length) items.push(this.row('No folders or files the wall can open here.'));
    this.listEl.replaceChildren(...items);
    this.moreEl.hidden = this.entries.length >= this.total;
    this.statusEl.textContent = this.total > this.entries.length ? `${this.entries.length} of ${this.total}` : '';
  }

  select(entry) {
    this.selected = entry;
    this.openEl.disabled = !entry || !entry.uri;
    for (const [i, item] of [...this.listEl.children].entries()) {
      item.classList.toggle('selected', this.entries[i] === entry);
    }
  }

  openSelected() {
    if (!this.selected || !this.selected.uri || this.openEl.hidden) return;
    this.dialog.close();
    this.cb.open(this.selected.uri);
  }
}

export class StateDialog {
  // callbacks: load(file), save(file)
  constructor(api, callbacks) {
    this.api = api;
    this.cb = callbacks;
    this.dialog = document.getElementById('state-dialog');
    this.titleEl = document.getElementById('state-title');
    this.saveEl = document.getElementById('state-save');
    this.nameEl = document.getElementById('state-name');
    this.listEl = document.getElementById('state-list');
    this.okEl = document.getElementById('state-ok');
    this.mode = 'load';
    this.selected = null;

    this.okEl.addEventListener('click', () => this.confirm());
    this.nameEl.addEventListener('input', () => { this.okEl.disabled = !this.nameEl.value.trim(); });
    this.nameEl.addEventListener('keydown', (e) => { if (e.key === 'Enter') this.confirm(); });
  }

  async show(mode) {
    this.mode = mode;
    this.selected = null;
    this.titleEl.textContent = mode === 'save' ? 'Save state' : 'Load state';
    this.okEl.textContent = mode === 'save' ? 'Save' : 'Load';
    this.saveEl.hidden = mode !== 'save';
    this.nameEl.value = '';
    this.okEl.disabled = true;
    this.listEl.replaceChildren();
    this.dialog.showModal();
    if (mode === 'save') this.nameEl.focus();

    let states = [];
    try {
      states = await this.api.get('/state');
    } catch (e) {
      this.listEl.append(this.row(e.message));
      return;
    }

    if (!states.length) {
      this.listEl.append(this.row(mode === 'save' ? 'No saved states yet.' : 'No saved states yet. Save one first.'));
      return;
    }

    this.listEl.replaceChildren(...states.map((s) => {
      const item = li('▤', s.file.replace(/\.dcx$/, ''), formatDate(s.modified));
      item.addEventListener('click', () => {
        this.selected = s.file;
        for (const other of this.listEl.children) other.classList.toggle('selected', other === item);
        if (mode === 'save') this.nameEl.value = s.file.replace(/\.dcx$/, '');
        this.okEl.disabled = false;
      });
      item.addEventListener('dblclick', () => { this.selected = s.file; this.confirm(); });
      return item;
    }));
  }

  row(text) {
    const item = document.createElement('li');
    item.className = 'empty-row';
    item.textContent = text;
    return item;
  }

  confirm() {
    if (this.mode === 'save') {
      const name = this.nameEl.value.trim();
      if (!name) return;
      this.dialog.close();
      this.cb.save(name);
    } else if (this.selected) {
      this.dialog.close();
      this.cb.load(this.selected);
    }
  }
}
