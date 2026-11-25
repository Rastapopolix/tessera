/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window on macOS (gui.h): a Cocoa NSView, a child of the host's. Its own timer (the main run loop, ~30
 * times a second, in every mode so it keeps going while a menu is open) reads the view and draws the panel's pixels
 * into the view's layer as one image, 1 pixel to 1 point; on a Retina screen the layer doubles them with no smoothing,
 * so the Mono face stays sharp. The mouse, the wheel and the computer keyboard go to the panel as on Windows and
 * Linux: the keys by where they are (macOS key codes turned into the PC scan codes panel.h expects), Cmd / Ctrl /
 * Option left to the host. OPEN / SAVE use the system's file dialogs; a .tsp or a backup dropped on the window opens.
 *
 * The view's class is made at run time under a name of this loaded copy's own (its address in it), so the CLAP, the
 * VST3 and the AU, or two versions of Tessera, can sit in one host without their Objective-C classes clashing. */
#include "gui.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
/* macOS key codes (Carbon's kVK_ANSI_*, by where the key is) to PC scan codes (set 1, panel.h key); 0: not a panel key */
int scan_code(unsigned short kc)
{
    static const unsigned char SC[0x33] = {
        0x1E, 0x1F, 0x20, 0x21, 0x23, 0x22, 0x2C, 0x2D, 0x2E, 0x2F, 0x00, 0x30, 0x10, 0x11, 0x12, 0x13,   // A S D F H G Z X C V . B Q W E R
        0x15, 0x14, 0x02, 0x03, 0x04, 0x05, 0x07, 0x06, 0x0D, 0x0A, 0x08, 0x0C, 0x09, 0x0B, 0x1B, 0x18,   // Y T 1 2 3 4 6 5 = 9 7 - 8 0 ] O
        0x16, 0x1A, 0x17, 0x19, 0x00, 0x26, 0x24, 0x28, 0x25, 0x27, 0x2B, 0x33, 0x35, 0x31, 0x32, 0x34,   // U [ I P . L J ' K ; \ , / N M .
        0x0F, 0x39, 0x29};                                                                                 // Tab Space `
    switch (kc)
    {
    case 0x7E: return Panel::KEY_UP;
    case 0x7D: return Panel::KEY_DOWN;
    case 0x7B: return Panel::KEY_LEFT;
    case 0x7C: return Panel::KEY_RIGHT;
    case 0x74: return Panel::KEY_PGUP;
    case 0x79: return Panel::KEY_PGDN;
    case 0x7A: return 0x3B;   // F1
    case 0x35: return 0x01;   // Esc
    case 0x24: return 0x1C;   // Return
    }
    return kc < sizeof SC ? SC[kc] : 0;
}

class GuiMac;
Ivar gui_ivar = nullptr;   // the view's pointer to its GuiMac
GuiMac *gui_of(id view) { return *reinterpret_cast<GuiMac **>((char *)(__bridge void *)view + ivar_getOffset(gui_ivar)); }

class GuiMac : public Gui
{
  public:
    explicit GuiMac(GuiLink *l) : link(l)
    {
        panel.set_shell(l->shell());
        panel.set_keys(l->keys_on());
        panel.set_hover_help(l->hover_help_on());
        panel.set_inks(l->inks());
        panel.set_tracks(l->tracks());
        panel.set_version(tess_version());
        panel.set_expanded(l->expanded());
        pw = panel.width();
    }
    GuiLink *link;   // this window's instance
    Panel panel;
    std::vector<uint32_t> px = std::vector<uint32_t>(Panel::W_MAX * Panel::H);
    int pw = Panel::W;               // the panel's width the view has (FM-1 or EXPANDED: panel.width())
    tess_view_t view{};
    std::unique_ptr<tess_tracks_t> tracks{new tess_tracks_t()};   // TRACKS (EXPANDED)
    NSView *nsview = nil;
    NSTimer *timer = nil;
    CGColorSpaceRef rgb = nullptr;
    CGContextRef bitmap = nullptr;   // over px
    double wheel_acc = 0, hwheel_acc = 0;   // a trackpad's scrolling, not yet a whole notch (up-down, sideways)
    int pct = 100;                   // SIZE 100 / 125 / 150 / 200 %: the layer stretches the 1:1 picture
    void set_scale(int n) override
    {
        if (n < 100 || n > 200)
            return;
        pct = n;
        resize();
    }
    // the view at the panel's width (FM-1 or EXPANDED) and SIZE; the bitmap over px as wide as the panel
    void resize()
    {
        panel.set_expanded(link->expanded());
        bool wider = panel.width() != pw;
        pw = panel.width();
        if (!nsview)
            return;   // (set_parent makes the view at this size)
        if (wider)
            make_bitmap();
        [nsview setFrameSize:NSMakeSize(pw * pct / 100, Panel::H * pct / 100)];
        filter();
    }
    bool make_bitmap()
    {
        if (bitmap)
            CGContextRelease(bitmap);
        // px is 0xAARRGGBB words: in memory B G R A, the alpha unused
        bitmap = CGBitmapContextCreate(px.data(), pw, Panel::H, 8, pw * 4, rgb,
                                       kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaNoneSkipFirst);
        return bitmap != nullptr;
    }
    // whole pixels doubled at 100 % and 200 % (Retina: 2x, 4x); smoothed at 125 and 150 %, where they cannot be whole
    void filter() { nsview.layer.magnificationFilter = pct % 100 ? kCAFilterLinear : kCAFilterNearest; }

