[@bs.module] external vscode: Js.t({..}) = "vscode";

type disposable;
type textEditor;
type uri = {toString: (. unit) => string};
type textDocument = {
  uri,
  fileName: string,
};
type textCommandArgs = {text: option(string)};

type extension_context = {subscriptions: array(disposable)};

module Commands = {
  let registerCommand: (string, 'a => unit) => disposable =
    (name, callback) => vscode##commands##registerCommand(name, callback);

  let executeCommand: string => unit =
    command => vscode##commands##executeCommand(command);

  let executeCommandWithArg: (string, textCommandArgs) => unit =
    (command, arg) => vscode##commands##executeCommand(command, arg);
};

module Window = {
  let activeTextEditor: unit => option(textEditor) =
    () => Js.toOption(vscode##window##activeTextEditor);
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

let overrideTypeCommand = context => {
  overrideCommand(context, "type", args => {
    switch (args.text) {
    | Some(t) =>
      Rpc.createKeysMessage(t) |> Rpc.stringifyMessage |> Kakoune.writeToKak
    | None => ()
    }
  });
};

let setCursorStyle = style => {
  switch (TextEditor.options()) {
  | None => ()
  | Some(o) => o.cursorStyle = style |> TextEditor.cursorStyleToJs
  };
};
