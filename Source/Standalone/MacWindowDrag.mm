// macOS: let the window server move the borderless standalone window (like a native title bar) instead of
// repositioning it from JUCE mouse events, which trails behind the cursor on macOS.
#import <AppKit/AppKit.h>

namespace voidworm
{
bool beginNativeWindowDrag (void* nativeView)
{
    auto* view = static_cast<NSView*> (nativeView);
    NSEvent* event = [NSApp currentEvent];
    if (view == nil || view.window == nil || event == nil || event.type != NSEventTypeLeftMouseDown)
        return false;
    [view.window performWindowDragWithEvent: event];
    return true;
}
}