    ~GuiMac() override
    {
        [timer invalidate];
        timer = nil;
        if (nsview)
        {
            *reinterpret_cast<GuiMac **>((char *)(__bridge void *)nsview + ivar_getOffset(gui_ivar)) = nullptr;
            [nsview removeFromSuperview];
            nsview = nil;
        }
        if (bitmap) CGContextRelease(bitmap);
        if (rgb) CGColorSpaceRelease(rgb);
    }

    bool set_parent(const clap_window_t *w) override;
    bool show() override
    {
        if (!nsview) return false;
        nsview.hidden = NO;
        return true;
    }
    bool hide() override
    {
        if (!nsview) return false;
        nsview.hidden = YES;
        return true;
    }

    // OPEN or SAVE, if one was clicked (and a shell picked): the system's dialog, then the instance opens or saves
    void files()
    {
        int kr = panel.take_keys_request();    // (KEYS ON or OFF: every window's)
        if (kr >= 0)
            link->keys_set(kr == 1);
        int hr = panel.take_hover_request();   // (HOVER HELP ON or OFF: every window's)
        if (hr >= 0)
            link->hover_help_set(hr == 1);
        int sh = panel.take_shell_request();   // (a shell picked: its ink)
        if (sh >= 0)
            link->shell_picked(sh);
        int ink = panel.take_ink_request();    // (an ink picked)
        if (ink >= 0)
            link->ink_picked(ink);
        int req = panel.take_file_request();
        if (req == Panel::FILE_NONE)
            return;
        if (req == Panel::FILE_MIDI)   // (no dialog: the clip goes to Documents/Tessera/MIDI)
        {
            panel.note(link->export_midi());
            return;
        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"   // (allowedFileTypes: allowedContentTypes needs macOS 11)
        if (req == Panel::FILE_OPEN)
        {
            NSOpenPanel *op = [NSOpenPanel openPanel];
            op.title = @"Open a Tessera project";
            op.message = @"A Tessera project (.tsp) or an FM-1 backup (.json)";
            op.allowedFileTypes = @[ @"tsp", @"json" ];
            op.canChooseDirectories = NO;
            op.allowsMultipleSelection = NO;
            if ([op runModal] == NSModalResponseOK && op.URL.fileSystemRepresentation)
                panel.note(link->open_file(op.URL.fileSystemRepresentation));
        }
        else
        {
            NSSavePanel *sp = [NSSavePanel savePanel];
            sp.title = @"Save the Tessera project";
            sp.allowedFileTypes = @[ @"tsp" ];
            sp.nameFieldStringValue = [NSString stringWithUTF8String:link->file_name().c_str()] ?: @"Tessera.tsp";
            std::string presets = projfile::presets_dir();   // (the DAW's preset browser lists what is saved there)
            if (!presets.empty())
                sp.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:presets.c_str()] isDirectory:YES];
            if ([sp runModal] == NSModalResponseOK && sp.URL.fileSystemRepresentation)
                panel.note(link->save_file(sp.URL.fileSystemRepresentation));
        }
#pragma clang diagnostic pop
        [nsview.window makeFirstResponder:nsview];   // (the keys back to the panel)
        draw();
    }

    void input(bool changed)
    {
        if (changed)
        {
            link->input(panel.input);
            draw();
        }
    }

    void draw()
    {
        if (!nsview || !bitmap)
            return;
        bool t2 = panel.expanded() && link->tracks_view(*tracks);
        panel.draw(px.data(), view, t2 ? tracks.get() : nullptr);
        CGImageRef img = CGBitmapContextCreateImage(bitmap);   // (a copy: the layer keeps it)
        [CATransaction begin];
        [CATransaction setDisableActions:YES];   // (no fade between frames)
        nsview.layer.contents = (__bridge id)img;
        [CATransaction commit];
        CGImageRelease(img);
    }

    // the screen's ink changed since the last frame (not at the window's first): the next window's
    int ink_last = -1;
    void ink_check()
    {
        if (ink_last >= 0 && (int)view.ink != ink_last)
            link->ink_seen((int)view.ink);
        ink_last = (int)view.ink;
    }

    void tick() override
    {
        link->view(view);
        panel.set_midi(link->midi_count());
        panel.set_standalone(link->standalone());
        gui_fm1_tick(panel, link);
        gui_size_tick(panel, link);
        if (panel.width() != pw)   // (MODE, or a shell picked: the view follows)
            resize();
        gui_listen_tick(panel, link);
        gui_gen_tick(panel, link);
        ink_check();
        draw();
    }

    // a point in the view (flipped: top left 0,0) as the panel's pixels (1:1, whatever SIZE)
    NSPoint where(NSEvent *e)
    {
        NSPoint p = [nsview convertPoint:e.locationInWindow fromView:nil];
        return NSMakePoint(p.x * 100 / pct, p.y * 100 / pct);
    }
};

