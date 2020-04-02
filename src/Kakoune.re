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

module Line = {
  let (>>=) = (m, f) => {
    switch (m) {
    | None => None
    | Some(v) => f(v)
    };
  };
  let return = t => Some(t);

  let listToArray = l => return(l->Array.of_list);

  let getAtomsBeforeAtomIndex = (line, atomIndex) => {
    switch (line, atomIndex) {
    | (Some(l), Some(i)) => l->Belt.List.take(i)
    | _ => None
    };
  };

  let getAtomIndexBy = (line, f) => {
    line >>= listToArray >>= Belt.Array.getIndexBy(_, f);
  };

  let getAtomsBeforeAtom = (line, a) => {
    line
    ->getAtomIndexBy(atom => Belt.Option.eq(Some(atom), a, (===)))
    ->getAtomsBeforeAtomIndex(line, _);
  };

  let getAtomBy = (line, f) => {
    line >>= Belt.List.getBy(_, f);
  };

  let reverse = line => {
    line >>= (l => return(l->Belt.List.reverse));
  };

  let getText = line => {
    line
    >>= (
      line => {
        return(
          line->Belt.List.reduce("", (lineText, atom) =>
            lineText ++ atom.contents
          ),
        );
      }
    );
  };

  let getNumberOfAtoms = line => {
    line >>= (l => return(l->Belt.List.length));
  };

  let getLineLength = line => {
    line
    >>= (
      line => {
        return(
          line->Belt.List.reduce(0, (lineLength, atom) =>
            lineLength + String.length(atom.contents)
          ),
        );
      }
    );
  };
};

module Document = {
  type t = list(line);

  let (>>=) = (m, f) => {
    switch (m) {
    | None => None
    | Some(v) => v->f
    };
  };

  let getLine = (lines: t, lineNumber) =>
    lineNumber >>= Belt.List.get(lines);

  let getLineBy = (lines: t, f) => lines->Belt.List.getBy(f);

  let getLineThatHasAtom = (lines: t, atom) => {
    atom
    >>= (
      a => {
        lines->getLineBy(l => l->Belt.List.has(a, (===)));
      }
    );
  };

