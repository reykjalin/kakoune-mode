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

module Decode = {
  let method = json => Json.Decode.(json |> field("method", string));

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

let findIndexOfCursor = (line: line, indexOfAtom, inclusive) => {
  switch (line->Belt.List.take(inclusive ? indexOfAtom + 1 : indexOfAtom)) {
  | Some(l) =>
    Some(
      l->Belt.List.reduce(0, (acc, a) => acc + (a.contents |> String.length)),
    )
  | None => None
  };
};

let findSelection = (lineNumber, line: line) => {
  let cursorAtom =
    ListUtil.findIndexOf(~l=line, ~f=atom => "black" === atom.face.fg, ());
  switch (cursorAtom) {
  | None => None
  | Some(ca) =>
    let selectionAtom =
      ListUtil.findIndexOf(~l=line, ~f=atom => "white" === atom.face.fg, ());

    switch (selectionAtom) {
    | None =>
      switch (findIndexOfCursor(line, ca, false)) {
      | None => None
      | Some(ci) =>
        let p = Vscode.Position.make(~line=lineNumber, ~character=ci);
        Some(Vscode.Selection.make(~anchor=p, ~active=p));
      }
    | Some(sa) =>
      switch (
        findIndexOfCursor(line, ca, false),
        findIndexOfCursor(line, sa, sa > ca),
      ) {
      | (Some(ci), Some(si)) =>
        let startPos = Vscode.Position.make(~line=lineNumber, ~character=ci);
        let endPos = Vscode.Position.make(~line=lineNumber, ~character=si);
        startPos |> Js.log;
        endPos |> Js.log;
        Some(Vscode.Selection.make(~anchor=endPos, ~active=startPos));
      | (_, _) => None
      }
    };
  };
};

let setCursor = selection => {
  switch (selection) {
  | Some(s) =>
    switch (Vscode.Window.activeTextEditor()) {
    | Some(ed) => ed->Vscode.TextEditor.setSelection(s)
    | None => ()
    }
  | None => ()
  };
};

let processCommand = msg => {
  switch (msg |> Json.parseOrRaise |> Decode.method) {
  | "draw" =>
    switch (Mode.getMode()) {
    | Mode.Normal =>
      "Perform draw" |> Js.log;
      switch (msg |> Json.parseOrRaise |> Decode.drawCommand) {
      | exception (Json.Decode.DecodeError(e)) => e |> Js.log
      | draw =>
        draw
        |> getLinesFromDraw
        |> List.mapi(findSelection)
        |> List.iter(setCursor)
      };
    | Mode.Insert => "Nothing to do" |> Js.log
    }
  | "draw_status" =>
    "process current mode" |> Js.log;

    switch (msg |> Json.parseOrRaise |> Decode.drawStatusCommand) {
    | dsc => getModeFromDrawStatus(dsc) |> Mode.setMode
    | exception (Json.Decode.DecodeError(e)) => e |> Js.log
    };
  | exception (Json.Decode.DecodeError(e)) => e |> Js.log
  | _ => ()
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