// ---- the view's methods (the class is made in view_class)
BOOL v_yes(id, SEL) { return YES; }
BOOL v_first_mouse(id, SEL, NSEvent *) { return YES; }
void v_down(id self, SEL, NSEvent *e)
{
    if (GuiMac *g = gui_of(self))
    {
        [[self window] makeFirstResponder:self];
        NSPoint p = g->where(e);
        if (e.modifierFlags & NSEventModifierFlagControl)   // Ctrl-click: as a right-click, a latch
            g->input(g->panel.latch((int)p.x, (int)p.y));
        else
            g->input(g->panel.down((int)p.x, (int)p.y));
    }
}
void v_right_down(id self, SEL, NSEvent *e)   // a button or key latched
{
    if (GuiMac *g = gui_of(self))
    {
        [[self window] makeFirstResponder:self];
        NSPoint p = g->where(e);
        g->input(g->panel.latch((int)p.x, (int)p.y));
    }
}
void v_other(id self, NSEvent *e, bool down)   // the mouse's back (3) and forward (4) buttons: BACK, OK
{
    GuiMac *g = gui_of(self);
    if (g && (e.buttonNumber == 3 || e.buttonNumber == 4))
        g->input(g->panel.side_button(e.buttonNumber == 4, down));
}
void v_other_down(id self, SEL, NSEvent *e) { v_other(self, e, true); }
void v_other_up(id self, SEL, NSEvent *e) { v_other(self, e, false); }
void v_drag(id self, SEL, NSEvent *e)
{
    if (GuiMac *g = gui_of(self))
    {
        NSPoint p = g->where(e);
        g->input(g->panel.move((int)p.x, (int)p.y));
    }
}
void v_up(id self, SEL, NSEvent *e)
{
    if (GuiMac *g = gui_of(self))
    {
        NSPoint p = g->where(e);
        g->input(g->panel.up((int)p.x, (int)p.y));
        g->files();   // (OPEN or SAVE clicked: its dialog, once the button is up)
    }
}
void v_wheel(id self, SEL, NSEvent *e)
{
    GuiMac *g = gui_of(self);
    if (!g)
        return;
    // sideways (a tilt, a swipe): a page, about 40 points of a trackpad's swipe one (Panel::hwheel then waits a
    // quarter second, so one swipe is one page); macOS reports right as negative
    double dx = e.hasPreciseScrollingDeltas ? e.scrollingDeltaX / 40.0 : e.scrollingDeltaX;
    if (e.isDirectionInvertedFromDevice) dx = -dx;
    if (dx != 0 && std::fabs(e.scrollingDeltaX) > std::fabs(e.scrollingDeltaY))
    {
        g->hwheel_acc -= dx;
        int n = (int)g->hwheel_acc;
        if (n)
        {
            g->hwheel_acc = 0;
            g->input(g->panel.hwheel(n));
        }
        return;
    }
    // a notch a step; a trackpad's smooth deltas gathered into steps of about a notch
    double d = e.hasPreciseScrollingDeltas ? e.scrollingDeltaY / 10.0 : e.scrollingDeltaY;
    if (e.isDirectionInvertedFromDevice) d = -d;
    g->wheel_acc += d;
    int n = (int)g->wheel_acc;
    if (!n)
        return;
    g->wheel_acc -= n;
    NSPoint p = g->where(e);
    g->input(g->panel.wheel((int)p.x, (int)p.y, n));
}
bool v_key(id self, NSEvent *e, bool down)
{
    GuiMac *g = gui_of(self);
    if (g && down && (e.modifierFlags & (NSEventModifierFlagCommand | NSEventModifierFlagControl |
                                         NSEventModifierFlagOption)) == NSEventModifierFlagCommand &&
        (e.keyCode == 0x1F || e.keyCode == 0x01) && g->panel.shortcut(e.keyCode == 0x1F ? 'O' : 'S'))
    {   // (the standalone app: Cmd+O / Cmd+S, OPEN / SAVE)
        g->files();
        return true;
    }
    if (!g || (e.modifierFlags & (NSEventModifierFlagCommand | NSEventModifierFlagControl | NSEventModifierFlagOption)))
        return false;   // (the host's)
    if (down && g->panel.typing())   // NAME open: the character the layout gives, typed
    {
        uint16_t kc = e.keyCode;
        uint32_t ch = kc == 51 ? TESS_TEXT_BACK : kc == 36 || kc == 76 ? TESS_TEXT_OK : kc == 53 ? TESS_TEXT_CANCEL
                    : kc == 123 ? TESS_TEXT_LEFT : kc == 124 ? TESS_TEXT_RIGHT : kc == 117 ? TESS_TEXT_DELETE : 0;
        NSString *typed = e.characters;
        if (!ch && typed.length == 1 && [typed characterAtIndex:0] >= 32 && [typed characterAtIndex:0] < 0xF700)
            ch = [typed characterAtIndex:0];   // (0xF700..: the function keys' private codes)
        if (ch && g->panel.text(ch))
        {
            g->input(true);
            return true;
        }
    }
    int code = scan_code(e.keyCode);
    if (!code)
        return false;
    if (down && e.isARepeat)
        return true;   // (held: the panel has it already)
    bool ours = g->panel.key(code, down);
    g->input(ours);
    return ours;   // (not the panel's: up the responder chain, to the host)
}
void v_key_down(id self, SEL, NSEvent *e)
{
    if (!v_key(self, e, true))
        [[self nextResponder] keyDown:e];
}
void v_key_up(id self, SEL, NSEvent *e)
{
    if (!v_key(self, e, false))
        [[self nextResponder] keyUp:e];
}
// Shift (left 56, right 60): no keyDown / keyUp of its own, only the modifiers' change; to the panel as the PC's 0x2A
void v_flags(id self, SEL, NSEvent *e)
{
    GuiMac *g = gui_of(self);
    bool ours = false;
    if (g && (e.keyCode == 56 || e.keyCode == 60))
    {
        ours = g->panel.key(0x2A, (e.modifierFlags & NSEventModifierFlagShift) != 0);
        g->input(ours);
    }
    if (!ours)
        [[self nextResponder] flagsChanged:e];
}
BOOL v_resign(id self, SEL)
{
    if (GuiMac *g = gui_of(self))
        g->input(g->panel.release_keys());
    return YES;
}
void v_tracking(id self, SEL)   // (the mouse over the window without a button: the panel's hover names)
{
    NSView *v = self;
    for (NSTrackingArea *a in [v.trackingAreas copy])
        [v removeTrackingArea:a];
    [v addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                    options:NSTrackingMouseMoved | NSTrackingActiveAlways |
                                                            NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited
                                                      owner:v
                                                   userInfo:nil]];
    struct objc_super sup = {self, class_getSuperclass(object_getClass(self))};
    reinterpret_cast<void (*)(struct objc_super *, SEL)>(objc_msgSendSuper)(&sup, @selector(updateTrackingAreas));
}
NSDragOperation v_drag_enter(id, SEL, id<NSDraggingInfo>) { return NSDragOperationCopy; }
BOOL v_drop(id self, SEL, id<NSDraggingInfo> info)
{
    GuiMac *g = gui_of(self);
    NSArray<NSURL *> *urls = [info.draggingPasteboard readObjectsForClasses:@[ [NSURL class] ]
                                                                    options:@{NSPasteboardURLReadingFileURLsOnlyKey : @YES}];
    if (!g || !urls.count || !urls[0].fileSystemRepresentation)
        return NO;
    g->panel.note(g->link->open_file(urls[0].fileSystemRepresentation));
    g->draw();
    return YES;
}

