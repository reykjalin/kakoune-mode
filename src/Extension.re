type stream_data = {data: bytes};

type process_stream = {
  on: (. string, bytes => unit) => unit,
  write: string => unit,
};

type child_process = {
  stdout: process_stream,
  stderr: process_stream,
};

type disposable;

type extension_context = {subscriptions: array(disposable)};

type vscode_commands = {
  registerCommand: (. string, unit => unit) => disposable,
};

type vscode = {commands: vscode_commands};

[@bs.module] external vscode: vscode = "vscode";

[@bs.module "child_process"]
external spawn: (string, array(string)) => child_process = "spawn";

let activate = context => {
  spawn("kak", [|"-clear"|])->ignore;
  let kak = spawn("kak", [|"-ui", "json", "-s", "vscode"|]);
  //   let handleCommand = command => Js.log(command);
  kak.stderr.on(. "data", e => Js.log(e->Bytes.to_string));
  kak.stdout.on(. "data", c => Js.log(c->Bytes.to_string));

  vscode.commands.registerCommand(. "extension.send_escape", () =>
    Js.log("escape")
  )
  ->Js_array.push(context.subscriptions)
  ->ignore;

  vscode.commands.registerCommand(. "extension.send_backspace", () =>
    Js.log("backspace")
  )
  ->Js_array.push(context.subscriptions)
  ->ignore;
};