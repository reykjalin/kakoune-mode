module Kakoune

type Color = Color of string

type Attribute = Attribute of string

type Face =
    { fg: string
      bg: string
      attributes: string list }

type Atom =
    { face: Face
      contents: string }

type Line = Atom list
