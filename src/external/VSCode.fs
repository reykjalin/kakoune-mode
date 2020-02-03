module VSCode

open Fable.Core
open Thoth.Json

open Rpc
open Kakoune

type IVSCodePosition =
    abstract character: int with get, set
    abstract line: int with get, set

type IVSCodePositionStatic =
    [<Emit("new $0($1, $2)")>]
    abstract Create: int * int -> IVSCodePosition

[<Import("Position", "vscode")>]
let Position: IVSCodePositionStatic = jsNative

type IVSCodeSelection =
    abstract active: IVSCodePosition
    abstract anchor: IVSCodePosition
    abstract ``end``: IVSCodePosition
    abstract start: IVSCodePosition

type IVScodeSelectionStatic =
    [<Emit("new $0($1, $2)")>]
    abstract Create: IVSCodePosition * IVSCodePosition -> IVSCodeSelection

[<Import("Selection", "vscode")>]
let Selection: IVScodeSelectionStatic = jsNative

type IVScodeTextEditor =
    abstract selection: IVSCodeSelection with get, set
    abstract selections: IVSCodeSelection list with get, set

type IVSCodeWindow =
    abstract showErrorMessage: message:string -> unit
    abstract activeTextEditor: IVScodeTextEditor

type IVSCode =
    abstract window: IVSCodeWindow with get, set

[<Import("*", "vscode")>]
let vscode: IVSCode = jsNative


let window = vscode.window

let showError (error: string) = window.showErrorMessage error

let findCursorInAtom (atom: Atom) =
    match atom.face.fg with
    | "white" -> ()
    | _ -> ()

let atomHasCursor (atom: Atom) = atom.face.fg = "black"

let rec findCursorInLine (line: Line) =
    match List.tryFindIndex atomHasCursor line with
    | Some value ->
        line
        |> List.toSeq
        |> Seq.truncate value
        |> Seq.sumBy (fun a -> String.length a.contents)
        |> Some
    | None -> None

let rec findCursor (lines: Line list) =
    match lines with
    | head :: tail ->
        match findCursorInLine head with
        | Some value -> Some value
        | None -> findCursor tail
    | [] -> None

let drawSelections (command: string) =
    // showError (rpc.getLines command)
    match (Decode.Auto.fromString<Line list> (rpc.getLines command)) with
    | Ok o ->
        match findCursor o with
        | Some value ->
            let pos = Position.Create(0, value)
            let sel = Selection.Create(pos, pos)
            window.activeTextEditor.selection <- sel
        | None -> showError "no cursor found"
    | Error e -> showError e

let drawMissingLines (command: string) = ()

let doDraw (command: string) =
    drawSelections command
    drawMissingLines command