void add(Class c, SEL s, IMP f, const char *types) { class_addMethod(c, s, f, types); }
const char *types_of(SEL s) { return method_getTypeEncoding(class_getInstanceMethod([NSView class], s)); }

Class view_class()
{
    static Class cls = nil;
    if (cls)
        return cls;
    static int here;   // (this loaded copy's address: a name of its own)
    char name[64];
    std::snprintf(name, sizeof name, "TesseraPanelView_%p", (void *)&here);
    cls = objc_allocateClassPair([NSView class], name, 0);
    class_addIvar(cls, "tess_gui", sizeof(void *), alignof(void *) == 8 ? 3 : 2, "^v");
    add(cls, @selector(isFlipped), (IMP)v_yes, types_of(@selector(isFlipped)));
    add(cls, @selector(acceptsFirstResponder), (IMP)v_yes, types_of(@selector(acceptsFirstResponder)));
    add(cls, @selector(acceptsFirstMouse:), (IMP)v_first_mouse, types_of(@selector(acceptsFirstMouse:)));
    add(cls, @selector(mouseDown:), (IMP)v_down, types_of(@selector(mouseDown:)));
    add(cls, @selector(rightMouseDown:), (IMP)v_right_down, types_of(@selector(rightMouseDown:)));
    add(cls, @selector(mouseDragged:), (IMP)v_drag, types_of(@selector(mouseDragged:)));
    add(cls, @selector(mouseMoved:), (IMP)v_drag, types_of(@selector(mouseMoved:)));
    add(cls, @selector(mouseUp:), (IMP)v_up, types_of(@selector(mouseUp:)));
    add(cls, @selector(scrollWheel:), (IMP)v_wheel, types_of(@selector(scrollWheel:)));
    add(cls, @selector(otherMouseDown:), (IMP)v_other_down, types_of(@selector(otherMouseDown:)));
    add(cls, @selector(otherMouseUp:), (IMP)v_other_up, types_of(@selector(otherMouseUp:)));
    add(cls, @selector(keyDown:), (IMP)v_key_down, types_of(@selector(keyDown:)));
    add(cls, @selector(keyUp:), (IMP)v_key_up, types_of(@selector(keyUp:)));
    add(cls, @selector(flagsChanged:), (IMP)v_flags, types_of(@selector(flagsChanged:)));
    add(cls, @selector(resignFirstResponder), (IMP)v_resign, types_of(@selector(resignFirstResponder)));
    add(cls, @selector(updateTrackingAreas), (IMP)v_tracking, types_of(@selector(updateTrackingAreas)));
    char t_enter[16], t_drop[16];   // (NSDraggingDestination: no NSView method to copy the types from)
    std::snprintf(t_enter, sizeof t_enter, "%s@:@", @encode(NSDragOperation));
    std::snprintf(t_drop, sizeof t_drop, "%s@:@", @encode(BOOL));
    add(cls, @selector(draggingEntered:), (IMP)v_drag_enter, t_enter);
    add(cls, @selector(performDragOperation:), (IMP)v_drop, t_drop);
    objc_registerClassPair(cls);
    gui_ivar = class_getInstanceVariable(cls, "tess_gui");
    return cls;
}

