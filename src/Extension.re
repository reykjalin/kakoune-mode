let activate = context => {
  switch (Vscode.TextEditor.document()) {
  | None =>
    Kakoune.setKak(Node.spawn("kak", [|"-ui", "json", "-s", "vscode"|]))
  | Some(doc) =>
    Kakoune.setKak(
      Node.spawn("kak", [|"-ui", "json", "-s", "vscode", doc.fileName|]),
    )
  };

  Kakoune.getKak().stderr.on(. "data", Kakoune.handleIncomingError);
  Kakoune.getKak().stdout.on(. "data", Kakoune.handleIncomingCommand);

  Vscode.overrideTypeCommand(context);
  Vscode.TextEditor.Block |> Vscode.setCursorStyle;

  Vscode.Commands.registerCommand("extension.send_escape", () =>
    Rpc.createKeysMessage("<esc>")
    |> Rpc.stringifyMessage
    |> Kakoune.writeToKak
  )
  |> Js.Array2.push(context.subscriptions)
  |> ignore;

  Vscode.Commands.registerCommand("extension.send_backspace", () => {
    switch (Mode.getMode()) {
    | Mode.Insert => Vscode.Commands.executeCommand("deleteLeft")
    | _ => ()
    };

    Rpc.createKeysMessage("<backspace>")
    |> Rpc.stringifyMessage
    |> Kakoune.writeToKak;
  })
  |> Js.Array2.push(context.subscriptions)
  |> ignore;
};
