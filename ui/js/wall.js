// The wall view: draws the wall's tiles and windows as SVG, and turns mouse,
// touch and pen input into window changes.
//
// Coordinates: the API speaks tile units; the SVG's viewBox is the wall in
// pixels (mullions included), so tiles and windows draw at their true places.
//
// Gestures, matching the control window where it makes sense:
//   window body   drag to move (mouse: any window; touch: the selected one,
//                 so scrolling past a window doesn't move it)
//   corner handle drag to resize, keeping the content's aspect ratio when
//                 the wall constrains it
//   wheel / pinch scale a window about its center
//   double-click / double-tap  "adjust view" mode: drag pans the content
//                 inside the window, wheel / pinch / right-drag zooms it;
//                 double-click again, Esc, or a tap elsewhere ends it
//
// Changes show immediately (as local overrides) and are reported through
// onChange while the gesture runs and onChangeDone when it ends; the override
// is dropped once the wall's own state has caught up.

import { typeIcon, typeOfUri } from './browser.js';

const SVG_NS = 'http://www.w3.org/2000/svg';
const DRAG_THRESHOLD = 4;       // screen px before a press becomes a drag
const DOUBLE_TAP_MS = 350;
const MIN_SIZE = 0.05;          // tiles

function el(name, attrs, parent) {
  const e = document.createElementNS(SVG_NS, name);
  for (const k in attrs) e.setAttribute(k, attrs[k]);
  if (parent) parent.appendChild(e);
  return e;
}

export function baseName(name) {
  const slash = name.lastIndexOf('/');
  return slash >= 0 ? name.slice(slash + 1) : name;
}

export class WallView {
  constructor(svg, callbacks) {
    this.svg = svg;
    this.cb = callbacks;      // onSelect, onFront, onChange, onChangeDone, onClose, onHint, canEdit,
                              // thumbnail(url) -> image URL, or null while it loads
    this.config = null;
    this.windows = [];
    this.options = {};
    this.selected = null;
    this.viewing = null;      // window in "adjust view" mode
    this.overrides = new Map();
    this.pointers = new Map();
    this.gesture = null;
    this.lastTap = null;
    this.lastPointerType = 'mouse';
    this.frame = null;

    svg.addEventListener('pointerdown', (e) => this.pointerDown(e));
    svg.addEventListener('pointermove', (e) => this.pointerMove(e));
    svg.addEventListener('pointerup', (e) => this.pointerUp(e));
    svg.addEventListener('pointercancel', (e) => this.pointerUp(e, true));
    svg.addEventListener('wheel', (e) => this.wheel(e), { passive: false });
    svg.addEventListener('contextmenu', (e) => e.preventDefault());
    new ResizeObserver(() => this.render()).observe(svg);
  }

  // ---- state ----

  setConfig(config) {
    this.config = config;
    // a margin around the wall, so windows hanging off it still show, and can
    // be grabbed and brought back
    const mx = config.pixelsWide * 0.06, my = config.pixelsHigh * 0.12;
    this.margin = { x: mx, y: my };
    this.base = { x: -mx, y: -my, w: config.pixelsWide + 2 * mx, h: config.pixelsHigh + 2 * my };
    this.resetView();
  }

  // zooming the view itself (not any window): pinch or drag on empty space,
  // ctrl+wheel; double-tap empty space to reset. Only changes what this
  // screen shows, so it needs no control.
  resetView() {
    this.view = { zoom: 1, cx: this.base.x + this.base.w / 2, cy: this.base.y + this.base.h / 2 };
    this.applyView();
  }

  get viewBox() {
    const b = this.base, v = this.view;
    const w = b.w / v.zoom, h = b.h / v.zoom;
    const x = Math.min(Math.max(v.cx - w / 2, b.x), b.x + b.w - w);
    const y = Math.min(Math.max(v.cy - h / 2, b.y), b.y + b.h - h);
    return { x, y, w, h };
  }

