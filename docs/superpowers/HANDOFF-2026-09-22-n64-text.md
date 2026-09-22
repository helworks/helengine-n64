# Handoff: Nintendo 64 authored text (2D step 3 of 3)

## What you are picking up

Text rendering works on Nintendo 64. A ROM exists that draws the FPS counter,
the console instruction panel with labels, and the rotating lit cube. The
remaining work is three open defects, a verification that could not run, and
the plan's final review.

Do not re-do the six implemented tasks. They are committed and reviewed.

## Repositories and their state

| Repo | Path | Branch | Tip when handed over |
| --- | --- | --- | --- |
| N64 platform | `C:\dev\helworks\helengine-n64` | `main` | `4fbceea` |
| Demo disc project | `C:\dev\helprojs\demodisc` | `main` | my last is `f9a3d31`, history has moved on since |
| Shared engine | `C:\dev\helworks\helengine` | `main` | moved a long way since my work; see Blocker |

All work is local commits only. Nothing was pushed. That was the standing
instruction and it still holds.

## What was built

The N64 2D pass now renders from the engine's shared 2D command list instead of
visiting drawables. `RenderCommandListBuilder2D::Build` flattens a camera's 2D
queue into clip pushes, textured quads, glyph quads and rounded rects, with
wrapping, alignment and the outline and shadow passes already resolved. The
platform contributes no layout code at all. This is the same path the Wii uses.

Commits in `helengine-n64`, oldest first:

- `b6aeff2`, `2aa01ae`, `f57705b` — `Nintendo64QuadBlitGeometry`, a pure seam
  turning one command-list quad into `rdpq_tex_blit` parameters. Sprites and
  glyphs both go through it. Host-tested, 15 cases.
- `0359b40`, `6023215` — `Nintendo64ClipRectStack`, a fixed-depth clip stack
  resolving clip commands into scissor rectangles. Host-tested, 12 cases. Its
  constructor throws on a non-positive frame size rather than clamping.
- `d883f6f`, `df234aa`, `33d23cd` — `Nintendo64RenderCommandExecutor2D` plus the
  render manager rewrite. The three drawable entry points are now fail-loud
  overrides because nothing should reach them.
- `4a3018c`, `04e7abc` — a second asset cook capability for source kind
  `font-atlas-texture` at 256 pixels, alongside the existing `texture`
  capability at 64. Also removes the stale `text2d` entry from
  `ForcedDisabledFeatures`.
- `8565523` — a trace of the command list's composition, added while debugging.
- `4fbceea` — the validation record. **Read `docs/Validation.md`, section
  "cube_test with authored text (2026-09-18)". It is the most complete account
  of what works and what does not.**

In `demodisc`: `b9d80b0` and `00dc78d` put the UI root into the N64 scene with
the two input-driven components excluded per component; `e608e35` sizes the FPS
overlay; `f9a3d31` fixes two viewport scaling omissions.

In `helengine`: `d8588dd` makes `RenderCommandListBuilder2D.EmitRoundedRect`
apply the parent scale to the rounded rect's size, mirroring `EmitSprite`. It
previously emitted the authored size unscaled, which the command list gives a
platform no way to recover.

## Verified on hardware

ROM SHA-256 `89692fe9e63879732e144a527b555e036272b8b7ed6e1ad76684c96dd3df56db`,
2031616 bytes, run in Gopher64. Read from captures directly:

- The FPS overlay renders two legible lines, `Update FPS: 60.0` and
  `Render FPS: 60.0`.
- The instruction panel carries legible `Camera` and `Light` labels beside its
  three icons.
- Both font atlases load at their authored sizes, 256x256 and 256x128, rather
  than the 64 pixels every texture took before.
- The panel fills at 86x38, the correctly scaled size, which is the evidence
  the shared engine fix reached the ROM.
- The cube rotates between captures.

Trace line worth knowing: `n64:command-list commands=87 clip=0/0 quads=3
glyphs=82 rects=2 frame=320x240`. Eighty-two glyph blits per frame.

## Open defects, in the order I would take them

**1. Intermittent RSP hang. Unresolved, most serious.**
The first ROM built in this step crashed once, immediately after
`n64:first-frame-complete`, with `RSP CRASH | rsp_queue | rspq_syncpoint_wait`,
"wait loop timed out (200 ms)", RSP halted. That wait is reached through
`rdpq_detach_wait` at end of frame in `Nintendo64Video.cpp`. It did not
reproduce in seven later runs across two builds, five of them back to back.
The leading suspect is the jump from three sprite blits to 82 glyph blits per
frame. The live alternative is the emulator itself: its stderr carried repeated
`Exhausted LinkedDeviceHost memory` and "will exceed budget" warnings during the
crashing run. One theory is already dead: the command-list trace shows zero clip
commands, so no degenerate scissor rectangle is ever issued.

**2. Two viewport fixes committed but never verified.**
`f9a3d31` in demodisc. Root cause was found by reading, not guessing:
`ViewportComponent.Update` returns before applying any scale unless
`ScalingMode == ReferenceCanvasScalingMode`, and that mode defaults to
`NoScalingMode`. The light indicator's viewport set its binding mode and its
1280x720 reference but never the scaling mode, so the reference was recorded and
never acted on; the scene label had no viewport at all while positioning itself
by subtracting from a 1280 constant. Nine other viewports in the project set the
mode. Both were latent everywhere and only visible on a 320-pixel screen. The
confirming build never ran. **Rebuild and check the capture: the word "Light"
should no longer cover a quarter of the screen, and the scene label should
appear at the top right.**

