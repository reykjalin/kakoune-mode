module KakouneMode

open Fable.Core

open Node
open Rpc
open VSCode
open VSCodeTypes

let hello (name: string) = "Hello " + name

type Mode =
    | Normal
    | Insert

type IHelpers =
    abstract overrideTypeCommand: IVSCodeExtensionContext * (unit -> Mode) * (string -> unit) -> unit
    abstract setCursorStyleToBlock: IVScodeTextEditor -> unit
    abstract registerWindowChangeEventHandler: (string -> unit) -> unit

[<ImportAll("./extension-helpers.js")>]
let helpers: IHelpers = jsNative

let createMode mode =
    match mode with
    | "insert" -> Insert
    | _ -> Normal

let mutable mode = createMode "normal"

let getMode (_: unit) = mode

let parseDrawStatusCommand command = mode <- createMode (rpc.getMode command)

let parseDrawCommand command =
    match mode with
    | Normal -> doDraw command
    | Insert -> ()

let rec parseCommands (commands: string list) =
    match commands with
    | head :: tail ->
        (match rpc.getMethod (head) with
         | "draw_status" -> parseDrawStatusCommand head
         | "draw" -> parseDrawCommand head
         | _ -> ()

         parseCommands tail)
    | [] -> ()

let separateCommands (cmd: string) =
    cmd.Split [| '\n' |]
    |> Array.toList
    |> List.filter (fun cmd -> not (cmd.Equals("")))
    |> parseCommands

let handleCommand command =
    let commandStr = sprintf "%s" command
    printfn "%s" commandStr

    separateCommands commandStr

let handleError error = showError (sprintf "Kakoune mode encountered an error: %s" error)

startKakoune handleCommand handleError activeTextEditor.document.fileName |> ignore

let sendEscape (_: unit) =
    let msg = rpc.stringify (createKeysMessage "<esc>")
    writeToKak msg

let sendBackspace (_: unit) =
    match mode with
    | Insert -> vscode.commands.executeCommand ("deleteLeft", ResizeArray([]))
    | _ -> ()

    let msg = rpc.stringify (createKeysMessage "<backspace>")
    writeToKak msg

let activate (context: IVSCodeExtensionContext) =
    helpers.setCursorStyleToBlock activeTextEditor
    helpers.overrideTypeCommand (context, getMode, writeToKak)

    let sendEscape = vscode.commands.registerCommand ("extension.send_escape", sendEscape)
    context.subscriptions.Add sendEscape
    let sendBackspace = vscode.commands.registerCommand ("extension.send_backspace", sendBackspace)
    context.subscriptions.Add sendBackspace

    helpers.registerWindowChangeEventHandler writeToKak

let deactivate (_: unit) = ()
