let rec findIndexOf = (~l: list('a), ~f: 'a => bool, ~i: int=0, ()) => {
  i >= List.length(l)
    ? None
    : {
      l->List.nth(i)->f ? Some(i) : findIndexOf(~l, ~f, ~i=i + 1, ());
    };
};
