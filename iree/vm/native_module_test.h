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

#include "iree/vm/context.h"
#include "iree/vm/instance.h"
#include "iree/vm/native_module.h"
#include "iree/vm/ref.h"
#include "iree/vm/stack.h"

// Wrappers for calling the import functions with type (i32)->i32.
// NOTE: we should have some common ones prebuilt or can generate and rely on
// LTO to strip duplicates across the entire executable.
static iree_status_t call_import_i32_i32(iree_vm_stack_t* stack,
                                         const iree_vm_function_t* import,
                                         int32_t arg0, int32_t* out_ret0) {
  // DO NOT SUBMIT
  return 0;
}

//===----------------------------------------------------------------------===//
// module_a
//===----------------------------------------------------------------------===//
// This simple stateless module exports two functions that can be imported by
// other modules or called directly by the user. When no imports, custom types,
// or per-context state is required this simplifies module definitions.
//
// module_b below imports these functions and demonstrates a more complex module
// with state.

static const iree_vm_native_export_descriptor_t module_a_exports_[] = {
    {iree_make_cstring_view("add_1"), 0, 0, 0, NULL},
    {iree_make_cstring_view("sub_1"), 0, 0, 0, NULL},
};
static const iree_vm_native_module_descriptor_t module_a_descriptor_ = {
    iree_make_cstring_view("module_a"),
    0,
    NULL,
    IREE_ARRAYSIZE(module_a_exports_),
    module_a_exports_,
    0,
    NULL,
};

struct module_a_s;
struct module_a_state_s;
typedef struct module_a_s module_a_t;
typedef struct module_a_state_s module_a_state_t;

// vm.import @module_a.add_1(%arg0 : i32) -> i32
static iree_status_t module_a_add_1(module_a_t* module, module_a_state_t* state,
                                    iree_vm_stack_t* stack,
                                    const iree_vm_function_call_t* call,
                                    iree_vm_execution_result_t* out_result) {
  // Add 1 to arg0 and return.
  // DO NOT SUBMIT
  return iree_ok_status();
}

// vm.import @module_a.sub_1(%arg0 : i32) -> i32
static iree_status_t module_a_sub_1(module_a_t* module, module_a_state_t* state,
                                    iree_vm_stack_t* stack,
                                    const iree_vm_function_call_t* call,
                                    iree_vm_execution_result_t* out_result) {
  // Sub 1 to arg0 and return. Fail if < 0.
  // DO NOT SUBMIT
  return iree_ok_status();
}

typedef iree_status_t (*module_a_func_t)(
    module_a_t* module, module_a_state_t* state, iree_vm_stack_t* stack,
    const iree_vm_function_call_t* call,
    iree_vm_execution_result_t* out_result);
static const module_a_func_t module_a_funcs_[] = {
    module_a_add_1,
    module_a_sub_1,
};
static_assert(IREE_ARRAYSIZE(module_a_funcs_) ==
                  IREE_ARRAYSIZE(module_a_exports_),
              "function pointer table must be 1:1 with exports");

static iree_status_t IREE_API_PTR module_a_begin_call(
    void* self, iree_vm_stack_t* stack, const iree_vm_function_call_t* call,
    iree_vm_execution_result_t* out_result) {
  // NOTE: we aren't using module state in this module.
  return module_a_funcs_[call->function.ordinal](
      /*module=*/NULL, /*module_state=*/NULL, stack, call, out_result);
}

static iree_status_t module_a_create(iree_allocator_t allocator,
                                     iree_vm_module_t** out_module) {
  // NOTE: this module has neither shared or per-context module state.
  iree_vm_module_t interface;
  IREE_RETURN_IF_ERROR(iree_vm_module_initialize(&interface, NULL));
  interface.begin_call = module_a_begin_call;
  return iree_vm_native_module_create(&interface, &module_a_descriptor_,
                                      allocator, out_module);
}

//===----------------------------------------------------------------------===//
// module_b
//===----------------------------------------------------------------------===//
// A more complex module that holds state for resolved types (shared across
// all instances), imported functions (stored per-context), per-context user
// data, and reflection metadata.

