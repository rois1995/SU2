import SU2Source.Semantics

namespace SU2Source

/-- A deliberately closed grammar. No token or statement is skipped on failure. -/
def identifierChar (c : Char) : Bool :=
  c.toNat < 128 && (c.isAlphanum || c == '_')

def flushToken (token : String) (out : List String) : List String :=
  if token.isEmpty then out else token :: out

def lexChars : List Char → String → List String → Option (List String)
  | [], token, out => some (flushToken token out).reverse
  | c :: rest, token, out =>
    if c == ' ' || c == '\n' || c == '\r' || c == '\t' then
      lexChars rest "" (flushToken token out)
    else if identifierChar c then lexChars rest (token.push c) out
    else if ['(', ')', '{', '}', '=', '+', '-', ';', ','].contains c then
      lexChars rest "" (String.singleton c :: flushToken token out)
    else none

def lex (source : String) : Option (List String) := lexChars source.toList "" []

structure ParseContext where
  pointName : String
  componentName : String
  argumentName : String
  localNames : List String := []

private def reserved : List String :=
  ["inline", "void", "AddSolution", "unsigned", "long", "su2double", "const", "Solution", "Solution_Old",
   "alignas", "alignof", "and", "and_eq", "asm", "auto", "bool", "break", "case", "catch", "char",
   "char16_t", "char32_t", "class", "compl", "constexpr", "const_cast", "continue", "decltype",
   "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
   "extern", "false", "float", "for", "friend", "goto", "if", "int", "mutable", "namespace", "new",
   "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
   "public", "register", "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
   "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
   "true", "try", "typedef", "typeid", "typename", "union", "using", "virtual", "volatile",
   "wchar_t", "while", "xor", "xor_eq"]

def identifier (name : String) : Bool :=
  match name.toList with
  | [] => false
  | c :: rest => identifierChar c && !c.isDigit && rest.all identifierChar && !reserved.contains name

def parseIndex (ctx : ParseContext) (name : String) : Option Index :=
  if name == ctx.pointName then some .point
  else if name == ctx.componentName then some .component else none

def parseField (name : String) : Option Field :=
  if name == "Solution" then some .current
  else if name == "Solution_Old" then some .old else none

def parseAccess (ctx : ParseContext) : List String → Option (Access × List String)
  | field :: "(" :: row :: "," :: col :: ")" :: rest => do
    return (⟨← parseField field, ← parseIndex ctx row, ← parseIndex ctx col⟩, rest)
  | _ => none

def parseAtom (ctx : ParseContext) (tokens : List String) : Option (Expr × List String) :=
  match tokens with
  | _ :: "(" :: _ => do
    let (access, rest) ← parseAccess ctx tokens
    return (.load access, rest)
  | name :: rest =>
    if name == ctx.argumentName then some (.argument, rest)
    else if ctx.localNames.contains name then some (.localValue name, rest) else none
  | _ => none

def parseTail : Nat → ParseContext → Expr → List String → Option (Expr × List String)
  | 0, _, _, _ => none
  | fuel + 1, ctx, left, "+" :: rest => do
    let (right, rest) ← parseAtom ctx rest
    parseTail fuel ctx (.add left right) rest
  | fuel + 1, ctx, left, "-" :: rest => do
    let (right, rest) ← parseAtom ctx rest
    parseTail fuel ctx (.sub left right) rest
  | _ + 1, _, left, rest => some (left, rest)

def parseExpr (ctx : ParseContext) (tokens : List String) : Option (Expr × List String) := do
  let (left, rest) ← parseAtom ctx tokens
  parseTail (tokens.length + 1) ctx left rest

def freshLocal (ctx : ParseContext) (name : String) : Bool :=
  identifier name && !([ctx.pointName, ctx.componentName, ctx.argumentName] ++ ctx.localNames).contains name

def parseBody : Nat → ParseContext → List String → Option (Program × List String)
  | 0, _, _ => none
  | _ + 1, _, "}" :: rest => some ([], rest)
  | fuel + 1, ctx, "const" :: "su2double" :: name :: "=" :: rest => do
    if !freshLocal ctx name then none else do
      let (value, ";" :: rest) ← parseExpr ctx rest | none
      let (body, rest) ← parseBody fuel {ctx with localNames := name :: ctx.localNames} rest
      return (.bind name value :: body, rest)
  | fuel + 1, ctx, tokens => do
    let (target, "=" :: rest) ← parseAccess ctx tokens | none
    let (value, ";" :: rest) ← parseExpr ctx rest | none
    let (body, rest) ← parseBody fuel ctx rest
    return (.store target value :: body, rest)

def parse (source : String) : Option Program := do
  let tokens ← lex source
  match tokens with
  | "inline" :: "void" :: "AddSolution" :: "(" :: "unsigned" :: "long" :: p :: "," ::
    "unsigned" :: "long" :: v :: "," :: "su2double" :: x :: ")" :: "{" :: rest =>
    if identifier p && identifier v && identifier x && p != v && p != x && v != x then do
      let (body, []) ← parseBody (tokens.length + 1) ⟨p, v, x, []⟩ rest | none
      return body
    else none
  | _ => none

/-- The proof argument forces parse success in Lean's kernel; there is no default program. -/
def parsedProgram (source : String) (success : (parse source).isSome) : Program :=
  (parse source).get success

end SU2Source
