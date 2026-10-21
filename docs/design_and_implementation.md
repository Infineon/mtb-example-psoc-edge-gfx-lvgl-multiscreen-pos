[Click here](../README.md) to view the README.

## Design and implementation

This project supports four displays :

   **[Waveshare 4.3-inch Raspberry Pi DSI LCD display](https://www.waveshare.com/4.3inch-DSI-LCD.htm):** The LCD houses a Chipone ICN6211 display controller and uses the MIPI DSI interface. This display is supported by default in the code example for all BSPs except KIT_PSE84_HMI

   **[Waveshare 7-inch Raspberry Pi DSI LCD C display](https://www.waveshare.com/7inch-dsi-lcd-c.htm)**: The LCD houses a Chipone ICN6211 display controller and uses the MIPI DSI interface

   **[10.1 inch WF101JTYAHMNB0](https://www.winstar.com.tw/products/tft-lcd/ips-tft/ips-touch.html):** The TFT LCD houses a [EK79007AD3](https://www.crystalfontz.com/controllers/Fitipower/EK79007AD3/505/) display controller and uses the MIPI DSI interface

   **[ST7701S 4-inch MIPI DSI display driver](https://www.rocktech.com.hk/lcd-product/rk040hf001):** The TFT LCD houses a [ST7701S](https://www.crystalfontz.com/controllers/Sitronix/ST7701/495) display controller and uses the MIPI DSI interface. This display by default equipped with the KIT_PSE84_HMI

The design of this application is minimalistic to get started with code examples on PSOC&trade; Edge MCU devices. All PSOC&trade; Edge E84 MCU applications have a dual-CPU three-project structure to develop code for the CM33 and CM55 cores. The CM33 core has two separate projects for the secure processing environment (SPE) and non-secure processing environment (NSPE). A project folder consists of various subfolders, each denoting a specific aspect of the project. The three project folders are as follows:

**Table 1. Application projects**

Project | Description
--------|------------------------
*proj_cm33_s* | Project for CM33 secure processing environment (SPE)
*proj_cm33_ns* | Project for CM33 non-secure processing environment (NSPE)
*proj_cm55* | CM55 project

<br>

In this code example, at device reset, the secure boot process starts from the ROM boot with the secure enclave (SE) as the root of trust (RoT). From the secure enclave, the boot flow is passed on to the system CPU subsystem where the secure CM33 application starts. After all necessary secure configurations, the flow is passed on to the non-secure CM33 application. Resource initialization for this example is performed by this CM33 non-secure project. It configures the system clocks, pins, clock to peripheral connections, and other platform resources. It then enables the CM55 core using the `Cy_SysEnableCM55()` function, starts FreeRTOS, and leaves its empty application task suspended.

The CM55 application drives the LCD and renders the image using the PSOC&trade; Edge graphics subsystem, which houses an independent 2.5D GPU, a display controller (DC), and a MIPI DSI host controller with a MIPI D-PHY physical layer interface.

`cm55_gfx_task` initializes the graphics subsystem, the panel selected by
`CONFIG_DISPLAY` in *common.mk*, and LVGL's display and touch drivers. It then
calls `ui_theme_init()`, `ui_nav_init()`, and `app_buttons_init()` in that order:
shared styles must exist before pages are created. The task runs
`lv_timer_handler()` to process input, animations, and rendering.

The **Settings** page can enable LVGL's FPS and CPU-usage monitor, which is
hidden at boot.

## The multi-screen state machine

### What it demonstrates

Paths below are relative to *proj_cm55/*.

**Table 2. Features and implementation**

Feature | Where to look
--------|--------------
Navigation host that keeps every page under application control | *source/ui/ui_nav_host.c*
Spring-physics easing as a custom `lv_anim` path | *source/ui/ui_spring_anim.c*
Page lifecycle events traced over UART | *source/ui/ui_lifecycle.c*
Shared header that survives every transition | *source/ui/ui_header.c*
Page registry, per-visit focus callbacks, and shared sale state | *source/ui/ui_pages.c*
Run-time colour schemes and dark/light theming of an already-built UI | *source/ui/ui_theme.c*
Resolution-independent layout across four panels | *source/ui/ui_layout.c*
Build-time spring, theme, button, and layout settings | *include/ui_config.h*
Debounced physical-button handling in a FreeRTOS task | *source/app_buttons.c*

### Why a navigation host is used

LVGL's `lv_screen_load_anim()` provides screen transitions but does not accept a
custom easing path. To use spring easing, this example keeps one LVGL screen as
a **navigation host**, with a persistent header and page containers underneath.
The host uses a flex column to place the header above the viewport:

```
host screen  (the one and only lv_screen_active(), flex column)
  +-- shared header      fixed height, created once, never destroyed
  +-- viewport           flex grow 1, clips its children
        +-- page[0]      LV_PCT(100) x LV_PCT(100), positioned absolutely
        +-- page[1]      ...
```

The viewport fills the space below the header. Each page fills the viewport
and owns its labels, buttons, and other widgets. During forward navigation,
the outgoing page slides left and the incoming page slides in from the right;
back navigation reverses this. The viewport clips anything outside its bounds.

The host animates both page containers with the same spring path and rejects
new navigation requests until the transition finishes. It also emits page
lifecycle events. The header stays outside the page containers, so its title
can change without recreating it or restarting its clock.

### Spring easing

*ui_spring_anim.c* models a damped spring. Tune it in *include/ui_config.h*:

- `UI_SPRING_STIFFNESS`: higher values make the motion faster for the same mass.
- `UI_SPRING_DAMPING`: controls how much the page bounces before settling.
- `UI_SPRING_MASS`: higher values make the motion slower for the same stiffness.
- `UI_SPRING_DURATION_MS`: ends the animation at the exact target position.

The damping ratio describes the response: below 1 gives overshoot, 1 is
critically damped, and above 1 settles without overshoot. Start with a preset:

**Table 3. Spring presets** (these are the same values listed in the comments of *ui_config.h*)

Feel | Stiffness | Damping | Mass | Damping ratio
-----|-----------|---------|------|---------------
Snappy | 260 | 26 | 1.0 | ~0.81
Default | 180 | 22 | 1.0 | ~0.82
Bouncy | 180 | 12 | 1.0 | ~0.45
No bounce | 180 | 27 | 1.0 | ~1.0

`UI_SPRING_DURATION_MS` is independent of the preset: it is the window the
motion is allowed to occupy, not part of the spring itself.

### Page lifecycle

An optional `prepare_cb` refreshes incoming page content before it becomes
current or slides into view. Payment and Receipt reuse their refresh callbacks
here so cached pages show the current sale throughout the transition. Their
existing focus callbacks still run on arrival, preserving the receipt approval
animation timing and lifecycle events.

The navigation framework defines five page events, printed to the UART as
`[LC] <page>: <event>`:

Event | Meaning
------|--------
`CREATE` | The container was created and its widget tree built. Happens once, on first visit
`FOCUS` | The page has finished arriving and is now the interactive one
`DEFOCUS` | A transition away from this page has started
`UNLOADED` | The transition finished; the page is parked off-screen and hidden
`DESTROY` | The container was deleted

Pages are created on first visit and kept for reuse. Scroll positions and
selected tender therefore survive navigation. `DESTROY` does not normally
appear because this example does not evict pages; adding eviction requires
coordinating deletion with navigation.

Callbacks are registered in *ui_pages.c*. `create_cb` builds widgets once,
`prepare_cb` refreshes content before entry, and `focus_cb` runs on arrival.
`defocus_cb` runs when leaving; Settings uses it to close its theme list.

Set `UI_LOG_LIFECYCLE` to `0` in *include/ui_config.h* to compile the trace out; the calls become empty.

### The sale flow

The main flow is **New sale > Basket > Payment > Receipt > New sale**.
Ad hoc and Settings are side branches reached by touch. `ui_nav_next()` and
`ui_nav_prev()` cycle the first `UI_PAGE_FLOW_COUNT` entries in `ui_page_id_t`.

*ui_pages.c* holds the shared sale amount and whether it came from Basket or
Ad hoc. Both routes feed the same Payment and Receipt pages through
`ui_sale_arm()`, `ui_sale_is_adhoc()`, and `ui_sale_amount_text()`.

The basket items are fixed demo data standing in for goods scanned at the till,
so there is no on-screen item entry; the subtotal, tax and total are computed
from them. The receipt replays its approval check on every visit.

### Adapting the framework

To add a page, add its id to `ui_page_id_t` in *include/ui_nav.h*, implement
its `page_*_create()` function under *source/pages/*, and add its title, caption,
create callback and optional prepare, focus, and defocus callbacks to `page_registry` in
*source/ui/ui_pages.c*. Build page content with the shared `ui_page_*()` helpers
and `ui_style_*()` styles so it inherits responsive layout and run-time themes.

Insert new carousel page IDs before `UI_PAGE_ADHOC` and new side-branch IDs
before `UI_PAGE_COUNT`. `UI_PAGE_FLOW_COUNT` is defined as `UI_PAGE_ADHOC`, so
it automatically includes new carousel pages while excluding side branches.
Route to the page with `ui_nav_go_to()` or a shared navigation footer.
Use `prepare_cb` for values that must be current during the incoming slide,
`focus_cb` for arrival effects, and `defocus_cb` for cleanup when leaving.
To replace the POS example entirely, keep the
navigation, header, lifecycle, theme and layout modules, then replace the page
files and registry entries with application-specific ones.

### Run-time theming

*ui_theme.c* owns the four colour schemes (Default, Classic, Sunset and Teal),
their light and dark variants, and the shared styles. The UI starts in the light
Default scheme. Use **Settings** to select a scheme and SW4 or the **Settings**
switch to change mode.

Pages use shared `ui_style_*()` styles rather than fixed colours. When the theme
changes, those styles and LVGL's default theme are updated, then
`lv_obj_report_style_change(NULL)` requests a redraw. Cached pages update too;
no page or header is recreated.

The shared styles also provide gradients, shadows, and press feedback. Reuse
`ui_page_add_press_style()` to keep content-sized rows stable during a press.
For widgets with separate parts, apply the style to the part that draws them:
the Ad hoc keypad uses `ui_style_key()` on `LV_PART_ITEMS`.

Gradients consume VGLite heap memory. *main.c* budgets for the full cache set by
`LV_VG_LITE_GRAD_CACHE_CNT`; keep that budget in step with any cache changes and
avoid continuously changing gradient colours in animations.

### Responsive layout

*ui_layout.c* selects shared dimensions and fonts from the visible panel width:

**Table 4. Size classes**

Size class | Visible width | Header | Padding | Radius | Grid columns | Title / body / caption font
-----------|---------------|--------|---------|--------|--------------|-----------------------------
`UI_SIZE_COMPACT` | < 600 px | 44 px | 8 px | 8 px | 2 | 20 / 14 / 12
`UI_SIZE_REGULAR` | 600-899 px | 52 px | 12 px | 10 px | 3 | 22 / 16 / 14
`UI_SIZE_WIDE` | >= 900 px | 64 px | 16 px | 12 px | 3 | 24 / 20 / 16

The classification uses the width reported by `lv_display_get_horizontal_resolution()`,
not `MY_DISP_HOR_RES`. On the 4.3-inch panel those differ: `MY_DISP_HOR_RES` is
832, the padded frame-buffer stride, while the visible width is 800.

### What moves during a transition

The outgoing and incoming page widget trees slide in opposite directions. LVGL
re-renders the visible parts of both during the animation, without allocating
full-screen transition images or layer buffers.

### Why there is no fade transition

A cross-fade needs extra compositing layers. With the available layer buffer,
LVGL would render them in slices, adding rendering passes. Slides avoid this
memory and rendering cost, so this example uses translations only.

### Physical buttons

On kits that expose SW2 and SW4, *app_buttons.c* maps **SW4** to the theme toggle
and **SW2** to `ui_nav_next()`. KIT_PSE84_AI has only one user button, so
`USE_KIT_PSE84_AI` compiles the theme button out and its SW2 drives navigation;
the theme is changed from the Settings page by touch.

### Thread safety

Two tasks touch the UI:

- `cm55_gfx_task` runs `lv_timer_handler()` and owns LVGL.
- The button task handles physical-button actions after debounce.

`LV_USE_OS` is set to `LV_OS_FREERTOS` in *lv_conf.h*, which gives LVGL an
internal mutex and exposes `lv_lock()` / `lv_unlock()`. Every LVGL call made
from the button task is inside that pair. Nothing else in the application calls
LVGL from outside the graphics task.

The shared GPIO ISR defers button handling to the button task; it never calls
LVGL. The task confirms stable press and release levels before accepting another
press.

### Display rendering mode

*lv_port_disp.c* configures a single, fixed pipeline: double-buffered
`LV_DISPLAY_RENDER_MODE_FULL`. Two full-screen frame buffers are allocated in
the uncached `.cy_gpu_buf` pool. LVGL renders the next frame into the back
buffer while the display controller scans out the front buffer; `disp_flush()`
receives the whole rendered screen, programs the new frame-buffer address with
`Cy_GFXSS_Set_FrameBuffer()`, and waits for a vsync notification before returning.

**Table 5. Frame-buffer RAM usage per display**

Display | Buffer stride × height | Per buffer | Two buffers
--------|------------------------|-----------|------------
4.3-inch (visible 800×480) | 832 × 480 × 2 B | 798,720 B (~780 KB) | **~1560 KB**
4-inch square (visible 480×480) | 512 × 480 × 2 B | 491,520 B (~480 KB) | **~960 KB**
7-inch / 10.1-inch (visible 1024×600) | 1024 × 600 × 2 B | 1,228,800 B (~1200 KB) | **~2400 KB**

> **Note:** The stride is padded for VG-Lite's 16-pixel alignment, which is why
> the buffer width can exceed the visible width. Both buffers plus the VGLite
> heap must fit the `gfx_mem` region; check the `gfx_mem` figure in
> *build/memreport.txt* after changing panels.

### Arm&reg; Helium (M-Profile Vector Extension) acceleration for Graphics

This code example by default enables Arm&reg; Helium (M-Profile Vector Extension) acceleration for LVGL's software blend operations. The following macros in *lv_conf.h* control this:

```c
#define LV_USE_NATIVE_HELIUM_ASM  1
#define LV_USE_DRAW_SW_ASM        LV_DRAW_SW_ASM_HELIUM
```

When enabled, LVGL uses optimized Helium vector intrinsics for pixel blending, alpha compositing, and color-format conversions. This significantly improves software rendering throughput on the CM55 core. To revert to scalar C code, set `LV_USE_NATIVE_HELIUM_ASM` to `0` and `LV_USE_DRAW_SW_ASM` to `LV_DRAW_SW_ASM_NONE`.

> **Note:** Enabling Helium ASM requires that the CM55 core's MVE unit is active.

> **Note (IAR toolchain):** The IAR assembler cannot process the GCC-style `.S` assembly files used by the Helium blend routines. When building with the IAR toolchain (`TOOLCHAIN=IAR`), `LV_USE_NATIVE_HELIUM_ASM` and `LV_USE_DRAW_SW_ASM` are automatically set to `0` and `LV_DRAW_SW_ASM_NONE` respectively. Software rendering falls back to scalar C code with no code changes required. FPS and CPU usage are therefore toolchain-dependent; IAR builds are not expected to match Helium-accelerated GCC_ARM.

### Memory placement strategy

To maximize rendering performance, the LVGL library code is split across two fast memory regions using the linker scripts:

- **ITCM (zero-wait-state):** The hot-path rendering modules are copied from QSPI flash into ITCM at startup, which gives the CM55 core an instruction-fetch latency of 0 cycles for the innermost rendering loops. ITCM is only 256 KB and is nearly full, so exactly which modules fit depends on how large the toolchain's code is:

  Toolchain | LVGL modules placed in ITCM
  ----------|----------------------------
  GCC_ARM, ARM, IAR | `src/core`, most of `src/draw` (including Helium blend), `src/display`, `src/misc`, `src/tick`, `src/indev`, `src/layouts`, `src/osal`
  LLVM_ARM | Most of `src/draw`, plus `src/tick` and `src/osal`

  LLVM_ARM generates noticeably larger code in the Debug (`-O0`) configuration; keeping the full set in ITCM overflows the region, so everything except the draw hot path is moved to SOCMEM for that toolchain.

- **SOCMEM:** The remaining library modules - `src/fonts`, `src/widgets`, `src/themes`, `src/stdlib`, `src/others`, plus whatever the table above does not place in ITCM - are placed in SOCMEM. A catch-all pattern captures any `src/` code not already matched by an ITCM pattern, so it must stay after the specific ITCM entries.

- **QSPI flash:** All other application code (CM55 startup, glue drivers, VGLite wrappers) resides in the external QSPI flash and executes in XIP mode.

This placement is defined in the BSP linker scripts under `bsps/TARGET_<kit>/COMPONENT_CM55/TOOLCHAIN_<toolchain>/pse84_ns_cm55.ld` (GCC_ARM / LLVM_ARM) and the equivalent `.sct` / `.icf` files for ARM and IAR toolchains.
