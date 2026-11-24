// Storyboard viewer for YouTube videos of the original game.
//
// Paste into the console of any youtube.com page (or run it through a
// browser automation tool). It reads the "storyboard" of a video - the
// small preview pictures YouTube shows while hovering over the seek bar -
// and lays them out over the page with their times, so that a whole video
// can be skimmed with a handful of screenshots instead of being played.
//
// Usage:
//   await openhogsStoryboard('VIDEO_ID', { firstSheet: 0, sheets: 6 });
//   openhogsStoryboardClose();
//
// Nothing is downloaded or stored: the pictures stay in the browser.

async function openhogsStoryboard(videoId, { firstSheet = 0, sheets = 6, level = 2, columns = 2 } = {}) {
  // The watch page carries the storyboard description in its player
  // response; fetching it from youtube.com itself needs no extra rights.
  const html = await (await fetch(`/watch?v=${videoId}`)).text();
  const match = html.match(/"playerStoryboardSpecRenderer":\{"spec":"([^"]+)"/);
  if (!match) throw new Error('no storyboard for ' + videoId);
  const spec = JSON.parse(`"${match[1]}"`).split('|');
  const lengthMatch = html.match(/"lengthSeconds":"(\d+)"/);
  const lengthSeconds = lengthMatch ? Number(lengthMatch[1]) : 0;

  // spec[0] is the URL template; entry level + 1 describes level L<level>:
  // width#height#count#columns#rows#interval(ms)#name#signature.
  // Level 2 has the largest pictures (about 160 x 90).
  const [width, height, count, gridColumns, gridRows, interval, name, signature] = spec[level + 1].split('#');
  const perSheet = gridColumns * gridRows;
  const secondsPerFrame = Number(interval) / 1000;
  const sheetCount = Math.ceil(count / perSheet);
  const url = (n) => spec[0].replace('$L', level).replace('$N', name.replace('$M', n))
    + '&sigh=' + signature;

  openhogsStoryboardClose();
  const overlay = document.createElement('div');
  overlay.id = 'openhogs-storyboard';
  Object.assign(overlay.style, {
    position: 'fixed', inset: '0', zIndex: 999999, background: '#000',
    display: 'grid', gridTemplateColumns: `repeat(${columns}, 1fr)`,
    gap: '4px', padding: '4px', overflow: 'hidden',
  });
  for (let n = firstSheet; n < Math.min(firstSheet + sheets, sheetCount); ++n) {
    const cell = document.createElement('div');
    Object.assign(cell.style, { position: 'relative', alignSelf: 'start' });
    const picture = document.createElement('img');
    picture.src = url(n);
    picture.style.width = '100%';
    picture.style.display = 'block';
    cell.appendChild(picture);
    overlay.appendChild(cell);
  }
  document.body.appendChild(overlay);
  const pictures = [...overlay.querySelectorAll('img')];
  await Promise.all(pictures.map((img) =>
    img.complete ? null : new Promise((done) => { img.onload = img.onerror = done; })));

  // One label per row of each sheet: the time of its first picture. The
  // last sheet can be shorter, so rows are counted from the loaded picture.
  pictures.forEach((picture, index) => {
    const sheet = firstSheet + index;
    const rows = Math.round(picture.naturalHeight / Number(height)) || gridRows;
    for (let row = 0; row < rows; ++row) {
      const seconds = (sheet * perSheet + row * gridColumns) * secondsPerFrame;
      const label = document.createElement('div');
      label.textContent = formatTime(seconds);
      Object.assign(label.style, {
        position: 'absolute', left: '0', top: `${(row / rows) * 100}%`,
        background: '#000c', color: '#ff0', font: '11px monospace', padding: '0 2px',
      });
      picture.parentElement.appendChild(label);
    }
  });
  return { broken: pictures.filter((p) => !p.naturalWidth).length, lengthSeconds, secondsPerFrame, picturesPerSheet: perSheet, sheetCount, size: `${width}x${height}` };
}

function openhogsStoryboardClose() {
  document.getElementById('openhogs-storyboard')?.remove();
}

function formatTime(seconds) {
  const s = Math.floor(seconds);
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  return (h ? h + ':' + String(m).padStart(2, '0') : m) + ':' + String(s % 60).padStart(2, '0');
}
