module VSCode

open Fable.Core
open Thoth.Json

open Rpc
open Kakoune

type IVSCodeWindow =
    abstract showErrorMessage: message:string -> unit

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
    showError (rpc.getLines command)
    match (Decode.Auto.fromString<Line list> (rpc.getLines command)) with
    | Ok o ->
        match findCursor o with
        | Some value -> showError (sprintf "Cursor is at %d" value)
        | None -> showError "no cursor found"
    | Error e -> showError e

let drawMissingLines (command: string) = ()

let doDraw (command: string) =
    drawSelections command
    drawMissingLines command
