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
    Some(
      Json.Decode.(json |> field("params", tuple3(list(line), face, face))),
    );
};

let getModeFromModeLine = modeLine => {
  switch (
    modeLine
    ->Belt.List.keep(atom => atom.contents |> Js.String.includes("insert"))
    ->Belt.List.length
  ) {
  | 0 => Mode.Normal
  | _ => Mode.Insert
  };
};

let getModeFromDrawStatus = drawStatusCommand => {
  let (_, modeLine, _) = drawStatusCommand;
  modeLine->getModeFromModeLine;
};

type searchDirection =
  | Left
  | Right;

type atomPosition = {
  line: int,
  index: int,
};

let (>>=) = (m, f) => {
  switch (m) {
  | None => None
  | Some(v) => v->f
  };
};
let return = t => Some(t);

let linesToSelections = lines => {
  let flatLines = lines->Belt.List.flatten;

  let getAtom = position => {
    lines->Belt.List.get(position.line) >>= Belt.List.get(_, position.index);
  };

  let atomPositionToPosition = atomPosition => {
    let character =
      (
        lines->Belt.List.get(_, atomPosition.line)
        >>= Belt.List.take(_, atomPosition.index)
        >>= (
          l =>
            return(
              l->Belt.List.reduce(0, (acc, a) =>
                acc + String.length(a.contents)
              ),
            )
        )
      )
      ->Belt.Option.getWithDefault(0);

    Vscode.Position.make(~line=atomPosition.line, ~character);
  };

  let hasSelection = (searchDirection, atomPosition) => {
    let indexToCheck =
      switch (searchDirection) {
      | Left => (i => return(i - 1))
      | Right => (i => return(i + 1))
      };

    (
      atomPosition->getAtom
      >>= (
        atom =>
          {
            flatLines->Array.of_list->Belt.Array.getIndexBy(a => a === atom);
          }
          >>= indexToCheck
          >>= (
            i =>
              flatLines->Belt.List.get(i)
              >>= (atom => return("blue" === atom.face.bg))
          )
      )
    )
    ->Belt.Option.getWithDefault(false);
  };

  let cursorAtomPositionsToSelections = atomPositions => {
    let positionsToSelection = ((start, end_)) => {
      Vscode.Selection.make(
        ~anchor=start->atomPositionToPosition,
        ~active=end_->atomPositionToPosition,
      );
    };

    let findSelectionAtomPositions = atomPosition => {
      let default = (atomPosition, atomPosition);

      switch (
        hasSelection(Left, atomPosition),
        hasSelection(Right, atomPosition),
      ) {
      | (true, false) =>
        let atomsBeforeCursorReversed =
          atomPosition->getAtom
          >>= (
            cursorAtom =>
              flatLines
              ->Array.of_list
              ->Belt.Array.getIndexBy(atom => atom === cursorAtom)
              >>= (
                cursorAtomIndex =>
                  flatLines->Belt.List.take(cursorAtomIndex)
                  >>= (
                    atomsBeforeCursor =>
                      return(atomsBeforeCursor->Belt.List.reverse)
                  )
              )
          );

        let atomBeforeCursor =
          atomsBeforeCursorReversed
          >>= (l => return(l->Belt.List.reverse) >>= Belt.List.head);

        let anchorAtom =
          (
            switch (
              atomsBeforeCursorReversed
              >>= Belt.List.getBy(_, atom =>
                    "default" === atom.face.fg && "default" === atom.face.bg
                  )
            ) {
            | None =>
              atomsBeforeCursorReversed
              >>= Belt.List.getBy(_, a => {
                    Belt.Option.isSome(atomBeforeCursor)
                    && Belt.Option.eq(Some(a), atomBeforeCursor, (===))
                  })
              >>= (
                selectionStartAtom =>
                  flatLines
                  ->Array.of_list
                  ->Belt.Array.getIndexBy(atom => atom === selectionStartAtom)
              )
            | Some(a) =>
              flatLines
              ->Array.of_list
              ->Belt.Array.getIndexBy(atom => atom === a)
              >>= (i => return(i + 1))
            }
          )
          >>= (i => flatLines->Belt.List.get(i));

        let anchorLine =
          anchorAtom
          >>= (
            anchorAtom =>
              lines
              ->Array.of_list
              ->Belt.Array.getIndexBy(line =>
                  line->Belt.List.has(anchorAtom, (===))
                )
          );
        let anchorIndex =
          anchorAtom
          >>= (
            anchorAtom =>
              lines->(lines => anchorLine >>= lines->Belt.List.get)
              >>= (
                line =>
                  return(line->Array.of_list)
                  >>= Belt.Array.getIndexBy(_, atom => atom === anchorAtom)
              )
          );

        switch (anchorLine, anchorIndex) {
        | (Some(line), Some(index)) => ({line, index}, atomPosition)
        | _ => default
        };
      | (false, true) =>
        let atomsAfterCursor =
          atomPosition->getAtom
          >>= (
            cursorAtom =>
              flatLines
              ->Belt.List.reverse
              ->Array.of_list
              ->Belt.Array.getIndexBy(atom => atom === cursorAtom)
              >>= (
                cursorAtomReverseIndex =>
                  flatLines
                  ->Belt.List.reverse
                  ->Belt.List.take(cursorAtomReverseIndex)
                  >>= (
                    atomsAfterCursorReversed =>
                      return(atomsAfterCursorReversed->Belt.List.reverse)
                  )
              )
          );

        let anchorAtom =
          atomsAfterCursor
          >>= (
            l =>
              {
                switch (
                  l->Belt.List.getBy(atom =>
                    "default" === atom.face.fg && "default" === atom.face.bg
                  )
                ) {
                | None =>
                  l
                  ->Belt.List.reverse
                  ->Belt.List.getBy(atom =>
                      Belt.Option.eq(Some(atom), l->Belt.List.head, (===))
                    )
                  >>= (
                    a =>
                      flatLines
                      ->Array.of_list
                      ->Belt.Array.getIndexBy(atom => atom === a)
                  )
                | Some(a) =>
                  flatLines
                  ->Array.of_list
                  ->Belt.Array.getIndexBy(atom => atom === a)
                };
              }
              >>= (
                anchorAtomIndex => flatLines->Belt.List.get(anchorAtomIndex)
              )
          );

        let anchorLine =
          anchorAtom
          >>= (
            anchorAtom => {
              lines
              ->Array.of_list
              ->Belt.Array.getIndexBy(l =>
                  l->Belt.List.has(anchorAtom, (===))
                );
            }
          );

        let anchorIndex =
          anchorAtom
          >>= (
            anchorAtom => {
              lines->(lines => anchorLine >>= lines->Belt.List.get)
              >>= (
                line =>
                  return(line->Array.of_list)
                  >>= Belt.Array.getIndexBy(_, atom => atom === anchorAtom)
              );
            }
          );

        switch (anchorLine, anchorIndex) {
        | (Some(line), Some(index)) => ({line, index}, atomPosition)
        | _ => default
        };
      | _ => (atomPosition, atomPosition)
      };
    };

    atomPositions
    ->Belt.List.map(findSelectionAtomPositions)
    ->Belt.List.map(positionsToSelection);
  };

  let findCursorAtomPositionsInLine = (line, lineNumber) => {
    line
    ->Belt.List.mapWithIndex((i, a) => (i, a))
    ->Belt.List.keep(((_i, a)) =>
        "white" === a.face.bg || "cyan" === a.face.bg
      )
    ->Belt.List.map(((i, _a)) => {line: lineNumber, index: i});
  };

  let cursorPositions =
    lines
    ->Belt.List.mapWithIndex((i, l) => l->findCursorAtomPositionsInLine(i))
    ->Belt.List.flatten;

  return(cursorPositions->cursorAtomPositionsToSelections->Array.of_list);
};

