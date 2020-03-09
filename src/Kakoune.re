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

type drawStatusCommand = (
  line /* status_line */,
  line /* mode_line */,
  face /* default_face */,
);
type drawCommand = (
  list(line) /* lines */,
  face /* default_face */,
  face /* padding_face */,
);

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
    Json.Decode.(json |> field("params", tuple3(line, line, face)));

  let drawCommand = json =>
    Json.Decode.(json |> field("params", tuple3(list(line), face, face)));
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

let getModeFromDrawStatus = drawStatusCommand => {
  let (_, modeLine, _) = drawStatusCommand;
  modeLine |> getModeFromModeLine;
};

let getLinesFromDraw = (drawCommand: drawCommand) => {
  let (lines, _defaultFace, _paddingFace) = drawCommand;
  lines;
};

let rec findIndexOfAtom =
        (~line: line, ~test: atom => bool, ~index: int=0, ()): option(int) =>
  index >= List.length(line)
    ? None
    : {
      let atom = index |> List.nth(line);
      test(atom)
        ? Some(index) : findIndexOfAtom(~line, ~test, ~index=index + 1, ());
    };

let findIndexOfCursor = (line: line, indexOfAtom) => {
  switch (line->Belt.List.take(indexOfAtom)) {
  | Some(l) =>
    Some(
      l->Belt.List.reduce(0, (acc, a) => acc + (a.contents |> String.length)),
    )
  | None => None
  };
};

let findCursor = (row, line: line) => {
  switch (findIndexOfAtom(~line, ~test=atom => "black" === atom.face.fg, ())) {
  | Some(i) =>
    switch (findIndexOfCursor(line, i)) {
    | Some(character) => Some(Vscode.Position.make(~line=row, ~character))
    | None => None
    }
  | None => None
  };
};

let setCursor = position => {
  switch (position) {
  | Some(p) =>
    switch (Vscode.Window.activeTextEditor()) {
    | Some(ed) =>
      ed->Vscode.TextEditor.setSelection(
        Vscode.Selection.make(~anchor=p, ~active=p),
      )
    | None => ()
    }
  | None => ()
  };
};

let processCommand = msg => {
  switch (msg |> Json.parseOrRaise |> Decode.msg |> getMethod) {
  | "draw" =>
    switch (Mode.getMode()) {
    | Mode.Normal =>
      "Perform draw" |> Js.log;
      switch (msg |> Json.parseOrRaise |> Decode.drawCommand) {
      | exception (Json.Decode.DecodeError(e)) =>
        e |> Js.log;
        [];
      | draw =>
        draw
        |> getLinesFromDraw
        |> List.mapi(findCursor)
        |> List.map(setCursor)
      };
    | Mode.Insert =>
      "Nothing to do" |> Js.log;
      [];
    }
  | "draw_status" =>
    "process current mode" |> Js.log;

    switch (msg |> Json.parseOrRaise |> Decode.drawStatusCommand) {
    | dsc =>
      getModeFromDrawStatus(dsc) |> Mode.setMode;
      [];
    | exception (Json.Decode.DecodeError(e)) =>
      Js.log(e);
      [];
    };
  | exception (Json.Decode.DecodeError(e)) =>
    Js.log(e);
    [];
  | _ =>
    Js.log("other");
    [];
  };
};
let handleIncomingError = error => error |> Bytes.to_string |> Js.log;

let handleIncomingCommand = command =>
  command
  |> Bytes.to_string
  |> String.split_on_char('\n')
  |> List.map(processCommand)
  |> ignore;

let kak = ref(Node.spawn(":", [||]));

let getKak = () => kak^;

let setKak = newKak => kak := newKak;

let writeToKak = message => getKak().stdin.write(. message);
