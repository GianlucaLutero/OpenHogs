# Video tools

Small browser scripts used in phase 5 to study footage of the original
game on YouTube quickly, without downloading it. They run on a
youtube.com page: paste them into the browser's console (or run them
through a browser automation tool), then call the functions.

Nothing is downloaded or saved: the pictures stay in the browser. Only
the notes taken from them go into `docs/video/`.

## `storyboard.js`: skim a whole video

YouTube keeps "storyboards" for every video, the small pictures shown
while hovering over the seek bar. This script lays them out over the
page with their times:

```js
await openhogsStoryboard('oEmbrof16pw');                        // level 2: 160 x 90, 6 sheets of 25
await openhogsStoryboard('oEmbrof16pw', { level: 1, sheets: 2, columns: 1 }); // level 1: 80 x 45, 100 a sheet
openhogsStoryboardClose();
```

It works from any youtube.com page, for any video id. With level 1 one
screenshot shows about half an hour of video (one picture every 10
seconds), enough to tell menus, briefings, missions and films apart.

## `frames.js`: look at chosen moments

On a watch page, this script seeks the page's own video and draws the
frames into a grid, each labelled with its time:

```js
await openhogsFrames([60, 75, 90, 105], { columns: 2 });       // chosen moments
await openhogsFrames(openhogsRange(600, 1 / 30, 16));          // 16 consecutive frames
openhogsFramesClose();
```

Wait for any advert to finish first (the script refuses to run while one
is playing). Consecutive frames, with the HUD's angle, power and clock in
view, are what the measurements of `docs/video/` are read from.

## Notes for browser automation

- Some embedded browsers keep showing the previous picture in a
  screenshot taken right after an overlay changes: scrolling the page by
  one pixel (`window.scrollBy(0, 1)`) forces it to repaint.
- An advert can play when a watch page opens; it has to end (or be
  skipped) before `frames.js` can use the video.