  applyView() {
    const vb = this.viewBox;
    this.view.cx = vb.x + vb.w / 2;
    this.view.cy = vb.y + vb.h / 2;
    this.svg.setAttribute('viewBox', `${vb.x} ${vb.y} ${vb.w} ${vb.h}`);
    this.render();
  }

  // zooms the view to `zoom`, keeping wall point (wx, wy) under screen point (sx, sy)
  zoomViewAt(zoom, wx, wy, sx, sy) {
    const rect = this.svg.getBoundingClientRect();
    this.view.zoom = Math.min(Math.max(zoom, 1), 8);
    const w = this.base.w / this.view.zoom, h = this.base.h / this.view.zoom;
    this.view.cx = wx - ((sx - rect.left) / rect.width) * w + w / 2;
    this.view.cy = wy - ((sy - rect.top) / rect.height) * h + h / 2;
    this.applyView();
  }

  // screen point -> wall px, in the current view
  screenToWallPx(sx, sy) {
    const rect = this.svg.getBoundingClientRect();
    const vb = this.viewBox;
    return { x: vb.x + ((sx - rect.left) / rect.width) * vb.w, y: vb.y + ((sy - rect.top) / rect.height) * vb.h };
  }

  setState(windows, options) {
    this.windows = windows;
    this.options = options || {};

    // drop overrides the wall has caught up with
    for (const [name, o] of this.overrides) {
      if (o.settled) this.overrides.delete(name);
    }

    if (this.selected && !this.windows.some((w) => w.name === this.selected)) {
      this.select(null);
    }
    if (this.viewing && this.viewing !== this.selected) this.viewing = null;

    this.render();
  }

  window(name) {
    const w = this.windows.find((w) => w.name === name);
    if (!w) return null;
    const o = this.overrides.get(name);
    return o ? { ...w, ...o.props } : w;
  }

  select(name) {
    if (name === this.selected) return;
    this.selected = name;
    if (this.viewing !== name) this.viewing = null;
    this.cb.onSelect(name);
    this.render();
    this.updateHint();
  }

  setViewing(name) {
    this.viewing = name;
    if (name) this.select(name);
    this.render();
    this.updateHint();
  }

  // ---- units ----

  get tilePx() {
    const c = this.config;
    return { x: c.pixelsWide / c.tilesWide, y: c.pixelsHigh / c.tilesHigh };
  }

  // screen px per wall px
  get scale() {
    const rect = this.svg.getBoundingClientRect();
    return this.config && rect.width ? rect.width / this.viewBox.w : 1;
  }

  toWall(e) {
    const m = this.svg.getScreenCTM();
    if (!m) return { x: 0, y: 0 };
    const p = new DOMPoint(e.clientX, e.clientY).matrixTransform(m.inverse());
    const t = this.tilePx;
    return { x: p.x / t.x, y: p.y / t.y };   // tiles
  }

  // the wall keeps windows the shape of their content when constrainAspectRatio
  // is on (ContentWindowInterface::fixAspectRatio()); do the same here so a
  // resize previews what the wall will do. anchor: which corner stays put.
  fitAspect(win, anchor) {
    const c = this.config;
    if (!this.options.constrainAspectRatio || !win.contentWidth || !win.contentHeight) return win;

    const aspect = (win.contentWidth / win.contentHeight) / (c.pixelsWide / c.pixelsHigh);
    let wn = win.w / c.tilesWide;
    let hn = win.h / c.tilesHigh;
    if (aspect > wn / hn) hn = wn / aspect;
    else wn = hn * aspect;

    const w = wn * c.tilesWide;
    const h = hn * c.tilesHigh;
    const out = { ...win, w, h };
    if (anchor && anchor.includes('e')) out.x = win.x + win.w - w;   // right edge fixed
    if (anchor && anchor.includes('s')) out.y = win.y + win.h - h;   // bottom edge fixed
    return out;
  }

