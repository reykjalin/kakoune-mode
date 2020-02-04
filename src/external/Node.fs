module Node

open Fable.Core
open Fable.Core.JsInterop

type INodeProcessStream =
    abstract on: string * (string -> unit) -> unit
    abstract write: string -> unit

type INodeChildProcess =
    abstract stdout: INodeProcessStream
    abstract stderr: INodeProcessStream
    abstract stdin: INodeProcessStream

type INodeChildProcessStatic =
    abstract spawn: string * string array -> INodeChildProcess

[<ImportAll("child_process")>]
let childProcess: INodeChildProcessStatic = jsNative

let mutable kak: INodeChildProcess = childProcess.spawn ("kak", [| "-clear" |])

let startKakoune handleCommand handleError currentFile =
    kak <- childProcess.spawn ("kak", [| "-ui"; "json"; "-s"; "vscode"; currentFile |])
    kak.stdout.on ("data", handleCommand) |> ignore
    kak.stderr.on ("data", handleError) |> ignore
    kak

let writeToKak msg = kak.stdin.write msg
