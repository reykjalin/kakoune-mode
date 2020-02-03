module KakouneMode

open Rpc
open VSCode

let hello (name: string) = "Hello " + name

type Mode =
    | Normal
    | Insert

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
