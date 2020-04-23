---
layout: default
permalink: Dialects/IREEDialect
parent: Dialect Definitions
title: "'iree' Dialect"
---

<!-- Autogenerated by mlir-tblgen; don't manually edit -->
# 'iree' Dialect
{: .no_toc }


A dialect used for types common across IREE subdialects.

1. TOC
{:toc}

## Type definition

### ptr

Pointer to a typed value.

### byte_buffer

A constant buffer of mapped host memory.

### mutable_byte_buffer

A buffer of read-write host memory.

## Operation definition

### `iree.do_not_optimize` (IREE::DoNotOptimizeOp)

Prevents compiler optimizations of a value.

Wraps any operands in an unoptimizable identity. This operation is declared
as having side effects, so no compiler optimizations will be able to reason
about it. This prevents its results from being folded. It will be dropped as
the final step in compilation.

#### Operands:

| Operand | Description |
| :-----: | ----------- |
`arguments` | any type

#### Results:

| Result | Description |
| :----: | ----------- |
`results` | any type

### `iree.load_input` (IREE::LoadInputOp)



Syntax:

```
operation ::= `iree.load_input` `(` $src `:` type($src) `)` attr-dict `:` type(results)
```



#### Operands:

| Operand | Description |
| :-----: | ----------- |
`src` | memref of signless integer or floating-point values

#### Results:

| Result | Description |
| :----: | ----------- |
&laquo;unnamed&raquo; | any type

### `iree.store_output` (IREE::StoreOutputOp)



Syntax:

```
operation ::= `iree.store_output` `(` $src `:` type($src) `,` $dst `:` type($dst) `)` attr-dict
```



#### Operands:

| Operand | Description |
| :-----: | ----------- |
`src` | any type
`dst` | memref of signless integer or floating-point values

### `iree.unfoldable_constant` (IREE::UnfoldableConstantOp)

A constant that cannot be folded by the compiler.

Similar to a std.constant, but is declared as having a side effect and has
no folder. This is really just syntactic sugar as it is canonicalized to a
std.constant wrapped in an iree.do_not_optimize.

#### Attributes:

| Attribute | MLIR Type | Description |
| :-------: | :-------: | ----------- |
`value` | Attribute | any attribute

#### Results:

| Result | Description |
| :----: | ----------- |
&laquo;unnamed&raquo; | any type

### `iree.unreachable` (IREE::UnreachableOp)

unreachable assertion op

Syntax:

```
operation ::= `iree.unreachable` attr-dict
```


Signals to the compiler that the parent block should not be reachable.
This may be converted into a runtime assertion, though ideally they are
stripped during translation.

```mlir
^bb0:
  %true = constant 1 : i1
  cond_br %true, ^bb2, ^bb1
^bb1:
  // Indicates that this branch should never be taken.
  iree.unreachable
^bb2:
  ...

```