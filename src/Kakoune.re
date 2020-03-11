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

type searchDirection =
  | Left
  | Right;

type atomPosition = {
  line: int,
  index: int,
};

let linesToSelections = lines => {
  let flatLines = lines->List.flatten;

  let getAtom = position => {
    switch (lines->List.nth(position.line)->List.nth(position.index)) {
    | exception (Invalid_argument(_)) => None
    | exception (Failure(_)) => None
    | a => Some(a)
    };
  };

  let atomPositionToPosition = atomPosition => {
    switch (
      lines->List.nth(atomPosition.line)->Belt.List.take(atomPosition.index)
    ) {
    | None => Vscode.Position.make(~line=atomPosition.line, ~character=0)
    | Some(l) =>
      let character =
        l |> List.fold_left((acc, a) => acc + String.length(a.contents), 0);
      Vscode.Position.make(~line=atomPosition.line, ~character);
    };
  };

  let hasSelection = (searchDirection, atomPosition) => {
    let atom =
      switch (getAtom(atomPosition)) {
      | None => None
      | Some(atom) => Some(atom)
      };
    let atomIndex =
      switch (atom) {
      | None => None
      | Some(atom) =>
        switch (
          flatLines->Array.of_list->Belt.Array.getIndexBy(a => a === atom)
        ) {
        | None => None
        | Some(index) => Some(index)
        }
      };
    switch (searchDirection, atomIndex) {
    | (Left, Some(i)) =>
      switch ("blue" === flatLines->List.nth(i - 1).face.bg) {
      | exception (Failure(_)) => false
      | exception (Invalid_argument(_)) => false
      | result => result
      }
    | (Right, Some(i)) =>
      switch ("blue" === flatLines->List.nth(i + 1).face.bg) {
      | exception (Failure(_)) => false
      | exception (Invalid_argument(_)) => false
      | result => result
      }
    | _ => false
    };
  };

  let cursorAtomPositionsToSelections = atomPositions => {
    let positionsToSelection = ((start, end_)) => {
      Vscode.Selection.make(
        ~anchor=start |> atomPositionToPosition,
        ~active=end_ |> atomPositionToPosition,
      );
    };

    let findSelectionAtomPositions = atomPosition => {
      let default = (atomPosition, atomPosition);

      switch (
        hasSelection(Left, atomPosition),
        hasSelection(Right, atomPosition),
      ) {
      | (true, false) =>
        let cursorAtomIndex =
          switch (getAtom(atomPosition)) {
          | None => None
          | Some(a) =>
            flatLines->Array.of_list->Belt.Array.getIndexBy(atom => atom === a)
          };

        let atomsBeforeCursor =
          switch (cursorAtomIndex) {
          | None => None
          | Some(i) => flatLines->Belt.List.take(i)
          };

        let atomIndexWhereSelectionStarts =
          switch (atomsBeforeCursor) {
          | None => None
          | Some(l) =>
            switch (
              l
              |> List.rev
              |> List.find(a =>
                   "default" === a.face.fg && "default" === a.face.bg
                 )
            ) {
            | exception Not_found =>
              switch (l |> List.rev |> List.find(a => a === l->List.hd)) {
              | exception (Failure(_)) => None
              | a =>
                switch (
                  flatLines
                  ->Array.of_list
                  ->Belt.Array.getIndexBy(atom => atom === a)
                ) {
                | None => None
                | Some(i) => Some(i)
                }
              }
            | a =>
              switch (
                flatLines
                ->Array.of_list
                ->Belt.Array.getIndexBy(atom => atom === a)
              ) {
              | None => None
              | Some(i) => Some(i + 1)
              }
            }
          };

        let selectionAnchorAtom =
          switch (atomIndexWhereSelectionStarts) {
          | None => None
          | Some(i) =>
            switch (flatLines->Array.of_list[i]) {
            | exception (Invalid_argument(_)) => None
            | a => Some(a)
            }
          };

        let anchorLine =
          switch (selectionAnchorAtom) {
          | None => None
          | Some(atom) =>
            lines
            ->Array.of_list
            ->Belt.Array.getIndexBy(l => l |> List.exists(a => a === atom))
          };

        let anchorIndex =
          switch (anchorLine, selectionAnchorAtom) {
          | (Some(line), Some(atom)) =>
            lines
            ->List.nth(line)
            ->Array.of_list
            ->Belt.Array.getIndexBy(a => a === atom)
          | _ => None
          };

        switch (anchorLine, anchorIndex) {
        | (Some(line), Some(index)) => ({line, index}, atomPosition)
        | _ => default
        };
      | (false, true) =>
        let flatLines = lines->List.flatten;
        let cursorAtomReverseIndex =
          switch (getAtom(atomPosition)) {
          | None => None
          | Some(a) =>
            flatLines
            ->List.rev
            ->Array.of_list
            ->Belt.Array.getIndexBy(atom => atom === a)
          };

        let atomsAfterCursor =
          switch (cursorAtomReverseIndex) {
          | None => None
          | Some(i) =>
            switch (flatLines->List.rev->Belt.List.take(i)) {
            | None => None
            | Some(l) => Some(l->List.rev)
            }
          };

        let atomIndexWhereSelectionEnds =
          switch (atomsAfterCursor) {
          | None => None
          | Some(l) =>
            switch (
              l
              |> List.find(a =>
                   "default" === a.face.fg && "default" === a.face.bg
                 )
            ) {
            | exception Not_found =>
              switch (l |> List.rev |> List.find(a => a === l->List.hd)) {
              | exception (Failure(_)) => None
              | a =>
                switch (
                  flatLines
                  ->Array.of_list
                  ->Belt.Array.getIndexBy(atom => atom === a)
                ) {
                | None => None
                | Some(i) => Some(i)
                }
              }
            | a =>
              switch (
                flatLines
                ->Array.of_list
                ->Belt.Array.getIndexBy(atom => atom === a)
              ) {
              | None => None
              | Some(i) => Some(i)
              }
            }
          };

        let selectionAnchorAtom =
          switch (atomIndexWhereSelectionEnds) {
          | None => None
          | Some(i) =>
            switch (flatLines->Array.of_list[i]) {
            | exception (Invalid_argument(_)) => None
            | a => Some(a)
            }
          };

        let anchorLine =
          switch (selectionAnchorAtom) {
          | None => None
          | Some(atom) =>
            lines
            ->Array.of_list
            ->Belt.Array.getIndexBy(l => l |> List.exists(a => a === atom))
          };

        let anchorIndex =
          switch (anchorLine, selectionAnchorAtom) {
          | (Some(line), Some(atom)) =>
            lines
            ->List.nth(line)
            ->Array.of_list
            ->Belt.Array.getIndexBy(a => a === atom)
          | _ => None
          };

        switch (anchorLine, anchorIndex) {
        | (Some(line), Some(index)) => ({line, index}, atomPosition)
        | _ => default
        };
      | _ => (atomPosition, atomPosition)
      };
    };

    atomPositions
    |> List.map(findSelectionAtomPositions)
    |> List.map(positionsToSelection);
  };

  let findCursorAtomPositionsInLine = (lineNumber, line: line) => {
    line
    |> List.mapi((i, a) => (i, a))
    |> List.filter(((_i, a)) =>
         "white" === a.face.bg || "cyan" === a.face.bg
       )
    |> List.map(((i, _a)) => {line: lineNumber, index: i});
  };

  let cursorPositions =
    lines
    |> List.mapi((i, l) => l |> findCursorAtomPositionsInLine(i))
    |> List.flatten;

  cursorPositions |> cursorAtomPositionsToSelections |> Array.of_list;
};

let getLinesFromDraw = (drawCommand: drawCommand) => {
  let (lines, _defaultFace, _paddingFace) = drawCommand;
  lines;
};

let processCommand = msg => {
  switch (msg |> Json.parseOrRaise |> Decode.method) {
  | "draw" =>
    switch (Mode.getMode()) {
    | Mode.Normal =>
      switch (msg |> Json.parseOrRaise |> Decode.drawCommand) {
      | exception (Json.Decode.DecodeError(e)) => e |> Js.log
      | draw =>
        draw |> getLinesFromDraw |> linesToSelections |> Vscode.setSelections
      }
    | Mode.Insert => () // Nothing to do.
    }
  | "draw_status" =>
    switch (msg |> Json.parseOrRaise |> Decode.drawStatusCommand) {
    | dsc => getModeFromDrawStatus(dsc) |> Mode.setMode
    | exception (Json.Decode.DecodeError(e)) => e |> Js.log
    }
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