static const iree_vm_native_import_descriptor_t module_b_imports_[] = {
    {iree_make_cstring_view("module_a.add_1")},
    {iree_make_cstring_view("module_a.sub_1")},
};
static const iree_vm_reflection_attr_t module_b_entry_attrs_[] = {
    {iree_make_cstring_view("key1"), iree_make_cstring_view("value1")},
};
static const iree_vm_native_export_descriptor_t module_b_exports_[] = {
    {iree_make_cstring_view("entry"), 0, 0,
     IREE_ARRAYSIZE(module_b_entry_attrs_), module_b_entry_attrs_},
};
static const iree_vm_native_module_descriptor_t module_b_descriptor_ = {
    iree_make_cstring_view("module_b"),
    IREE_ARRAYSIZE(module_b_imports_),
    module_b_imports_,
    IREE_ARRAYSIZE(module_b_exports_),
    module_b_exports_,
    0,
    NULL,
};

struct module_b_s;
struct module_b_state_s;
typedef struct module_b_s module_b_t;
typedef struct module_b_state_s module_b_state_t;

// Stores shared state across all instances of the module.
// This should generally be treated as read-only and if mutation is possible
// then users must synchronize themselves.
typedef struct module_b_s {
  // Allocator the module must be freed with and that can be used for any other
  // shared dynamic allocations.
  iree_allocator_t allocator;
  // Resolved types; these never change once queried and are safe to store on
  // the shared structure to avoid needing to look them up again.
  const iree_vm_ref_type_descriptor_t* types[1];
} module_b_t;

// Stores per-context state; at the minimum imports, but possibly other user
// state data. No synchronization is required as the VM will not call functions
// with the same state from multiple threads concurrently.
typedef struct module_b_state_s {
  // Allocator the state must be freed with and that can be used for any other
  // per-context dynamic allocations.
  iree_allocator_t allocator;
  // Resolved import functions matching 1:1 with the module import descriptors.
  iree_vm_function_t imports[2];
  // Example user data stored per-state.
  int counter;
} module_b_state_t;
static_assert(IREE_ARRAYSIZE(module_b_state_t::imports) ==
                  IREE_ARRAYSIZE(module_b_imports_),
              "import storage must be able to hold all imports");

// Frees the shared module; by this point all per-context states have been
// freed and no more shared data is required.
static void IREE_API_PTR module_b_destroy(void* self) {
  module_b_t* module = (module_b_t*)self;
  iree_allocator_free(module->allocator, module);
}

// Allocates per-context state, which stores resolved import functions and any
// other non-shared user state.
static iree_status_t IREE_API_PTR
module_b_alloc_state(void* self, iree_allocator_t allocator,
                     iree_vm_module_state_t** out_module_state) {
  module_b_state_t* state = NULL;
  IREE_RETURN_IF_ERROR(
      iree_allocator_malloc(allocator, sizeof(*state), (void**)&state));
  memset(state, 0, sizeof(*state));
  *out_module_state = (iree_vm_module_state_t*)state;
  return iree_ok_status();
}

// Frees the per-context state.
static void IREE_API_PTR
module_b_free_state(void* self, iree_vm_module_state_t* module_state) {
  module_b_state_t* state = (module_b_state_t*)module_state;
  iree_allocator_free(state->allocator, state);
}

// Called once per import function so the module can store the function ref.
static iree_status_t IREE_API_PTR
module_b_resolve_import(void* self, iree_vm_module_state_t* module_state,
                        iree_host_size_t ordinal, iree_vm_function_t function) {
  module_b_state_t* state = (module_b_state_t*)module_state;
  state->imports[ordinal] = function;
  return iree_ok_status();
}

