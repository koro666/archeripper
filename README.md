# Archeage Crest Ripper

## What is it?

It's a tool that allows you to see the user crests that are downloaded by the game and cached locally, as well as save them for reuse.

## How does it work?

The game downloads every crest it encounters in a specific radius inside files in the `C:\ArcheAge\Documents\USER\UCC\down`, which are of a format specific to the game. It stores them as binary blobs inside `.sst` files in the aforementioned folder, multiple crests per file. That binary blob so happens to be a [DDS file](https://msdn.microsoft.com/en-us/library/windows/desktop/bb943991.aspx) which is a standard texture container format for DirectX. This tool simply opens each `.sst` file, and scans its entirety for DDS header signatures (the byte sequence `44 44 53 20 7C 00 00 00`), and when it finds one, tries to load it using [D3DX](https://msdn.microsoft.com/en-us/library/windows/desktop/bb172638.aspx).

## How to get it?

### Download it

Head to the [releases](https://github.com/koro666/archeripper/releases) page and grab the latest EXE.

### Build it yourself

To build the source, you will need:

- [Microsoft Visual Studio 2012](http://www.microsoft.com/en-ca/download/details.aspx?id=34673)
- [Microsoft DirectX SDK (June 2010)](http://www.microsoft.com/en-ca/download/details.aspx?id=6812) (for `d3dx9.h` and `d3dx9.lib`)

Make sure the `DXSDK_DIR` environment variable is properly set.

## How to use it?

Start the program, preferably with the game not running so that it does not interfere with it. It will show every crest it can find. You can double-click one to pop up a Save dialog, or you can drag-and-drop them into an Explorer window.

## Miscellaneous information

### System requirements

- Windows Vista+ 64-bit (no XP support)
- A lot of RAM (about 320k per loaded crest, I had ~400-500MB usage when testing with about 1400 crests)
