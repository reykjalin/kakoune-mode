let rec findIndexOf = (~l: list('a), ~f: 'a => bool, ~i: int=0, ()) => {
  i >= List.length(l)
    ? None
    : {
      i |> List.nth(l) |> f ? Some(i) : findIndexOf(~l, ~f, ~i=i + 1, ());
    };
};
