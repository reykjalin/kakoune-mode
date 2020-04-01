type keysMessage = {
  jsonrpc: string,
  method: string,
  params: list(string),
};

let createKeysMessage = keys => {
  let correctedKeys = keys |> Js.String.replace("\n", "<ret>");
  {jsonrpc: "2.0", method: "keys", params: [correctedKeys]};
};

module Encode = {
  let keysMessage = km =>
    Json.Encode.(
      object_([
        ("jsonrpc", string(km.jsonrpc)),
        ("method", string(km.method)),
        ("params", list(string, km.params)),
      ])
    );
};

let stringifyMessage = msg => {
  msg->Encode.keysMessage->Json.stringify;
};
