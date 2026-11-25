# Movement and physics (block `video-physics`)

Notes from the footage indexed in [index.md](index.md) and from the PC
executable. Times are *part + time in the part*.

## What the footage shows

- On the PlayStation the computer's hogs walk before they shoot (part 3,
  1:52–2:03: the French GUSTAV walks some 8 seconds, the camera behind
  him, then fires), as the `ps1` rules already do (decision 44).
- The camera stays behind the hog in play while it walks and turns, and
  turns with it (decision 42's chase camera).
- Frame-by-frame measurements of running, turning and jumping were not
  possible with the tools at hand: the footage cannot be stepped reliably
  frame by frame from the browser, and no moment gives a known distance to
  measure against. Decisions 6 and 7 stay open for the refinement.

## What the executable says

**Movement is driven by the controls' amount, not by the animation.**
`FUN_0046b1e0` moves the hog each tick by the amount of the control held
(forward, back, scramble, swim: four input slots of the hog), then steps
the animation's frame counter (0x1000 per frame, `FUN_00473350`) by that
amount times a speed, divided by 64 for the run cycle, 36 for the wounded
one, 15 for the more wounded one, 16 for walking back and swimming, 4 for
scrambling. So the cycle changes, the distance does not.

**Hurt hogs run differently.** The run cycle is animation 0 above 25
health, 1 ("Run cycle (wounded)") from 25 down and 2 ("Run cycle (more
wounded)") from 10 down, unless a timed effect of the hog (`+0x3B0`,
`+0x3AE`) is running. OpenHogs now picks them (`game::runAnimationFor`,
decision 178).

**Jumping has a recovery.** `FUN_0046afc0` allows a jump only once a
counter of the hog, raised each tick, is past 14, and sets it on take-off
to 0, −5 (25 health or less) or −10 (10 or less). It forbids jumping while
falling, swimming, in a vehicle and from water or lava tiles. With
OpenHogs' jump (0.75 s in the air plus the landing) the counter is always
past 14 again on landing, so the rule changes nothing yet; it is noted in
decision 7 with the jump itself.

**The hogs' bodies are rigid bodies.** Landing ("Hit landscape ... turning
off jumping", `FUN_00476d30`) and tumbling (state 29, "Bouncing on
B-Hind", when the body's speed is past 20) are handled through the
engine's rigid bodies (`FUN_00409c10` family), whose parameters were not
decoded.
