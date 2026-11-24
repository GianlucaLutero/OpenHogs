// Contact sheets from the video of a YouTube watch page.
//
// Run on a youtube.com/watch page (console or browser automation). It seeks
// the page's own <video> to the given times and draws each frame into a
// grid laid over the page, every frame labelled with its time, so that one
// screenshot shows many moments of the video:
//
//   await openhogsFrames([60, 61, 62, 63], { columns: 2 });  // chosen times
//   await openhogsFrames(openhogsRange(600, 1/30, 16));      // 16 frames in a row
//   openhogsFramesClose();
//
// The video of a watch page is played from a same-origin blob, so its frames
// can be drawn on a canvas. Nothing is downloaded or stored.

async function openhogsFrames(times, { columns = 4, label = true } = {}) {
  const video = document.querySelector('video');
  if (document.querySelector('.ad-showing')) throw new Error('an advert is playing');
  video.muted = true;
  video.pause();

  openhogsFramesClose();
  const sheet = document.createElement('canvas');
  sheet.id = 'openhogs-frames';
  sheet.width = window.innerWidth;
  sheet.height = window.innerHeight;
  Object.assign(sheet.style, { position: 'fixed', left: '0', top: '0', zIndex: 999999, background: '#000' });
  document.body.appendChild(sheet);

  // Cells keep the video's proportions and fill the width of the page.
  const cellWidth = sheet.width / columns;
  const cellHeight = cellWidth * video.videoHeight / video.videoWidth;
  const context = sheet.getContext('2d');
  context.font = '14px monospace';
  for (let i = 0; i < times.length; ++i) {
    await seek(video, times[i]);
    const x = (i % columns) * cellWidth;
    const y = Math.floor(i / columns) * cellHeight;
    context.drawImage(video, x, y, cellWidth - 2, cellHeight - 2);
    if (label) {
      const text = formatPreciseTime(video.currentTime);
      context.fillStyle = '#000c';
      context.fillRect(x, y, context.measureText(text).width + 6, 17);
      context.fillStyle = '#ff0';
      context.fillText(text, x + 3, y + 13);
    }
  }
  return { frames: times.length, cellWidth, cellHeight, video: `${video.videoWidth}x${video.videoHeight}` };
}

// Times from `start`, `count` of them, `step` seconds apart.
function openhogsRange(start, step, count) {
  return Array.from({ length: count }, (_, i) => start + i * step);
}

function openhogsFramesClose() {
  document.getElementById('openhogs-frames')?.remove();
}

// Seeks and waits for the frame. A part of the video that is not buffered
// yet can take a while to arrive; after 5 seconds the frame on screen is
// taken anyway (its label shows the time actually drawn).
function seek(video, seconds) {
  return new Promise((done) => {
    const timer = setTimeout(done, 5000);
    video.addEventListener('seeked', () => { clearTimeout(timer); done(); }, { once: true });
    video.currentTime = seconds;
  });
}

function formatPreciseTime(seconds) {
  const m = Math.floor(seconds / 60);
  const s = (seconds % 60).toFixed(2).padStart(5, '0');
  return Math.floor(m / 60) ? `${Math.floor(m / 60)}:${String(m % 60).padStart(2, '0')}:${s}` : `${m}:${s}`;
}
