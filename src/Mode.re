type t =
  | Normal
  | Insert;

let mode = ref(Normal);

let setMode = newMode => mode := newMode;

let getMode = () => mode^;
