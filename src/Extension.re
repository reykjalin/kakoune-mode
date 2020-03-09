let activate = context => {
  Kakoune.setKak(Node.spawn("kak", [|"-ui", "json", "-s", "vscode"|]));

  Kakoune.getKak().stderr.on(. "data", Kakoune.handleIncomingError);
  Kakoune.getKak().stdout.on(. "data", Kakoune.handleIncomingCommand);

  Vscode.overrideTypeCommand(context);

  Vscode.Commands.registerCommand("extension.send_escape", () =>
    Js.log("escape")
  )
  |> Js.Array2.push(context.subscriptions)
  |> ignore;

  Vscode.Commands.registerCommand("extension.send_backspace", () =>
    Js.log("backspace")
  )
  |> Js.Array2.push(context.subscriptions)
  |> ignore;
};
