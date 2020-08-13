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

#include "iree/vm/native_module.h"

// Native module implementation allocated for all modules.
typedef struct {
  // Interface containing default function pointers.
  // base_interface.self will be the self pointer to iree_vm_native_module_t.
  //
  // Must be first in the struct as we dereference the interface to find our
  // members below.
  iree_vm_module_t base_interface;

  // Interface with optional user-provided function pointers.
  // user_interface.self will contain the user's module pointer that must be
  // passed to all functions.
  iree_vm_module_t user_interface;

  // Allocator this module was allocated with and must be freed with.
  iree_allocator_t allocator;

  // Module descriptor used for reflection.
  const iree_vm_native_module_descriptor_t* descriptor;
} iree_vm_native_module_t;

static void IREE_API_PTR iree_vm_native_module_destroy(void* self) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;

  // Destroy the optional user-provided self.
  if (module->user_interface.destroy) {
    module->user_interface.destroy(module->user_interface.self);
  }

  iree_allocator_free(module->allocator, module);
}

static iree_string_view_t IREE_API_PTR iree_vm_native_module_name(void* self) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (module->user_interface.name) {
    return module->user_interface.name(module->user_interface.self);
  }
  return module->descriptor->module_name;
}

static iree_vm_module_signature_t IREE_API_PTR
iree_vm_native_module_signature(void* self) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (module->user_interface.signature) {
    return module->user_interface.signature(module->user_interface.self);
  }
  iree_vm_module_signature_t signature;
  memset(&signature, 0, sizeof(signature));
  signature.import_function_count = module->descriptor->import_count;
  signature.export_function_count = module->descriptor->export_count;
  signature.internal_function_count = 0;  // unused
  return signature;
}

static iree_status_t IREE_API_PTR iree_vm_native_module_get_import_function(
    iree_vm_native_module_t* module, iree_host_size_t ordinal,
    iree_vm_function_t* out_function, iree_string_view_t* out_name,
    iree_vm_function_signature_t* out_signature) {
  if (ordinal >= module->descriptor->import_count) {
    return iree_make_status(IREE_STATUS_INVALID_ARGUMENT,
                            "import ordinal out of range (0 < %zu < %zu)",
                            ordinal, module->descriptor->import_count);
  }
  const iree_vm_native_import_descriptor_t* import_descriptor =
      &module->descriptor->imports[ordinal];
  if (out_function) {
    out_function->module = &module->base_interface;
    out_function->linkage = IREE_VM_FUNCTION_LINKAGE_IMPORT;
    out_function->ordinal = (uint16_t)ordinal;
  }
  if (out_name) {
    *out_name = import_descriptor->full_name;
  }
  // TODO(#1979): signature queries when info is useful.
  return iree_ok_status();
}

static iree_status_t IREE_API_PTR iree_vm_native_module_get_export_function(
    iree_vm_native_module_t* module, iree_host_size_t ordinal,
    iree_vm_function_t* out_function, iree_string_view_t* out_name,
    iree_vm_function_signature_t* out_signature) {
  if (ordinal >= module->descriptor->export_count) {
    return iree_make_status(IREE_STATUS_INVALID_ARGUMENT,
                            "export ordinal out of range (0 < %zu < %zu)",
                            ordinal, module->descriptor->export_count);
  }
  const iree_vm_native_export_descriptor_t* export_descriptor =
      &module->descriptor->exports[ordinal];
  if (out_function) {
    out_function->module = &module->base_interface;
    out_function->linkage = IREE_VM_FUNCTION_LINKAGE_EXPORT;
    out_function->ordinal = (uint16_t)ordinal;
    out_function->i32_register_count = export_descriptor->i32_register_count;
    out_function->ref_register_count = export_descriptor->ref_register_count;
  }
  if (out_name) {
    *out_name = export_descriptor->local_name;
  }
  // TODO(#1979): signature queries when info is useful.
  return iree_ok_status();
}