  clampCenter(zoom, cx, cy) {
    const half = 0.5 / zoom;
    return {
      centerX: Math.min(Math.max(cx, half), 1 - half),
      centerY: Math.min(Math.max(cy, half), 1 - half),
    };
  }

  // ---- drawing ----

  render() {
    if (this.frame) return;
    this.frame = requestAnimationFrame(() => { this.frame = null; this.draw(); });
  }

  draw() {
    const svg = this.svg;
    const c = this.config;
    if (!c) return;

    svg.replaceChildren();
    svg.classList.toggle('read-only', !this.cb.canEdit());

    const s = this.scale;
    const t = this.tilePx;

    // the wall as it looks switched off: a dark bezel, dark screens
    const bezel = Math.max(c.mullionWidth, c.mullionHeight, 6 / s);
    el('rect', { class: 'bezel', x: -bezel, y: -bezel, width: c.pixelsWide + 2 * bezel, height: c.pixelsHigh + 2 * bezel, rx: bezel / 2 }, svg);

    for (let i = 0; i < c.tilesWide; i++) {
      for (let j = 0; j < c.tilesHigh; j++) {
        el('rect', {
          class: 'tile',
          x: i * (c.screenWidth + c.mullionWidth), y: j * (c.screenHeight + c.mullionHeight),
          width: c.screenWidth, height: c.screenHeight,
        }, svg);
      }
    }

    const fontPx = 11 / s;
    const touch = this.lastPointerType !== 'mouse';
    const handleVisual = 10 / s;
    const handleHit = (touch ? 44 : 18) / s;

    const sorted = [...this.windows].sort((a, b) => a.z - b.z);

    for (const base of sorted) {
      const w = this.window(base.name);
      const x = w.x * t.x, y = w.y * t.y, width = w.w * t.x, height = w.h * t.y;
      const selected = w.name === this.selected;
      const viewing = w.name === this.viewing;

      const g = el('g', {
        class: 'window' + (selected ? ' selected' : '') + (viewing ? ' viewing' : '') + (w.hidden ? ' hidden-window' : ''),
        'data-name': w.name,
      }, svg);

      if (selected) {
        const r = 3 / s;
        el('rect', { class: 'ring', x: x - r, y: y - r, width: width + 2 * r, height: height + 2 * r, rx: r }, g);
      }

      // the panel, which also takes the pointer for the whole window
      el('rect', { class: 'body', x, y, width, height, 'data-name': w.name, 'data-role': 'body' }, g);

      // the content: its thumbnail, cropped to what the window shows when
      // zoomed in (the wall stretches content to the window, so no aspect
      // is kept here either); an icon until it's loaded, or if there's none
      const src = w.thumbnail ? this.cb.thumbnail(w.thumbnail) : null;
      if (src) {
        const half = 0.5 / w.zoom;
        const crop = el('svg', { x, y, width, height, viewBox: `${w.centerX - half} ${w.centerY - half} ${1 / w.zoom} ${1 / w.zoom}`,
                                 preserveAspectRatio: 'none', 'pointer-events': 'none' }, g);
        el('image', { href: src, x: 0, y: 0, width: 1, height: 1, preserveAspectRatio: 'none' }, crop);
      } else {
        // centered in the space above the label chip
        const above = height - fontPx * 1.6 - 10 / s;
        const iconPx = Math.min(width, above) * 0.4;
        if (iconPx * s > 10) {
          const icon = el('text', { class: 'placeholder', x: x + width / 2, y: y + above / 2, 'font-size': iconPx,
                                    'text-anchor': 'middle', 'dominant-baseline': 'central' }, g);
          icon.textContent = typeIcon(typeOfUri(w.uri));
        }
      }

      el('rect', { class: 'edge', x, y, width, height }, g);

      // label: a chip at the bottom left, clipped to the window
      const clip = el('svg', { x, y, width, height, overflow: 'hidden', 'pointer-events': 'none' }, g);
      const pad = 5 / s;
      const text = el('text', { class: 'label', x: 2 * pad, y: height - 2 * pad - fontPx * 0.3, 'font-size': fontPx }, clip);
      text.textContent = (typeOfUri(w.uri) === 'movie' ? '▶ ' : '') + baseName(w.name) + (w.hidden ? ' · hidden' : '');
      const textWidth = text.getComputedTextLength();
      clip.insertBefore(el('rect', { class: 'chip', x: pad, y: height - pad - fontPx * 1.6, width: textWidth + 2 * pad,
                                     height: fontPx * 1.6, rx: 3 / s }), text);

      if (viewing) {
        // overview of what part of the content the window shows
        // in the bottom-right of whatever part of the window is in view
        const m = this.margin;
        const vx0 = Math.max(x, -m.x), vy0 = Math.max(y, -m.y);
        const vx1 = Math.min(x + width, c.pixelsWide + m.x), vy1 = Math.min(y + height, c.pixelsHigh + m.y);
        const ow = Math.max(0, Math.min(vx1 - vx0, vy1 - vy0) * 0.3), oh = ow;
        const ox = vx1 - ow - 6 / s, oy = vy1 - oh - 6 / s;
        el('rect', { class: 'viewport', x: ox, y: oy, width: ow, height: oh, 'stroke-dasharray': 'none' }, g);
        const vw = ow / w.zoom, vh = oh / w.zoom;
        el('rect', { class: 'viewport', x: ox + (w.centerX * ow) - vw / 2, y: oy + (w.centerY * oh) - vh / 2, width: vw, height: vh }, g);
        const zt = el('text', { class: 'zoom-label', x: ox, y: oy - 4 / s, 'font-size': fontPx }, g);
        zt.textContent = `${w.zoom.toFixed(1)}×`;
      }

      if (selected && !viewing && this.cb.canEdit()) {
        for (const [role, hx, hy] of [['nw', x, y], ['ne', x + width, y], ['sw', x, y + height], ['se', x + width, y + height]]) {
          el('rect', { class: 'handle', x: hx - handleVisual / 2, y: hy - handleVisual / 2, width: handleVisual, height: handleVisual, 'pointer-events': 'none' }, g);
          el('rect', { class: 'handle-hit', x: hx - handleHit / 2, y: hy - handleHit / 2, width: handleHit, height: handleHit,
                       'data-name': w.name, 'data-role': role, style: `cursor: ${role === 'nw' || role === 'se' ? 'nwse' : 'nesw'}-resize` }, g);
        }

        // close button, inside the top-right corner (clear of the handle)
        const b = (touch ? 28 : 18) / s;
        const bx = x + width - b - handleHit / 2, by = y + 4 / s;
        if (width > b * 3 && height > b * 2) {
          const close = el('g', { class: 'close-button', 'data-name': w.name, 'data-role': 'close', style: 'cursor: pointer' }, g);
          el('rect', { x: bx, y: by, width: b, height: b, rx: 3 / s, 'data-name': w.name, 'data-role': 'close' }, close);
          const p = b * 0.3;
          el('path', { d: `M${bx + p},${by + p} L${bx + b - p},${by + b - p} M${bx + b - p},${by + p} L${bx + p},${by + b - p}`, 'pointer-events': 'none' }, close);
        }
      }
    }
  }