bool GuiMac::set_parent(const clap_window_t *w)
{
    NSView *parent = (__bridge NSView *)w->cocoa;
    if (!parent || nsview)
        return false;
    rgb = CGColorSpaceCreateDeviceRGB();
    if (!make_bitmap())
        return false;
    nsview = [[view_class() alloc] initWithFrame:NSMakeRect(0, 0, pw * pct / 100, Panel::H * pct / 100)];
    *reinterpret_cast<GuiMac **>((char *)(__bridge void *)nsview + ivar_getOffset(gui_ivar)) = this;
    nsview.wantsLayer = YES;
    nsview.layer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
    nsview.layer.contentsGravity = kCAGravityResize;
    filter();   // (Retina: whole pixels doubled, no blur)
    nsview.layer.opaque = YES;
    [nsview registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];
    [parent addSubview:nsview];
    GuiMac *self_ = this;
    timer = [NSTimer timerWithTimeInterval:1.0 / 30 repeats:YES block:^(NSTimer *) { self_->tick(); }];
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
    tick();
    return true;
}
}   // namespace

const char *Gui::api() { return CLAP_WINDOW_API_COCOA; }
int Gui::screen_width()
{
    NSScreen *s = [NSScreen mainScreen];   // (points; the visible frame: without the Dock)
    return s ? (int)s.visibleFrame.size.width : 0;
}
std::unique_ptr<Gui> Gui::make(const clap_host_t *, GuiLink *link) { return std::make_unique<GuiMac>(link); }
