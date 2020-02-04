# kakoune-mode README

**WARNING: This VSCode extension is still under heavy development and should only be considered ready for testing. It is _not_ ready for real use.**

Kakoune mode allows you to use the text editor [Kakoune](https://kakoune.org) as the text editing driver instead of VSCode. This extension runs an instance of Kakoune in the background, passess all inputs to Kakoune and only uses VSCode to display the text file.
While in insert mode VSCode does take care of insertion, but everything VSCode does is echoed to Kakoune.
Once you exit insert mode, the VSCode view is synced back up with Kakoune.

Because Kakoune is used in the background, user defined functions _should_ work. As an example, custom keybindings — e.g. a custom keybinding to exit insert mode — will work by simply modifying the `.config/kak/kakrc` file.
All other custom configuration should work, at least _in theory_. Plugins might not work, depending on what kind of functionality they provide.

# Motivation

I've been trying to find a good code editor for day to day use and haven't really been able to find one.
VSCode provides the best overall experience, but I want a modal editor so I need to rely on extensions.
Currently I use the Vim extension for VSCode to simulate Vim, but I like the Kakoune way of doing things so much better.
Since there's no proper Kakoune mode plugin I decided I might as well just work on it myself!

# Current functionality

- You can open a file

# Upcoming functionality

1. Correct cursor movement.
1. Sync text from Kakoune to VSCode after changes.
1. Correct primary selection.
1. Correct multiple selections.
1. Change every instance of selected text.