  let getLineIndexBy = (lines: t, f) =>
    lines->Array.of_list->Belt.Array.getIndexBy(f);
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
  atom: int,
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
    lines->Belt.List.get(position.line) >>= Belt.List.get(_, position.atom);
  };

  let makePosition = atomPosition => {
    let character =
      lines
      ->Belt.List.get(atomPosition.line)
      ->Line.getAtomsBeforeAtomIndex(Some(atomPosition.atom))
      ->Line.getLineLength
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
        atom => {
          flatLines->Some->Line.getAtomIndexBy(a => a === atom);
        }
      )
      >>= indexToCheck
      >>= (
        i =>
          flatLines->Belt.List.get(i)
          >>= (atom => return("blue" === atom.face.bg))
      )
    )
    ->Belt.Option.getWithDefault(false);
  };

  let cursorAtomPositionsToSelections = atomPositions => {
    let makeSelection = ((start, end_)) => {
      Vscode.Selection.make(
        ~anchor=makePosition(start),
        ~active=makePosition(end_),
      );
    };

    let findSelectionAtomPositions = cursorPosition => {
      let getAtomPositionWithDefault = (atom, default) => {
        let startLine = lines->Document.getLineThatHasAtom(atom);
        let lineIndex =
          startLine
          >>= (line => lines->Document.getLineIndexBy(l => l === line));
        let atomIndex =
          startLine->Line.getAtomIndexBy(a => {
            Belt.Option.eq(a->Some, atom, (===))
          });
        switch (lineIndex, atomIndex) {
        | (Some(line), Some(atom)) => {line, atom}
        | _ => default
        };
      };

      switch (
        hasSelection(Left, cursorPosition),
        hasSelection(Right, cursorPosition),
      ) {
      | (true, false) =>
        let flatLines = Belt.List.flatten(lines)->Some;
        let atomsBeforeSelectionReversed =
          flatLines
          ->Line.getAtomsBeforeAtom(cursorPosition->getAtom)
          ->Line.reverse;

        atomsBeforeSelectionReversed
        ->Line.getAtomBy(atom => "blue" !== atom.face.bg)
        ->Line.getAtomsBeforeAtom(atomsBeforeSelectionReversed, _)
        ->Line.reverse
        ->Line.getAtomBy(atom => "blue" === atom.face.bg)
        /* Select first possible atom by default. */
        ->getAtomPositionWithDefault({line: 0, atom: 0})
        ->(startPosition => (startPosition, cursorPosition));
      | (false, true) =>
        let flatLines = Belt.List.flatten(lines)->Some;
        let atomsAfterCursor =
          flatLines
          ->Line.reverse
          ->Line.getAtomsBeforeAtom(cursorPosition->getAtom)
          ->Line.reverse;

        let lastAtomPosition =
          lines
          ->Belt.List.length
          ->(
              numberOfLines =>
                lines->Document.getLine(Some(numberOfLines - 1))
            )
          ->Line.getLineLength
          /* Defaults to -1 since we have to add one later on. */
          ->Belt.Option.getWithDefault(-1);

        let startPosition =
          atomsAfterCursor
          ->Line.getAtomBy(atom => "blue" !== atom.face.bg)
          ->Line.getAtomsBeforeAtom(atomsAfterCursor, _)
          ->Line.reverse
          ->Line.getAtomBy(atom => "blue" === atom.face.bg)
          ->getAtomPositionWithDefault({
              /* Select the last possible atom by default. */
              line: lines->Belt.List.length - 1,
              atom: lastAtomPosition,
            })
          /*
           * Select 1 atom further to make sure the last atom's text is included
           * when getting the correct character position.
           */
          ->(pos => {...pos, atom: pos.atom + 1});

        (startPosition, cursorPosition);
      | (true, true) =>
        /**
         * Weird edge case where e.g. you select words side-by side.
         *
         * Example:
         * Text in document is: `hello hello world`.
         * Search for `hello `.
         *
         * To handle this, figure out where selections are going (backwards vs forwards)
         * and use the appropriate method to split the selections correctly.
         */
        (cursorPosition, cursorPosition)
      | _ => (cursorPosition, cursorPosition)
      };
    };

    atomPositions
    ->Belt.List.map(findSelectionAtomPositions)
    ->Belt.List.map(makeSelection);
  };

  let findCursorAtomPositionsInLine = (line, lineNumber) => {
    line
    ->Belt.List.mapWithIndex((i, a) => (i, a))
    ->Belt.List.keep(((_i, a)) =>
        "white" === a.face.bg || "cyan" === a.face.bg
      )
    ->Belt.List.map(((i, _a)) => {line: lineNumber, atom: i});
  };

  let cursorPositions =
    lines
    ->Belt.List.mapWithIndex((i, l) => l->findCursorAtomPositionsInLine(i))
    ->Belt.List.flatten;

  return(cursorPositions->cursorAtomPositionsToSelections->Array.of_list);
};

let getLinesFromDraw = (drawCommand: option(drawCommand)) => {
  drawCommand
  >>= (
    drawCommand => {
      let (lines, _defaultFace, _paddingFace) = drawCommand;
      return(lines);
    }
  );
};

let processCommand = msg => {
  switch (msg->Json.parseOrRaise->Decode.method) {
  | "draw" =>
    switch (Mode.getMode()) {
    | Mode.Normal =>
      (msg->Json.parse >>= Decode.drawCommand)
      ->(drawCommand => getLinesFromDraw(drawCommand) >>= linesToSelections)
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

let handleIncomingError = error =>
  error->Bytes.to_string->Vscode.Window.showError;

let handleIncomingCommand = command =>
  command
  ->Bytes.to_string
  ->String.split_on_char('\n', _)
  ->Belt.List.forEach(processCommand);

let kak = ref(Node.spawn(":", [||]));

let getKak = () => kak^;

let setKak = newKak => kak := newKak;

let writeToKak = message => getKak().stdin.write(. message);
