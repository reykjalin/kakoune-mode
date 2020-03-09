[@bs.module] external vscode: Js.t({..}) = "vscode";

type disposable;

type position;
type selection;

type uri = {toString: (. unit) => string};
type textDocument = {
  uri,
  fileName: string,
};
type textCommandArgs = {text: option(string)};
type textEditor = {
  document: textDocument,
  selection,
};

type extension_context = {subscriptions: array(disposable)};

module Commands = {
  let registerCommand: (string, 'a => unit) => disposable =
    (name, callback) => vscode##commands##registerCommand(name, callback);

  let executeCommand: string => unit =
    command => vscode##commands##executeCommand(command);

  let executeCommandWithArg: (string, textCommandArgs) => unit =
    (command, arg) => vscode##commands##executeCommand(command, arg);
};

module TextEditor = {
  type t = textEditor;

  [@bs.deriving jsConverter]
  type cursorStyle =
    | [@bs.as 1] Line
    | [@bs.as 2] Block
    | [@bs.as 3] Underline
    | [@bs.as 4] LineThin
    | [@bs.as 5] BlockOutline
    | [@bs.as 6] UnderlineThin;

  type textEditorOptions = {mutable cursorStyle: int};

  let options: unit => option(textEditorOptions) =
    () => Js.toOption(vscode##window##activeTextEditor##options);

  let document: unit => option(textDocument) =
    () => Js.toOption(vscode##window##activeTextEditor##document);

  [@bs.set] external setSelection: (t, selection) => unit = "selection";
};

module Window = {
  type event('a) = option('a);

  let activeTextEditor: unit => option(textEditor) =
    () => Js.toOption(vscode##window##activeTextEditor);

  let showError: string => unit =
    message => vscode##window##showErrorMessage(message);

  let onDidChangeActiveTextEditor: (event(TextEditor.t) => unit) => unit =
    event => vscode##window##onDidChangeActiveTextEditor(event);
};

module Position = {
  type t = position;

  [@bs.module "vscode"] [@bs.new]
  external make: (~line: int, ~character: int) => t = "Position";
  [@bs.get] external character: t => int = "character";
  [@bs.get] external line: t => int = "line";
};

module Selection = {
  type t = selection;

  [@bs.module "vscode"] [@bs.new]
  external make: (~anchor: position, ~active: position) => t = "Selection";
  [@bs.get] external anchor: t => position = "anchor";
  [@bs.get] external active: t => position = "active";
  [@bs.get] external start: t => position = "start";
  [@bs.get] external end_: t => position = "end";
};

let overrideCommand = (context, command, callback) => {
  Commands.registerCommand(command, args => {
    switch (TextEditor.document()) {
    | None => ()
    | Some(document) =>
      switch (document.uri.toString(.), Mode.getMode()) {
      | ("debug:input", _currentMode) =>
        Commands.executeCommandWithArg("default:" ++ command, args)
      | (_documentUri, Mode.Insert) =>
        Commands.executeCommandWithArg("default:" ++ command, args);
        callback(args);
      | _ => callback(args)
      }
    }
  })
  |> Js.Array2.push(context.subscriptions)
  |> ignore;
};

let overrideTypeCommand = (context, writeToKak) => {
  overrideCommand(context, "type", args => {
    switch (args.text) {
    | Some(t) =>
      Rpc.createKeysMessage(t) |> Rpc.stringifyMessage |> writeToKak
    | None => ()
    }
  });
};

let registerWindowChangeEventHandler = writeToKak => {
  Window.onDidChangeActiveTextEditor(event => {
    switch (event) {
    | None => ()
    | Some(e) =>
      Rpc.createKeysMessage(":e " ++ e.document.fileName ++ "<ret>")
      |> Rpc.stringifyMessage
      |> writeToKak
    }
  });
};

let setCursorStyle = style => {
  switch (TextEditor.options()) {
  | None => ()
  | Some(o) => o.cursorStyle = style |> TextEditor.cursorStyleToJs
  };
};
