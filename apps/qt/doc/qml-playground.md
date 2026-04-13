# QML Playground

The QML Playground is a live preview tool built into the AO-SDL Qt client.
It lets you load any `.qml` file from disk and see it rendered instantly,
without recompiling.  All `AO.Components` and `AO.Panels` imports are
available inside the preview, so you can prototype new panels or experiment
with existing components in isolation.

## Opening the Playground

Press **F8** while the application is running.  A separate window appears
with a toolbar, a checkerboard preview area, and a status bar.

## Loading a file

There are three ways to load a QML file:

| Method | How |
|--------|-----|
| **Browse** | Click the **Browse...** button and pick a `.qml` file from the file dialog. |
| **Paste + Load** | Paste or type a full file path into the text field, then click **Load** (or press Enter). |
| **Reload** | Click **Reload** to re-render the file that is already loaded.  Useful after saving edits in your text editor. |

The path field accepts:

- Windows paths: `C:\Users\me\samples\MyPanel.qml`
- Forward-slash paths: `C:/Users/me/samples/MyPanel.qml`
- `file://` URLs: `file:///C:/Users/me/samples/MyPanel.qml`

### Status bar

The bottom bar shows the current state:

- **Gray text** -- the name of the loaded file, or a prompt if nothing is loaded.
- **Red text** -- a QML error message.  Check your file for syntax errors or
  missing imports, fix the issue, then click **Reload**.

## Writing a playground file

A playground file is a normal `.qml` file with a single root item.  The root
item is centered on the checkerboard, so you can see exactly how large it is
and whether it has transparent regions.

### Minimal example

```qml
import QtQuick
import QtQuick.Controls

Rectangle {
    width: 200; height: 100
    color: "#2d2d30"
    radius: 6

    Label {
        anchors.centerIn: parent
        text: "Hello, Playground!"
        color: "white"
    }
}
```

Save this as a `.qml` file anywhere on disk, browse to it, and it appears in
the preview.

### Available imports

Because the playground runs inside the full application, every registered
QML module is available:

| Import | What it gives you |
|--------|-------------------|
| `import QtQuick` | Core items: Rectangle, Item, MouseArea, Canvas, etc. |
| `import QtQuick.Controls` | Button, Label, CheckBox, Slider, ComboBox, etc. |
| `import QtQuick.Layouts` | RowLayout, ColumnLayout, GridLayout, etc. |
| `import AO.Components` | AccordionDrawer, AccordionSection, ExpandingBox, DockPanel, OverlayPanel |

> **Note:** `AO.Panels` is also importable, but most panels depend on C++
> controllers that are only wired up during a courtroom session.  The
> `AO.Components` module is controller-free and works perfectly in the
> playground.

### Sizing tips

- **Give the root item an explicit size** (`width` / `height`) so it renders
  at a predictable size on the checkerboard.
- If you want the component to demonstrate resizing, wrap it in an `Item`
  with a fixed size and use `anchors.fill: parent` on the component inside.
- The preview area scrolls, so oversized items will not be clipped.

## Component reference

These components are available under `import AO.Components` and are the
primary building blocks for the AO-SDL UI.

### AccordionDrawer / AccordionSection

An exclusive-expand container.  Only one section can be open at a time.

```qml
import AO.Components

AccordionDrawer {
    direction: AccordionDrawer.TopToBottom   // or BottomToTop, LeftToRight, RightToLeft
    expandedIndex: 0                         // which section starts open (-1 = all closed)

    AccordionSection {
        title: "Section A"
        Label { text: "Content A" }
    }
    AccordionSection {
        title: "Section B"
        Label { text: "Content B" }
    }
}
```

**Properties (AccordionDrawer):**
- `direction` -- `TopToBottom` (default), `BottomToTop`, `LeftToRight`, `RightToLeft`
- `expandedIndex` -- index of the open section, or `-1` for all collapsed.

**Properties (AccordionSection):**
- `title` -- header label text.
- `expanded` -- read/write open state (managed automatically inside a drawer).
- `animationDuration` -- milliseconds for the open/close animation (default 200, set to 0 to disable).

### ExpandingBox

A collapsible container that animates between a compact button and an
expanded content area.

```qml
import AO.Components

ExpandingBox {
    expandedHeight: 160
    buttonComponent: Component { Button { text: "Show details" } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        Label { text: "Detail content here" }
    }
}
```

**Properties:**
- `expandedWidth` / `expandedHeight` -- target size when open (set one or both; `-1` = natural size).
- `invertHorizontal` / `invertVertical` -- flip expansion direction for edge-mounted buttons.
- `buttonComponent` -- a Component containing the clickable trigger.
- `animationDuration` -- milliseconds (default 200).

### OverlayPanel

A lightweight floating container that auto-sizes to its content, with an
optional semi-transparent background.

```qml
import AO.Components

OverlayPanel {
    backgroundColor: "#aa000000"
    backgroundRadius: 6
    padding: 8

    Label { text: "Status: OK"; color: "white" }
}
```

**Properties:**
- `backgroundColor` -- defaults to `"transparent"` (no rect drawn).
- `backgroundRadius` -- corner rounding for the background rect.
- `padding` -- uniform inset between background edge and content.

## Sample files

A `samples/` folder ships next to the executable with ready-made playground
files.  Open any of them with **Browse** to see the component in action:

| File | What it shows |
|------|---------------|
| `ColorPalette.qml` | Interactive HSL color picker using Canvas and sliders |
| `DashboardCards.qml` | Stat cards in a responsive grid with progress bars |
| `AccordionShowcase.qml` | All four AccordionDrawer directions side by side |
| `OverlayStack.qml` | Layered OverlayPanels with different styles and positions |

## Workflow

A typical design iteration looks like this:

1. Open the playground with **F8**.
2. Create or edit a `.qml` file in your text editor.
3. Click **Browse** (first time) or **Reload** (subsequent edits) to see changes.
4. If the status bar turns red, read the error, fix the file, and reload.
5. Once the component looks right, move it into the project's `qml/` tree and
   wire it up.

The playground does not hot-reload on file save -- you must click **Reload**
(or press Enter in the path field) each time you want to refresh.
