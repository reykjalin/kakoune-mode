# kakoune-mode README

**WARNING: This VSCode extension is still under heavy development and should only be considered ready for testing. It is _not_ ready for real use.**

Kakoune mode allows you to use the text editor [Kakoune](https://kakoune.org) as the text editing driver instead of VSCode. This extension runs an instance of Kakoune in the background, passess all inputs to Kakoune and only uses VSCode to display the text file.

Because Kakoune is used in the background, user defined functions _should_ work. As an example, custom keybindings — e.g. a custom keybinding to exit insert mode — will work by simply modifying the `.config/kak/kakrc` file.
All other custom configuration should work, at least _in theory_. Plugins might not work, depending on what kind of functionality they provide.

# Known major issues

- Displaying a file longer than a couple lines is severely broken.
- Scrolling the view of the file is severely broken.
- Multiple selections don't work.
- Using the mouse to move the cursor doesn't work.
- Using the mouse to create selections doesn't work.

These are only the major known issues. There are a whole bunch of minor issues that have to be addressed as well.

# Annoying issues

#### Cursor jumps around

When writing the cursor jumps around a lot. This is because currently a line is [replaced](https://code.visualstudio.com/api/references/vscode-api#TextEdit) via the VSCode API every time you make a change to a line, causing the cursor to jump to the end of that line. The cursor immediately moved to the right location afterwards, but this causes an annoying flickering of the cursor when typing.

The most ideal solution to this would be to use VSCode to _only_ display text, but that would require you to save the file after every edit to sync the view. A better solution would be to have both VSCode and Kakoune type when in insert mode, then sync the views once you leave insert mode.

In essence, VSCode should be used as little as possible to make _edits_, and instead used as much as possible to just _view_ the file.