module Rpc

open Fable.Core.JsInterop
open Fable.Import
open Fable.Core

type KeysRpcMessage = string list

type RpcMessage =
    { jsonrpc: string
      method: string
      ``params``: KeysRpcMessage }

type IRpc =
    abstract getMethod: rpcMsg:string -> string
    abstract getMode: rpcMsg:string -> string

[<ImportAll("./rpc.js")>]
let rpc: IRpc = jsNative

let create method ``params`` =
    { jsonrpc = "2.0"
      method = method
      ``params`` = ``params`` }

let createKeysMessage (keys: string) =
    let correctedKeys = keys.Replace("\n", "<ret>")
    create "keys" [ correctedKeys ]
