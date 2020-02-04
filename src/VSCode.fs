module VSCode

open Fable.Core
open Fable.Core.JS
open Thoth.Json

open Rpc
open Kakoune
open VSCodeTypes

[<Import("Position", "vscode")>]
let Position: IVSCodePositionStatic = jsNative

[<Import("Selection", "vscode")>]
let Selection: IVScodeSelectionStatic = jsNative

[<Import("TextEdit", "vscode")>]
let TextEdit: IVSCodeTextEditStatic = jsNative

[<Import("WorkspaceEdit", "vscode")>]
let WorkspaceEdit: IVSCodeWorkspaceEditStatic = jsNative

[<Import("*", "vscode")>]
let vscode: IVSCode = jsNative

let window = vscode.window
let activeTextEditor = window.activeTextEditor

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

let getLines command = Decode.Auto.fromString<Line list> (rpc.getLines command)

let drawSelections (command: string) =
    // showError (rpc.getLines command)
    match getLines command with
    | Ok o ->
        match findCursor o with
        | Some value ->
            let pos = Position.Create(0, value)
            let sel = Selection.Create(pos, pos)
            window.activeTextEditor.selection <- sel
        | None -> showError "no cursor found"
    | Error e -> showError e

let textAtLine (i: int): string = (activeTextEditor.document.lineAt i).text

let drawMissingLines (command: string) =
    match getLines command with
    | Ok o ->
        showError "Drawing missing lines"
        o
        |> List.mapi (fun i l ->
            let text =
                l
                |> List.map (fun l -> l.contents)
                |> List.fold (+) ""

            showError (sprintf "from kak: '%s'" text)
            showError (sprintf "from code: '%s'" (textAtLine i))

            if ((textAtLine i) <> text) then showError "different" else showError "same")
        |> ignore
    | Error e -> showError e

let doDraw (command: string) =
    drawMissingLines command
    drawSelections command