static iree_status_t IREE_API_PTR iree_vm_native_module_get_function(
    void* self, iree_vm_function_linkage_t linkage, iree_host_size_t ordinal,
    iree_vm_function_t* out_function, iree_string_view_t* out_name,
    iree_vm_function_signature_t* out_signature) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (out_function) memset(out_function, 0, sizeof(*out_function));
  if (out_name) memset(out_name, 0, sizeof(*out_name));
  if (out_signature) memset(out_signature, 0, sizeof(*out_signature));
  if (module->user_interface.get_function) {
    return module->user_interface.get_function(module->user_interface.self,
                                               linkage, ordinal, out_function,
                                               out_name, out_signature);
  }
  switch (linkage) {
    case IREE_VM_FUNCTION_LINKAGE_IMPORT:
      return iree_vm_native_module_get_import_function(
          module, ordinal, out_function, out_name, out_signature);
    case IREE_VM_FUNCTION_LINKAGE_EXPORT:
      return iree_vm_native_module_get_export_function(
          module, ordinal, out_function, out_name, out_signature);
    default:
      return iree_make_status(
          IREE_STATUS_UNIMPLEMENTED,
          "native modules do not support internal function queries");
  }
}

static iree_status_t IREE_API_PTR
iree_vm_native_module_get_function_reflection_attr(
    void* self, iree_vm_function_linkage_t linkage, iree_host_size_t ordinal,
    iree_host_size_t index, iree_string_view_t* key,
    iree_string_view_t* value) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (module->user_interface.get_function_reflection_attr) {
    return module->user_interface.get_function_reflection_attr(
        module->user_interface.self, linkage, ordinal, index, key, value);
  }
  // TODO(benvanik): implement native module reflection.
  return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                          "reflection not yet implemented");
}

static iree_status_t IREE_API_PTR iree_vm_native_module_lookup_function(
    void* self, iree_vm_function_linkage_t linkage, iree_string_view_t name,
    iree_vm_function_t* out_function) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  memset(out_function, 0, sizeof(*out_function));
  if (module->user_interface.lookup_function) {
    return module->user_interface.lookup_function(module->user_interface.self,
                                                  linkage, name, out_function);
  }

  if (linkage != IREE_VM_FUNCTION_LINKAGE_EXPORT) {
    // NOTE: we could support imports if required.
    return iree_make_status(
        IREE_STATUS_UNIMPLEMENTED,
        "native modules do not support import/internal function queries");
  }

  // Binary search through the export descriptors.
  ptrdiff_t min_ordinal = 0;
  ptrdiff_t max_ordinal = module->descriptor->export_count - 1;
  const iree_vm_native_export_descriptor_t* exports =
      module->descriptor->exports;
  while (min_ordinal <= max_ordinal) {
    ptrdiff_t ordinal = (min_ordinal + max_ordinal) / 2;
    int cmp = iree_string_view_compare(exports[ordinal].local_name, name);
    if (cmp == 0) {
      return iree_vm_native_module_get_function(self, linkage, ordinal,
                                                out_function, NULL, NULL);
    } else if (cmp < 0) {
      min_ordinal = ordinal + 1;
    } else {
      max_ordinal = ordinal - 1;
    }
  }
  return iree_make_status(
      IREE_STATUS_NOT_FOUND, "no function %.*s.%.*s exported by module",
      (int)module->descriptor->module_name.size,
      module->descriptor->module_name.data, (int)name.size, name.data);
}

static iree_status_t IREE_API_PTR
iree_vm_native_module_alloc_state(void* self, iree_allocator_t allocator,
                                  iree_vm_module_state_t** out_module_state) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  *out_module_state = NULL;
  if (module->user_interface.alloc_state) {
    return module->user_interface.alloc_state(module->user_interface.self,
                                              allocator, out_module_state);
  }
  // Default to no state.
  return iree_ok_status();
}

