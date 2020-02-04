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

# Why F#?

I've been wanting to learn a functional language for a long time and while working on this project I came across [Reason](https://reasonml.github.io/) and [Fable](https://fable.io/).
Reason allows you to transpile OCaml to JavaScript, and Fable does the same for F#.
Both languages looked like good candidates, but ultimately I decided to go with F# because I liked the structure of the language more than OCaml.

I also wanted to use a functional language for features such as pattern matching, currying, and pipes (`|>` and `<|` in F#).
I think there are many functions that can be made simpler and smaller by using these constructs.

Type safety is also a consideration, although you do get that with TypeScript, just not to the same extent.
JavaScript/TypeScript concepts like `undefined`, `null`, and `any` make the type system more complicated than I'd like it to be.

**There are some downsides to this**, particularly in the form of glue code.
The best example are probably the `src/VSCode.fs` and `src/VSCodeTypes.fs` where I've mapped the part of the VSCode API I use to F# types, and then the `.js` files under `src/` where I keep functions that are more easily implemented in JavaScript than F#.

# Build instructions

You'll need to have `npm` and `dotnet` installed.

## Script to paste into your shell of choice

```sh
dotnet tool restore
dotnet paket restore
dotnet paket install
npm install
npm run build
```

## Detailed instructions

First you'll want to install [Paket](https://fsprojects.github.io/Paket/get-started.html) for managing the Fable packages.
Once you've installed paket you can run `dotnet tool restore && dotnet paket restore && dotnet paket install` to install the necessary Fable packages.

Finally, it's a pretty standard npm build process: `npm install && npm run build` to transpile the F# code to JS.

# Current functionality

- You can open a file

# Upcoming functionality

1. Correct cursor movement.
1. Sync text from Kakoune to VSCode after changes.
1. Correct primary selection.
1. Correct multiple selections.
1. Change every instance of selected text.