type disposable;
type textEditor;
type uri = {toString: (. unit) => string};
type textDocument = {uri};
type textCommandArgs = {text: option(string)};

type extension_context = {subscriptions: array(disposable)};

[@bs.module] external vscode: Js.t({..}) = "vscode";

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

  let document: unit => option(textDocument) =
    () => Js.toOption(vscode##window##activeTextEditor##document);
};

let overrideCommand = (context, command, callback) => {
  Commands.registerCommand(command, args => {
    switch (Window.activeTextEditor()) {
    | None => ()
    | Some(_) =>
      switch (TextEditor.document()) {
      | None => ()
      | Some(document) =>
        switch (document.uri.toString(.)) {
        | "debug:input" =>
          Commands.executeCommandWithArg("default:" ++ command, args)
        | _ =>
          switch (Mode.getMode()) {
          | Mode.Insert =>
            Commands.executeCommandWithArg("default:" ++ command, args)
          | Mode.Normal => callback(args)
          }
        }
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