**3. The FPS millisecond readout is garbage.**
It displays `214748364.7 ms`, which is `int32.MaxValue / 10`. Some render-time
metric is never fed on this platform and saturates. The glyphs are correct; the
number is not. Start at `FPSComponent` in `helengine.core/components/2d/` and
find which timing source the N64 host leaves unset.

## Blocker as of 2026-09-18, may be stale

My last build failed because another session had advanced the shared
`helengine` checkout, and its new file broke transpilation of the engine core:

```
CPPOWN005 engine/helengine.core/assets/raw/scene/SceneOverrideScopePath.cs(50,9):
Method 'Normalize' mixes owned and borrowed non-null return values.
```

That aborted codegen, so no console platform could build. It was not mine and I
did not touch it. `helengine` has moved considerably since, so check whether it
still fails before assuming it is a problem. If it does still fail, the option I
was about to put to the user was building against a worktree pinned to a known
good engine commit rather than the shared checkout, because that checkout is
whatever the last session left there.

## How to build, exactly

Three steps. The editor build's own final step always fails; that is expected.

```powershell
# 1. Builder assembly, so the editor loads the current cook capabilities
& "C:\Program Files\dotnet\dotnet.exe" build C:\dev\helworks\helengine-n64\builder -c Debug

# 2. Editor build. Regenerates cube_test.helen as a prebuild, cooks, stages.
#    It FAILS at its last step with "cannot start process 'pwsh'". Expected:
#    PowerShell 7 is not installed on this machine. Everything before succeeds.
$env:HELENGINE_N64_REPOSITORY_ROOT='C:\dev\helworks\helengine-n64'
$env:HELENGINE_BUILD_WORKSPACE_ROOT='C:\dev\helworks\builds\helengine-n64\text\work'
& "C:\Program Files\dotnet\dotnet.exe" `
  "C:\dev\helworks\helengine\helengine.ui\helengine.editor.app\bin\Debug\net9.0-windows\helengine.editor.app.dll" `
  --project C:\dev\helprojs\demodisc\project.heproj `
  --build n64 --build-profile release `
  --output C:\dev\helworks\builds\helengine-n64\text\editor-output
```

Then the native step by hand, against the newest workspace under
`C:\dev\helworks\builds\helengine-n64\text\work\n64\`. It **must** go through
`Start-Process` with redirected streams: Windows PowerShell 5.1 turns docker's
stderr progress into a terminating error under `ErrorActionPreference = 'Stop'`,
so calling `scripts/build.ps1` directly aborts before compiling.

```powershell
Start-Process powershell.exe -Wait -NoNewWindow `
  -WorkingDirectory C:\dev\helworks\helengine-n64 `
  -RedirectStandardOutput out.log -RedirectStandardError err.log `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'C:\dev\helworks\helengine-n64\scripts\build.ps1',
    '-Mode','authored',
    '-GeneratedCoreRoot','<workspace>\generated-core',
    '-FilesystemRoot','<workspace>\builder\n64-dragonfs')
```

Host tests: `.\scripts\test-host.ps1` from the repo root, under Windows
PowerShell 5.1. Nine suites, all in Docker image `helengine-n64-build`.
Builder tests: `& "C:\Program Files\dotnet\dotnet.exe" test builder.tests`, 36
passing.

Emulator: `C:\dev\helworks\emus\n64\gopher64-windows-x86_64.exe <rom>`. The ROM
writes its trace to stdout, so redirect it. **Ask before launching an emulator
or capturing the screen** — that is a standing instruction from the user.
Capture by taking the process's `MainWindowHandle` and using
`Graphics.CopyFromScreen`; there is no navigator-service profile for this
emulator.

## Things that will waste your time if you do not know them

- PowerShell 7 does not exist on this machine. Do not hunt for `pwsh`. Both
  project scripts run fine under Windows PowerShell 5.1.
- The Bash tool routes `dotnet`, `grep`, `find` and `ls` through a shim that
  mangles arguments. Use the PowerShell tool with full executable paths, and the
  Grep and Glob tools for searching.
- `.helen` scenes are binary. Searching one for a quoted string returns zero and
  will convince you an authoring change failed. Decode the bytes instead.
- The engine pin in `helengine/user_settings/platforms.json` does **not** control
  which engine sources get compiled. Code generation reads the live checkout.
  The pin only decides whether a platform appears in the build menu, matched by
  exact string equality against the project's `requiredEngineVersion`. I got this
  wrong twice and built a user decision on it before proving otherwise; the proof
  is that a commit postdating the pin appears in the generated C++.
- The demodisc working tree carries dozens of unrelated modified files. Stage
  your own paths explicitly. Never `git add -A`, never a bare `git commit`. I
  swept up other people's work twice doing exactly that.
- Never write source-file-substring tests. The user rejects them in her own
  words and the repo already being full of them is not permission. I removed two
  such files that a subagent added; see demodisc `00dc78d`.
- The N64 profile's prebuild regenerates only `cube_test.helen`. Other modified
  scenes in that tree predate this work.

## Process artifacts

The plan is `docs/superpowers/plans/2026-09-18-n64-text.md` and the design is
`docs/superpowers/specs/2026-09-18-n64-text-design.md`, both in `helengine-n64`.

The execution ledger is
`.superpowers/sdd/2026-09-18-n64-text/progress.md`, git-ignored. It holds every
ruling I made with its reasoning and what it would cost if wrong, the per-task
review verdicts, and the deferred minor findings. **Read it before changing any
decision, because several look arbitrary without the reasoning.** The two that
matter most: the clip stack throws rather than clamping on a bad frame size,
because a silently empty clip discards every drawable and prints nothing; and
the per-corner rounding mask is knowingly ignored because the shared geometry
builder takes no per-corner parameter.

The plan's final whole-branch review was never run. That is the last process
step, and the deferred minors in the ledger are what it should triage.