let getLinesFromDraw = (drawCommand: drawCommand) => {
  let (lines, _defaultFace, _paddingFace) = drawCommand;
  return(lines);
};

let processCommand = msg => {
  switch (msg->Json.parseOrRaise->Decode.method) {
  | "draw" =>
    switch (Mode.getMode()) {
    | Mode.Normal =>
      (
        msg->Json.parse
        >>= Decode.drawCommand
        >>= getLinesFromDraw
        >>= linesToSelections
      )
      ->Belt.Option.getWithDefault([||])
      ->Vscode.setSelections
    | Mode.Insert => () // Nothing to do.
    }
  | "draw_status" =>
    switch (msg->Json.parseOrRaise->Decode.drawStatusCommand) {
    | dsc => getModeFromDrawStatus(dsc)->Mode.setMode
    | exception (Json.Decode.DecodeError(e)) => e->Js.log
    }
  | exception (Json.Decode.DecodeError(e)) => e->Js.log
  | _ => ()
  };
};

let handleIncomingError = error => error->Bytes.to_string->Js.log;

let handleIncomingCommand = command =>
  command
  ->Bytes.to_string
  ->String.split_on_char('\n', _)
  ->Belt.List.forEach(processCommand);

let kak = ref(Node.spawn(":", [||]));

let getKak = () => kak^;

let setKak = newKak => kak := newKak;

let writeToKak = message => getKak().stdin.write(. message);
