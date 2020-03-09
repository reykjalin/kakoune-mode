/**
 * Finds the index of the item in list `l` that fulfills the condition `f`.
 */
let rec findIndexOf = (~l: list('a), ~f: 'a => bool, ~i: int=0, ()) => {
  i >= List.length(l)
    ? None
    : {
      l->List.nth(i)->f ? Some(i) : findIndexOf(~l, ~f, ~i=i + 1, ());
    };
};