  updateHint() {
    let hint;
    const touch = this.lastPointerType !== 'mouse';
    if (!this.cb.canEdit()) hint = 'Take control to arrange windows.';
    else if (this.viewing) hint = touch ? 'Drag to pan · pinch to zoom · double-tap when done'
                                        : 'Drag to pan · scroll or right-drag to zoom · double-click or Esc when done';
    else if (touch) hint = 'Tap to select, then drag to move · drag a corner to resize · pinch to scale · double-tap to pan and zoom inside';
    else hint = 'Drag to move · drag a corner to resize · scroll to scale · double-click to pan and zoom inside';
    this.cb.onHint(hint);
  }

  // ---- changing windows ----

  // shows props on the named window at once, and reports them
  apply(name, props, done) {
    const o = this.overrides.get(name) || { props: {}, settled: false };
    Object.assign(o.props, props);
    o.settled = false;
    this.overrides.set(name, o);
    this.render();
    this.cb.onChange(name, props);
    if (done) this.finish(name);
  }

  finish(name) {
    Promise.resolve(this.cb.onChangeDone(name)).finally(() => {
      const o = this.overrides.get(name);
      // keep showing the local version until the next snapshot from the wall
      if (o) o.settled = true;
    });
  }

  scaleAbout(w, factor, cx, cy) {
    let nw = Math.max(MIN_SIZE, w.w * factor);
    let nh = Math.max(MIN_SIZE, w.h * factor);
    const f = nw / w.w;
    nh = w.h * f;
    return { x: cx - (cx - w.x) * f, y: cy - (cy - w.y) * f, w: nw, h: nh };
  }

