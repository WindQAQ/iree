// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef IREE_VM_NATIVE_MODULE_H_
#define IREE_VM_NATIVE_MODULE_H_

#include <stdint.h>

#include "iree/base/api.h"
#include "iree/vm/module.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Describes an imported native function in a native module.
// All of this information is assumed read-only and will be referenced for the
// lifetime of any module created with the descriptor.
typedef struct {
  // Fully-qualified function name (for example, 'other_module.foo').
  iree_string_view_t full_name;
} iree_vm_native_import_descriptor_t;

// Describes an exported native function in a native module.
// All of this information is assumed read-only and will be referenced for the
// lifetime of any module created with the descriptor.
typedef struct {
  // Module-local function name (for example, 'foo' for function 'module.foo').
  iree_string_view_t local_name;

  // TODO(#1979): move register info to iree_vm_function_signature_t.
  // Total number of valid i32 registers used by the function.
  uint16_t i32_register_count;
  // Total number of valid ref registers used by the function.
  uint16_t ref_register_count;

  // An optional list of function-level reflection attributes.
  iree_host_size_t reflection_attr_count;
  const iree_vm_reflection_attr_t* reflection_attrs;
} iree_vm_native_export_descriptor_t;

// Describes an native module implementation by way of descriptor tables.
// All of this information is assumed read-only and will be referenced for the
// lifetime of any module created with the descriptor.
//
// The common native module code will use this descriptor to return metadata on
// query, lookup exported functions, and call module-provided implementation
// functions for state and call management.
typedef struct {
  // Name of the module prefixed on all exported functions.
  iree_string_view_t module_name;

  // All imported function descriptors.
  // interface.resolve_import will be called for each import.
  // Imports must be in order sorted by name compatible with
  // iree_string_view_compare.
  iree_host_size_t import_count;
  const iree_vm_native_import_descriptor_t* imports;

  // All exported function descriptors.
  // Exports must be in order sorted by name compatible with
  // iree_string_view_compare.
  iree_host_size_t export_count;
  const iree_vm_native_export_descriptor_t* exports;

  // An optional list of module-level reflection attributes.
  iree_host_size_t reflection_attr_count;
  const iree_vm_reflection_attr_t* reflection_attrs;
} iree_vm_native_module_descriptor_t;

// Creates a new native module with the metadata tables in |descriptor|.
// These tables will be used for reflection and function lookup, and the
// provided function pointers will be called when state needs to be managed or
// exported functions need to be called.
//
// An implementation |interface| providing functions for state management and
// function calls can be provided to override default implementations of
// functions. The structure will be copied and the self pointer will be passed
// to all |interface| functions.
//
// The provided |descriptor| will be referenced by the created module and must
// be kept live for the lifetime of the module.
IREE_API_EXPORT iree_status_t IREE_API_CALL iree_vm_native_module_create(
    const iree_vm_module_t* interface,
    const iree_vm_native_module_descriptor_t* module_descriptor,
    iree_allocator_t allocator, iree_vm_module_t** out_module);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // IREE_VM_NATIVE_MODULE_H_
