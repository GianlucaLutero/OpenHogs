# Video index: a full playthrough of the original

Phase 5 of the roadmap compares OpenHogs with footage of the original
game. This page is the map of that footage: which video shows which
mission, and where the recurring moments (briefing, title card, end of
the mission, newspaper, films) are. The later blocks (`video-campaign`,
`video-ui`, `video-physics`, `video-weapons`) start from here and add
their own findings in pages next to this one: [campaign.md](campaign.md),
[ui.md](ui.md).

Only notes and numbers are kept in the repository: no frames, no
recordings.

## The footage

Both videos are by **RedFlameFox** and show the same uncommented run of
the **PlayStation** version, the Italian PAL release *Hogs of War: Nati
per Soffritto* (SLES-02769) with its **Italian dubbing**: the texts are
the ones of OpenHogs' Italian pack, and the voices are the ones the
`pal-dubbing` block (phase 6) is about.

| Video | What it is |
|-------|------------|
| [Playlist *Hogs of War: Nati Per Soffritto - Gameplay in ITALIANO (PS1 COMPLETO)*](https://www.youtube.com/playlist?list=PLRFgeGC1204mAKinYUCl110cnltBQNQn7) | 20 parts, 8 h 40 min in all. **The reference for timestamps** on these pages: a time is *part number + time in that part*. |
| [*HOGS OF WAR: Nati per Soffritto - Completo in ITALIANO*](https://www.youtube.com/watch?v=SzlrZG9y4lo) | The same run in one video of 8 h 04 min, trimmed (menus and waits shortened), so its times drift from the parts': mission 2's title card is at 21:10 here, 26:41 into the playlist. |

Because it is the PlayStation version, what it shows can differ from the
PC executable OpenHogs reads (see `version-variants` and
`docs/open-decisions.md`): differences are written down as such, not
copied blindly.

## How the footage is read

Two scripts in [`tools/video`](../../tools/video/README.md) run in the
browser on youtube.com:

- `storyboard.js` lays out YouTube's own preview pictures (the ones shown
  while hovering over the seek bar): level 1 has 100 pictures of 80 × 45
  per sheet, one every 10 seconds, so a 30-minute part fits in one
  screenshot; level 2 has 25 pictures of 160 × 90, one every 5 or 10
  seconds. Used for this index.
- `frames.js` draws chosen moments of the page's own video (720p) into a
  grid: 16 moments per screenshot, or consecutive frames for
  measurements. Used to read titles and, in the later blocks, positions.

## Missions

All 26 missions are played, in the campaign's order, Boot Camp first.
Some parts hold two missions. Parts 12 and 13 are swapped in the
playlist: mission 14 was played before mission 15 (part 13 saves over
slot "13" before mission 14, and part 12 ends with the film that closes
the island).

Italian titles are read from the title cards, English ones are from the
Hogs of War Wiki, the maps from the executable's mission table
([campaign.md](../formats/campaign.md)). The turn time is the
executable's; the clock on the title cards agrees wherever it is
readable (99 for missions 1–2, 60 for 3–6, 45 for 7–17, 30 for 18–25
but 15 for 24).

| # | Part | Italian title (title card) | English title | Map | Turn |
|---|------|----------------------------|---------------|-----|------|
| 0 | 1 | Addestramento | Boot Camp | CAMP | 99 |
| 1 | 2 | Guerra: episode one | The War Foundation | ESTU | 99 |
| 2 | 3 | L'imboscata | Routine Patrol | ROAD | 99 |
| 3 | 4 | Missione bontà | Trench Warfare | TRENCH | 60 |
| 4 | 4 | Il doppiogiochista | Morning Glory! | DEVI | 60 |
| 5 | 5 | Lo sbarco dei mille | Island Invasion | RUMBLE | 60 |
| 6 | 6 | Sotto assedio | Under Siege | ZULUS | 60 |
| 7 | 6 | Blocco delle comunicazioni | Communication Breakdown | TWIN | 45 |
| 8 | 7 | Controspionaggio | The Spying Game | SNIPER | 45 |
| 9 | 7 | Il trogolo sulla collina | The Village People | MASHED | 45 |
| 10 | 8 | Operazione silenzio | Bangers 'N' Mash | GUNS | 45 |
| 11 | 9 | Salvate il soldato Mayal | Saving Private Rind | LIBERATE | 45 |
| 12 | 10 | Desert Storm | Just Deserts | OASIS | 45 |
| 13 | 11 | Accoglienza glaciale | Glacier Guns | FJORDS | 45 |
| 14 | 13 | Missione palla d'acciaio | Battle Stations | EYRIE | 45 |
| 15 | 12 | Operazione Bordeaux | Fortified Swine | BAY | 45 |
| 16 | 14 | Maiali da trincea | Over The Top | MEDIX | 45 |
| 17 | 14 | La convenzione di Ginevra | Geneva Convention | BRIDGE | 45 |
| 18 | 15 | Spie come voi | I Spy... | DESVAL | 30 |
| 19 | 15 | The Chemical Brother | Chemical Compound | SNAKE | 30 |
| 20 | 16 | Op. tallone d'Achille | Achilles Heal | EMPLACE | 30 |
| 21 | 17 | Lunga e dritta | High And Dry | SUPLINE | 30 |
| 22 | 17 | Missione: improbabile | Assassination | KEEP | 30 |
| 23 | 18 | Operazione tavola rotonda | Hero Warship | TESTER | 30 |
| 24 | 19 | Cheeseburger Hill | Hamburger Hill | FOOT | 15 |
| 25 | 20 | La battaglia finale | Well, Well, Well! | FINAL | 30 |

## Parts

Times are `minutes:seconds` into the part, rounded to the storyboard's
10 seconds where no frame was read. "End" is the MISSIONE COMPIUTA
message.

| Part | Video | Length | Moments |
|------|-------|--------|---------|
| 1 | [oEmbrof16pw](https://www.youtube.com/watch?v=oEmbrof16pw) | 15:05 | 0:00 loading, Infogrames logo; 0:40–1:30 opening films (newsreel countdown); 1:40 title screen; 1:50–4:40 front end (main menu, ONE PLAYER, team, naming, squad); ~4:50–13:20 Boot Camp (5:50 rifle sight); 13:40 films; 14:10 squad page |
| 2 | [0KgJOWmKKAo](https://www.youtube.com/watch?v=0KgJOWmKKAo) | 10:56 | 0:30 campaign map (Maialustralasia, island Capo Porcello); 0:38 briefing with its loading bar, then PREMI X PER CONTINUARE; 0:56 title card while the hogs come down by parachute; end ~9:10; debriefing ~9:30; 9:35 newspaper; 10:25 save |
| 3 | [abQX0EZS3FE](https://www.youtube.com/watch?v=abQX0EZS3FE) | 8:12 | 0:20 map; 0:30 briefing; 0:40 title card; 1:20 IL NEMICO CONTRATTACCA...; 1:40 weapon menu |
| 4 | [6qpEgsd4Z_Y](https://www.youtube.com/watch?v=6qpEgsd4Z_Y) | 28:03 | Mission 3: 0:30 map, 0:50 briefing, 1:30 title card, end ~11:10, 11:30 debriefing, 11:40 newspaper, 12:30 save. Mission 4: 13:20 map, 13:40 briefing, 13:50 title card, end ~26:50, 27:10 debriefing, 27:20 newspaper |
| 5 | [28lzGw4qBpQ](https://www.youtube.com/watch?v=28lzGw4qBpQ) | 19:41 | 0:20 save (slot "04"); 0:35 squad; 0:50 map; 1:05 briefing; 1:20 title card; end ~17:00; 17:20 debriefing; 17:30–18:30 film closing the island; 18:40 newspaper; 19:20 save |
| 6 | [lYFAQFeVKS0](https://www.youtube.com/watch?v=lYFAQFeVKS0) | 39:00 | Mission 6: 0:20 map, 0:35 briefing, 1:20 title card, end ~20:30, 20:40 debriefing, 21:20 newspaper. Mission 7: 22:30 map, 23:00 briefing, 23:30 title card, end ~37:50, 38:10 debriefing, 38:20 newspaper |
| 7 | [bbSJS0vUyzs](https://www.youtube.com/watch?v=bbSJS0vUyzs) | 35:32 | Mission 8: 0:20 briefing, 0:35 title card, end ~16:10, 16:40 debriefing, 16:50 newspaper, 17:30 save. Mission 9: 18:00 map, 18:20 title card, 18:35 REDFLAMEFOX PRONTO..., ~25:50 airship |
| 8 | [LbJDQMovGVY](https://www.youtube.com/watch?v=LbJDQMovGVY) | 29:39 | 0:20 map; 0:35 briefing; 0:50 title card; 2:05 pause menu, mission restarted; 2:20 title card again; end ~27:00; film closing the island ~27:30; newspaper ~28:30 |
| 9 | [HeCUBA68G98](https://www.youtube.com/watch?v=HeCUBA68G98) | 22:02 | 0:20 squad; 0:35 loading; 1:05 map; 1:20 briefing; 1:35 title card; end ~20:20; 20:50 debriefing; 21:00 newspaper; 21:40 save |
| 10 | [fHL1l1RMnOQ](https://www.youtube.com/watch?v=fHL1l1RMnOQ) | 26:22 | 0:20 map; 0:35 briefing; 0:50 title card; MISSIONE COMPIUTA shown at ~22:10 and again at ~25:10; 25:40 debriefing; 26:10 newspaper |
| 11 | [W8RZIiLO7Ig](https://www.youtube.com/watch?v=W8RZIiLO7Ig) | 27:27 | 0:35 briefing; 1:05 title card; 2:05 REDFLAMEFOX PRONTO...; ~15:00 airship; end ~26:40; 26:50 debriefing; 27:00 newspaper |
| 13 | [UkE1wrPRgZU](https://www.youtube.com/watch?v=UkE1wrPRgZU) | 17:53 | (played before part 12) 0:05 squad; 0:35 loading; 0:50 save (slot "13"); 1:05 map; 1:20 briefing; 3:10 title card |
| 12 | [nYswe1poHtU](https://www.youtube.com/watch?v=nYswe1poHtU) | 12:05 | 0:00 briefing; 0:06 title card while an airship brings the hogs (night map); end ~10:25; 10:50 debriefing; 10:55–11:40 film closing the island; 11:40 newspaper |
| 14 | [DWdjR5QTL1I](https://www.youtube.com/watch?v=DWdjR5QTL1I) | 40:49 | Mission 16: 0:20 map (Lardopolis), 0:35 briefing, 1:05 title card, end ~27:10, 27:50 newspaper, 28:20–29:40 save. Mission 17: 30:00 map, 30:15 briefing, 30:45 title card |
| 15 | [r3LyfIUdFSI](https://www.youtube.com/watch?v=r3LyfIUdFSI) | 55:50 | Mission 18: 0:05 briefing, 0:20 title card, end ~28:40, ~29:00 newspaper. Mission 19: 30:30 map, 30:45 briefing, 31:00 title card, end ~55:00, 55:20 newspaper |
| 16 | [-iQaljuueVI](https://www.youtube.com/watch?v=-iQaljuueVI) | 37:20 | 0:20 map; 0:35 briefing; 0:50 title card; end ~34:50; 35:10–35:50 film closing the island; 36:00 newspaper |
| 17 | [FLpEQgGrT90](https://www.youtube.com/watch?v=FLpEQgGrT90) | 57:52 | Mission 21: 0:50 save (slot "20"), 1:20 map, 1:35 briefing, 1:50 title card, end ~29:00, ~29:20 newspaper. Mission 22: 30:10 briefing, 30:25 title card, 30:40 weapon menu (ATTACCO AEREO X1), 30:55 airship, end ~56:40 |
| 18 | [8nq4TxBVPeA](https://www.youtube.com/watch?v=8nq4TxBVPeA) | 11:24 | 0:50 map; 1:05 briefing; 1:20 title card |
| 19 | [WkaerMRL_bw](https://www.youtube.com/watch?v=WkaerMRL_bw) | 12:26 | 0:20 save, overwrite question; 0:50 map; 1:05 briefing; 1:35 title card |
| 20 | [4REYW6UAHCQ](https://www.youtube.com/watch?v=4REYW6UAHCQ) | 12:46 | 1:00 briefing; 1:30 title card; airships throughout; end ~10:25; ~10:35 debriefing; 10:50–12:30 final films; 12:35 last newspaper |

## First observations

Noted while indexing, to be checked in the block named:

- **Campaign** (`video-campaign`): the campaign is linear, as in
  OpenHogs; the save slot is named after the missions done ("04" before
  mission 5, "13" before 14, "20" before 21). Every mission opens on the
  campaign map with the island, then the briefing (a loading bar runs
  under it before PREMI X PER CONTINUARE). Mission 12 shows MISSIONE
  COMPIUTA twice.
- **Mission start** (`video-campaign`, `video-ui`): the title card
  MISSIONE *nn* : *TITLE* is written over the map while the squad comes
  down by parachute (an airship brings it in mission 15).
- **Films** (`video-campaign`): after missions 5, 10, 15 and 20 a film
  closes the island before the newspaper; mission 25 ends with the final
  films and a last newspaper. A newspaper follows every mission.
- **Interface** (`video-ui`): radar bottom left, clock bottom right,
  angle gauge top right; the label over each hog (name or rank, heart and
  health); messages IL NEMICO CONTRATTACCA..., *NAME* PRONTO..., the
  crate box at the top (GRANATA MULT X1); the pause menu (GIOCO IN
  PAUSA).
- **Weapons** (`video-weapons`): rifle sight in almost every mission;
  airships of the air strike (missions 9, 13, 22, 25).
