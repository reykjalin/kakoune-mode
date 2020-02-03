module VSCodeTypes

open Fable.Core.JS
open Fable.Core

type IVSCodePosition =
    abstract character: int with get, set
    abstract line: int with get, set

type IVSCodePositionStatic =
    [<Emit("new $0($1, $2)")>]
    abstract Create: int * int -> IVSCodePosition

type IVSCodeSelection =
    abstract active: IVSCodePosition
    abstract anchor: IVSCodePosition
    abstract ``end``: IVSCodePosition
    abstract start: IVSCodePosition

type IVScodeSelectionStatic =
    [<Emit("new $0($1, $2)")>]
    abstract Create: IVSCodePosition * IVSCodePosition -> IVSCodeSelection

type IVSCodeUri =
    abstract path: string

type IVSCodeTextEdit =
    abstract newText: string

type IVSCodeTextEditStatic =
    [<Emit("$0.replace($1, $2)")>]
    abstract insert: IVSCodePosition * string -> IVSCodeTextEdit

type IVSCodeWorkspaceEdit =
    abstract set: IVSCodeUri * IVSCodeTextEdit list -> unit

type IVSCodeWorkspaceEditStatic =
    [<Emit("new $0()")>]
    abstract Create: unit -> IVSCodeWorkspaceEdit

type IVSCodeWorkspace =
    abstract name: string with get, set
    abstract rootPath: string with get, set

    abstract applyEdit: string -> Promise<bool>

type IVSCodeTextLine =
    abstract text: string

type IVSCodeTextDocument =
    abstract lineAt: int -> IVSCodeTextLine

type IVScodeTextEditor =
    abstract selection: IVSCodeSelection with get, set
    abstract selections: IVSCodeSelection list with get, set
    abstract document: IVSCodeTextDocument

type IVSCodeWindow =
    abstract showErrorMessage: message:string -> unit
    abstract activeTextEditor: IVScodeTextEditor with get, set

type IVSCode =
    abstract window: IVSCodeWindow with get, set