// Our actual function. Here we directly access the registers but one could also
// use this as a trampoline into user code with a native signature (such as
// fetching the args, calling the function as a normal C function, and stashing
// back the results).
//
// vm.import @module_b.entry(%arg0 : i32) -> i32
static iree_status_t module_b_entry(module_b_t* module, module_b_state_t* state,
                                    iree_vm_stack_t* stack,
                                    const iree_vm_function_call_t* call,
                                    iree_vm_execution_result_t* out_result) {
  // TODO(benvanik): iree_vm_stack native frame enter/leave.
  // By not enter/leaving a frame here we won't be able to set breakpoints or
  // tracing on the function. Fine for now.
  iree_vm_stack_frame_t* caller_frame = iree_vm_stack_current_frame(stack);
  const iree_vm_register_list_t* arg_list = call->argument_registers;
  const iree_vm_register_list_t* ret_list = call->result_registers;
  auto& regs = caller_frame->registers;

  // NOTE: if we needed to use ref types here we have them under module->types.
  assert(module->types[0]);

  // Load the input argument.
  // This should really be generated code (like module_abi_cc.h).
  int32_t arg0 = regs.i32[arg_list->registers[0] & regs.i32_mask];

  // Call module_a.add_1.
  IREE_RETURN_IF_ERROR(
      call_import_i32_i32(stack, &state->imports[0], arg0, &arg0));

  // Increment per-context state (persists across calls). No need for a mutex as
  // only one thread can be using the per-context state at a time.
  state->counter += arg0;
  int32_t ret0 = state->counter;

  // Call module_a.sub_1.
  IREE_RETURN_IF_ERROR(
      call_import_i32_i32(stack, &state->imports[1], ret0, &ret0));

  // Store the result.
  regs.i32[ret_list->registers[0] & regs.i32_mask] = ret0;

  return iree_ok_status();
}

// Table of exported function pointers. Note that this table could be read-only
// (like here) or shared/per-context to allow exposing different functions based
// on versions, access rights, etc.
typedef iree_status_t (*module_b_func_t)(
    module_b_t* module, module_b_state_t* state, iree_vm_stack_t* stack,
    const iree_vm_function_call_t* call,
    iree_vm_execution_result_t* out_result);
static const module_b_func_t module_b_funcs_[] = {
    module_b_entry,
};
static_assert(IREE_ARRAYSIZE(module_b_funcs_) ==
                  IREE_ARRAYSIZE(module_b_exports_),
              "function pointer table must be 1:1 with exports");

static iree_status_t IREE_API_PTR module_b_begin_call(
    void* self, iree_vm_stack_t* stack, const iree_vm_function_call_t* call,
    iree_vm_execution_result_t* out_result) {
  iree_vm_module_state_t* module_state = NULL;
  IREE_RETURN_IF_ERROR(iree_vm_stack_query_module_state(
      stack, call->function.module, &module_state));
  return module_b_funcs_[call->function.ordinal](
      (module_b_t*)self, (module_b_state_t*)module_state, stack, call,
      out_result);
}

static iree_status_t module_b_create(iree_allocator_t allocator,
                                     iree_vm_module_t** out_module) {
  // Allocate shared module state.
  module_b_t* module = NULL;
  IREE_RETURN_IF_ERROR(
      iree_allocator_malloc(allocator, sizeof(*module), (void**)&module));
  memset(module, 0, sizeof(*module));

  // Resolve types used by the module once so that we can share it across all
  // instances of the module.
  module->types[0] = iree_vm_ref_lookup_registered_type(
      iree_make_cstring_view("iree.byte_buffer"));
  if (!module->types[0]) {
    iree_allocator_free(allocator, module);
    return iree_make_status(
        IREE_STATUS_NOT_FOUND,
        "required type iree.byte_buffer not registered with the type system");
  }

  // Setup the interface with the functions we implement ourselves. Any function
  // we omit will be handled by the base native module.
  iree_vm_module_t interface;
  iree_status_t status = iree_vm_module_initialize(&interface, module);
  if (!iree_status_is_ok(status)) {
    iree_allocator_free(allocator, module);
    return status;
  }
  interface.destroy = module_b_destroy;
  interface.alloc_state = module_b_alloc_state;
  interface.free_state = module_b_free_state;
  interface.resolve_import = module_b_resolve_import;
  interface.begin_call = module_b_begin_call;
  return iree_vm_native_module_create(&interface, &module_b_descriptor_,
                                      allocator, out_module);
}