static void IREE_API_PTR iree_vm_native_module_free_state(
    void* self, iree_vm_module_state_t* module_state) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (module->user_interface.free_state) {
    module->user_interface.free_state(module->user_interface.self,
                                      module_state);
    return;
  }
  // No-op in the default implementation.
  // TODO(#2843): IREE_DCHECK_EQ(NULL, module_state);
  assert(!module_state);
}

static iree_status_t IREE_API_PTR iree_vm_native_module_resolve_import(
    void* self, iree_vm_module_state_t* module_state, iree_host_size_t ordinal,
    iree_vm_function_t function) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (module->user_interface.resolve_import) {
    return module->user_interface.resolve_import(
        module->user_interface.self, module_state, ordinal, function);
  }
  return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                          "native module does not support imports");
}

static iree_status_t IREE_API_PTR iree_vm_native_module_begin_call(
    void* self, iree_vm_stack_t* stack, const iree_vm_function_call_t* call,
    iree_vm_execution_result_t* out_result) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (call->function.linkage != IREE_VM_FUNCTION_LINKAGE_EXPORT ||
      call->function.ordinal >= module->descriptor->export_count) {
    return iree_make_status(IREE_STATUS_INVALID_ARGUMENT,
                            "function ordinal out of bounds: 0 < %u < %zu",
                            call->function.ordinal,
                            module->descriptor->export_count);
  }
  if (module->user_interface.begin_call) {
    return module->user_interface.begin_call(module->user_interface.self, stack,
                                             call, out_result);
  }
  return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                          "native module does not support calls");
}

static iree_status_t IREE_API_PTR
iree_vm_native_module_resume_call(void* self, iree_vm_stack_t* stack,
                                  iree_vm_execution_result_t* out_result) {
  iree_vm_native_module_t* module = (iree_vm_native_module_t*)self;
  if (module->user_interface.resume_call) {
    return module->user_interface.resume_call(module->user_interface.self,
                                              stack, out_result);
  }
  return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                          "native module does not support resume");
}

IREE_API_EXPORT iree_status_t IREE_API_CALL iree_vm_native_module_create(
    const iree_vm_module_t* interface,
    const iree_vm_native_module_descriptor_t* module_descriptor,
    iree_allocator_t allocator, iree_vm_module_t** out_module) {
  IREE_ASSERT_ARGUMENT(out_module);
  *out_module = NULL;

  // TODO(benvanik): invert allocation such that caller allocates and we init.
  // This would avoid the need for any dynamic memory allocation in the common
  // case as the outer user module interface could nest us. Note that we'd need
  // to expose this via a query_size function so that we could adjust the size
  // of our storage independent of the definition of the user module.
  iree_vm_native_module_t* module = NULL;
  IREE_RETURN_IF_ERROR(iree_allocator_malloc(
      allocator, sizeof(iree_vm_native_module_t), (void**)&module));
  module->allocator = allocator;
  module->descriptor = module_descriptor;

  // TODO(benvanik): version interface and copy only valid bytes.
  memcpy(&module->user_interface, interface, sizeof(*interface));

  // Base interface that routes through our thunks.
  iree_vm_module_initialize(&module->base_interface, module);
  module->base_interface.destroy = iree_vm_native_module_destroy;
  module->base_interface.name = iree_vm_native_module_name;
  module->base_interface.signature = iree_vm_native_module_signature;
  module->base_interface.get_function = iree_vm_native_module_get_function;
  module->base_interface.get_function_reflection_attr =
      iree_vm_native_module_get_function_reflection_attr;
  module->base_interface.lookup_function =
      iree_vm_native_module_lookup_function;
  module->base_interface.alloc_state = iree_vm_native_module_alloc_state;
  module->base_interface.free_state = iree_vm_native_module_free_state;
  module->base_interface.resolve_import = iree_vm_native_module_resolve_import;
  module->base_interface.begin_call = iree_vm_native_module_begin_call;
  module->base_interface.resume_call = iree_vm_native_module_resume_call;

  *out_module = &module->base_interface;
  return iree_ok_status();
}