  // ---- input ----

  targetOf(e) {
    const t = e.target.closest ? e.target.closest('[data-role]') : null;
    return t ? { name: t.getAttribute('data-name'), role: t.getAttribute('data-role') } : { name: null, role: 'empty' };
  }

  pointerDown(e) {
    if (!this.config) return;
    if (e.pointerType !== this.lastPointerType) {
      this.lastPointerType = e.pointerType;
      this.render();
      this.updateHint();
    }

    this.svg.setPointerCapture(e.pointerId);
    const p = this.toWall(e);
    this.pointers.set(e.pointerId, { ...p, sx: e.clientX, sy: e.clientY });

    // a second finger on empty space (or anywhere, without control) zooms the view
    if (this.pointers.size === 2 && this.gesture && (!this.gesture.name || !this.cb.canEdit())) {
      const [a, b] = [...this.pointers.values()];
      const mid = { x: (a.sx + b.sx) / 2, y: (a.sy + b.sy) / 2 };
      this.gesture = {
        kind: 'viewpinch', start: { ...this.view }, dist: Math.hypot(a.sx - b.sx, a.sy - b.sy) || 1,
        anchor: this.screenToWallPx(mid.x, mid.y), moved: true,
      };
      return;
    }

    // a second finger on a window turns the gesture into a pinch
    if (this.pointers.size === 2 && this.gesture && this.gesture.name && this.cb.canEdit()) {
      const w = this.window(this.gesture.name);
      const [a, b] = [...this.pointers.values()];
      this.gesture = {
        kind: 'pinch', name: w.name, start: { ...w },
        dist: Math.hypot(a.x - b.x, a.y - b.y) || 1e-6,
        mid: { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 },
        moved: true,
      };
      return;
    }
    if (this.pointers.size > 1) return;

    const target = this.targetOf(e);
    const edit = this.cb.canEdit();
    const w = target.name ? this.window(target.name) : null;

    this.gesture = { kind: 'none', name: target.name, role: target.role, start: w ? { ...w } : null, from: p,
                     sx: e.clientX, sy: e.clientY, moved: false, button: e.button, pointerType: e.pointerType };

    if (!w) {
      if (this.view.zoom > 1) this.gesture.kind = 'viewpan';
      return;
    }
    if (!edit) return;

    if (target.role === 'close') {
      this.gesture.kind = 'close';
    } else if (['nw', 'ne', 'sw', 'se'].includes(target.role)) {
      this.gesture.kind = 'resize';
    } else if (this.viewing === w.name) {
      this.gesture.kind = e.button === 2 ? 'zoom' : 'pan';
    } else if (e.pointerType === 'mouse') {
      // a mouse press selects and raises at once, like the control window
      if (this.selected !== w.name) this.select(w.name);
      this.cb.onFront(w.name);
      this.gesture.kind = 'move';
    } else if (this.selected === w.name) {
      this.gesture.kind = 'move';
    }
  }

