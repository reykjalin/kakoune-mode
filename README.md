# Kakoune mode

**WARNING: This VSCode extension is still under heavy development and should only be considered ready for testing. It is _not_ ready for real use.**

Kakoune mode allows you to use the text editor [Kakoune](https://kakoune.org) as the text editing driver instead of VSCode. This extension runs an instance of Kakoune in the background, passess all inputs to Kakoune and only uses VSCode to display the text file.
While in insert mode VSCode does take care of insertion, but everything VSCode does is echoed to Kakoune.
Once you exit insert mode, the VSCode view is synced back up with Kakoune.

Because Kakoune is used in the background, user defined functions _should_ work. As an example, custom keybindings — e.g. a custom keybinding to exit insert mode — will work by simply modifying the `.config/kak/kakrc` file.
All other custom configuration should work, at least _in theory_. Plugins might not work, depending on what kind of functionality they provide.

# Motivation

I've been [trying to find a good code editor](https://thorlaksson.com/post/its-2019-why-dont-we-have-good-code-editors) for day to day use and haven't really been able to find one.
VSCode provides the best overall experience, but I want a modal editor so I need to rely on extensions.
Currently I use the Vim extension for VSCode to simulate Vim, but I like the Kakoune way of doing things so much better.
Since there's no proper Kakoune mode plugin I decided I might as well just work on it myself!

# Why Reason?

I've been wanting to learn a functional language for a long time and while working on this project I came across [Reason](https://reasonml.github.io/) and [Fable](https://fable.io/).
Reason allows you to compile OCaml to JavaScript, and Fable compiles F# to JavaScript.
Both languages looked like good candidates, and at first I decided to go with F# because I liked the structure and syntax of the language more than OCaml and Reason.

However, after using Fable for a bit, I felt that the documentation was [a bit lacking](https://thorlaksson.com/post/calling-fable-from-typescript), and the JavaScript interoperability, while usable, isn't great.
So I tried to use Reason, and I find the JavaScript interop much more user friendly and terse.
The codebase in Reason is a bit smaller, mostly thanks to less glue code for JavaScript interop.

The Reason tooling is also much better.
The F# tooling for VSCode can feel a bit slow, often taking ~1sec to think for auto-completions and types, and you need to compile manually (or use a watcher) while developing.
The Reason tooling is relatively fast, auto-completions come in instantly, and the toolchain is fast enough to **recompile automatically after every save**, which is an immense productivity boost.

My reason for looking into these 2 frameworks is that I wanted to use a functional language for features such as pattern matching, currying, and pipes (`|>` and `->` in Reason).
I think there are many functions that can be made simpler and smaller by using these constructs and unfortunately JavaScript isn't optimal for this; it's more difficult to write functional code in JavaScript than it is to write imperative code.

Type safety is also a big consideration, although you do get that with TypeScript, just not to the same extent.
JavaScript/TypeScript concepts like `undefined`, `null`, and `any` make the type system more complicated than I'd like it to be.

**There are some downsides to this**, particularly in the form of glue code.
The best example is probably `src/VSCode.re` where I've mapped the part of the VSCode API I use to Reason types.
Most of this code still looks like Reason, which was not the case with F#.

# Build instructions

`kak` **needs** to be available in your `PATH`.
You can find installation instructions for Kakoune in the [GitHub repository](https://github.com/mawww/kakoune#22-installing).

You'll need to have `npm` or `yarn` installed.

## Script to paste into your shell of choice

```sh
npm install
npm run build
```

# Current functionality

- You can open a file.
- The cursor can be moved around with the standard movement keys for Kakoune.

# Upcoming functionality

1. Correct primary selection.
1. Correct multiple selections.
1. Sync text from Kakoune to VSCode after changes.
1. Change every instance of selected text.
1. Search and replace.
