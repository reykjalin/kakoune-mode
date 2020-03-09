type stream_data = {data: bytes};

type process_stream = {
  on: (. string, bytes => unit) => unit,
  write: (. string) => unit,
};

type child_process = {
  stdout: process_stream,
  stderr: process_stream,
  stdin: process_stream,
};

[@bs.module "child_process"]
external spawn: (string, array(string)) => child_process = "spawn";