  pointerMove(e) {
    const tracked = this.pointers.get(e.pointerId);
    if (!tracked || !this.gesture) return;

    const p = this.toWall(e);
    Object.assign(tracked, p, { sx: e.clientX, sy: e.clientY });
    const g = this.gesture;

    if (g.kind === 'viewpinch') {
      const [a, b] = [...this.pointers.values()];
      if (!b) return;
      const mid = { x: (a.sx + b.sx) / 2, y: (a.sy + b.sy) / 2 };
      this.zoomViewAt(g.start.zoom * Math.hypot(a.sx - b.sx, a.sy - b.sy) / g.dist, g.anchor.x, g.anchor.y, mid.x, mid.y);
      return;
    }

    if (g.kind === 'pinch') {
      const [a, b] = [...this.pointers.values()];
      if (!b) return;
      const ratio = Math.hypot(a.x - b.x, a.y - b.y) / g.dist;
      const mid = { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
      const s = g.start;

      if (this.viewing === g.name) {
        const zoom = Math.max(1, s.zoom * ratio);
        this.apply(g.name, { zoom, ...this.clampCenter(zoom, s.centerX, s.centerY) });
      } else {
        const scaled = this.scaleAbout(s, ratio, g.mid.x, g.mid.y);
        scaled.x += mid.x - g.mid.x;
        scaled.y += mid.y - g.mid.y;
        this.apply(g.name, this.pick(this.fitAspect({ ...s, ...scaled }), ['x', 'y', 'w', 'h']));
      }
      return;
    }

    if (!g.moved) {
      if (Math.hypot(e.clientX - g.sx, e.clientY - g.sy) < DRAG_THRESHOLD) return;
      g.moved = true;
    }

    const s = g.start;
    const dx = p.x - g.from.x, dy = p.y - g.from.y;

    if (g.kind === 'viewpan') {
      const rect = this.svg.getBoundingClientRect();
      const vb = this.viewBox;
      this.view.cx -= (e.clientX - (g.lastSx ?? g.sx)) / rect.width * vb.w;
      this.view.cy -= (e.clientY - (g.lastSy ?? g.sy)) / rect.height * vb.h;
      g.lastSx = e.clientX;
      g.lastSy = e.clientY;
      this.applyView();
    } else if (g.kind === 'move') {
      this.apply(g.name, { x: s.x + dx, y: s.y + dy });
    } else if (g.kind === 'resize') {
      let { x, y, w, h } = s;
      if (g.role.includes('w')) { x = Math.min(s.x + dx, s.x + s.w - MIN_SIZE); w = s.x + s.w - x; }
      if (g.role.includes('e')) { w = Math.max(MIN_SIZE, s.w + dx); }
      if (g.role.includes('n')) { y = Math.min(s.y + dy, s.y + s.h - MIN_SIZE); h = s.y + s.h - y; }
      if (g.role.includes('s')) { h = Math.max(MIN_SIZE, s.h + dy); }
      // the corner opposite the handle stays put
      const anchor = (g.role.includes('n') ? 's' : 'n') + (g.role.includes('w') ? 'e' : 'w');
      const fitted = this.fitAspect({ ...s, x, y, w, h }, anchor);
      this.apply(g.name, this.pick(fitted, ['x', 'y', 'w', 'h']));
    } else if (g.kind === 'pan') {
      // the content follows the pointer
      const cx = s.centerX - dx / (s.w * s.zoom);
      const cy = s.centerY - dy / (s.h * s.zoom);
      this.apply(g.name, this.clampCenter(s.zoom, cx, cy));
    } else if (g.kind === 'zoom') {
      // right-drag, as in the control window: up zooms in
      const zoom = Math.max(1, s.zoom * (1 - dy / (this.config.tilesHigh)));
      this.apply(g.name, { zoom, ...this.clampCenter(zoom, s.centerX, s.centerY) });
    }
  }

  pointerUp(e, cancelled) {
    if (!this.pointers.has(e.pointerId)) return;
    this.pointers.delete(e.pointerId);
    const g = this.gesture;
    if (!g) return;

    if (g.kind === 'viewpinch') {
      if (this.pointers.size === 0) this.gesture = null;
      else g.kind = 'ended';
      return;
    }

    if (g.kind === 'pinch') {
      // lifting one finger of a pinch ends it; the other does nothing more
      if (this.pointers.size === 0) this.gesture = null;
      else g.kind = 'ended';
      this.finish(g.name);
      return;
    }
    if (g.kind === 'ended') {
      if (this.pointers.size === 0) this.gesture = null;
      return;
    }

    this.gesture = null;
    if (cancelled) {
      if (g.moved && g.name) this.finish(g.name);
      return;
    }

    if (g.moved && ['move', 'resize', 'pan', 'zoom'].includes(g.kind)) {
      this.finish(g.name);
      return;
    }
    if (g.moved) return;

    // a tap or click
    if (g.kind === 'close') {
      this.cb.onClose(g.name);
      return;
    }

    const now = Date.now();
    const double = this.lastTap && this.lastTap.name === g.name && now - this.lastTap.time < DOUBLE_TAP_MS &&
                   Math.hypot(e.clientX - this.lastTap.x, e.clientY - this.lastTap.y) < 24;
    this.lastTap = { name: g.name, time: now, x: e.clientX, y: e.clientY };

    if (!g.name) {
      if (double) {
        this.lastTap = null;
        this.resetView();
      }
      this.setViewing(null);
      this.select(null);
      return;
    }

    if (double && this.cb.canEdit()) {
      this.lastTap = null;
      this.setViewing(this.viewing === g.name ? null : g.name);
      return;
    }

    if (this.selected !== g.name) {
      this.select(g.name);
      if (g.pointerType !== 'mouse' && this.cb.canEdit()) this.cb.onFront(g.name);
    }
  }

  wheel(e) {
    if (!this.config) return;

    // ctrl+wheel (also what a trackpad pinch sends) zooms the view
    if (e.ctrlKey) {
      e.preventDefault();
      const at = this.screenToWallPx(e.clientX, e.clientY);
      this.zoomViewAt(this.view.zoom * (1 - e.deltaY / 300), at.x, at.y, e.clientX, e.clientY);
      return;
    }

    if (!this.cb.canEdit()) return;
    const target = this.targetOf(e);
    if (!target.name) return;
    e.preventDefault();

    const w = this.window(target.name);
    const delta = e.deltaY * (e.deltaMode === 1 ? 33 : e.deltaMode === 2 ? 400 : 1);

    if (this.viewing === w.name) {
      const zoom = Math.max(1, w.zoom * (1 - delta / 720));
      this.apply(w.name, { zoom, ...this.clampCenter(zoom, w.centerX, w.centerY) });
    } else {
      const scaled = this.scaleAbout(w, 1 - delta / 1200, w.x + w.w / 2, w.y + w.h / 2);
      this.apply(w.name, this.pick(this.fitAspect({ ...w, ...scaled }), ['x', 'y', 'w', 'h']));
    }

    // a wheel has no "up": finish once it's been still a moment
    clearTimeout(this.wheelTimer);
    this.wheelTimer = setTimeout(() => this.finish(w.name), 250);
  }

  keyDown(e) {
    if (e.key === 'Escape') {
      if (this.viewing) this.setViewing(null);
      else this.select(null);
    }
  }

  pick(obj, keys) {
    const out = {};
    for (const k of keys) out[k] = obj[k];
    return out;
  }
}
