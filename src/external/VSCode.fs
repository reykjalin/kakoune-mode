module VSCode

open Fable.Core.JsInterop
open Fable.Import
open Fable.Core

type IVSCodeWindow =
    abstract showErrorMessage: message:string -> unit

type IVSCode =
    abstract window: IVSCodeWindow with get, set

[<Import("*", "vscode")>]
let vscode: IVSCode = jsNative

let window = vscode.window
