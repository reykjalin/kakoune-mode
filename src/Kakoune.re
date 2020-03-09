type color = string;
type attribute = string;
type face = {
  fg: color,
  bg: color,
  attributes: array(attribute),
};
type atom = {
  face,
  contents: string,
};
type line = list(atom);
type coord = {
  line: int,
  column: int,
};
type drawStatusCommand = {params: (line, line, face)};

type msg = {method: string};

module Decode = {
  let msg = json => Json.Decode.{method: json |> field("method", string)};

  let face = json =>
    Json.Decode.{
      fg: json |> field("fg", string),
      bg: json |> field("bg", string),
      attributes: json |> field("attributes", array(string)),
    };

  let atom = json =>
    Json.Decode.{
      face: json |> field("face", face),
      contents: json |> field("contents", string),
    };

  let coord = json =>
    Json.Decode.{
      line: json |> field("line", int),
      column: json |> field("column", int),
    };

  let line = json => Json.Decode.(json |> list(atom));

  let drawStatusCommand = json =>
    Json.Decode.{params: json |> field("params", tuple3(line, line, face))};
};

let getMethod = msg => msg.method;

let getModeFromModeLine = modeLine => {
  switch (
    modeLine
    |> List.filter(atom => atom.contents |> Js.String.includes("insert"))
    |> List.length
  ) {
  | 0 => Mode.Normal
  | _ => Mode.Insert
  };
};

let pmsg = msg => {
  switch (msg |> Json.parseOrRaise |> Decode.msg |> getMethod) {
  | "draw" => Js.log("do draw")
  | "draw_status" =>
    Js.log("process current mode");
    switch (msg |> Json.parseOrRaise |> Decode.drawStatusCommand) {
    | dsc =>
      let (_, modeLine, _) = dsc.params;
      getModeFromModeLine(modeLine) |> Mode.setMode;
    | exception (Json.Decode.DecodeError(e)) => Js.log(e)
    };
  | exception (Json.Decode.DecodeError(e)) => Js.log(e)
  | _ => Js.log("other")
  };
};
let handleIncomingError = error => error |> Bytes.to_string |> Js.log;

let handleIncomingCommand = command =>
  command |> Bytes.to_string |> String.split_on_char('\n') |> List.iter(pmsg);

let kak = ref(Node.spawn("kak", [|"-clear"|]));

let getKak = () => kak^;

let setKak = newKak => kak := newKak;

let writeToKak = message => getKak().stdin.write(. message);
