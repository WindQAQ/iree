// RUN: iree-opt -split-input-file -iree-hal-materialize-interfaces %s | IreeFileCheck %s

// CHECK-LABEL: hal.executable @simpleMath_ex_dispatch_0
// CHECK-DAG: hal.interface @legacy_io {
// CHECK-NEXT:  hal.interface.binding @arg0, set=0, binding=0, type="StorageBuffer", access="Read"
// CHECK-NEXT:  hal.interface.binding @ret0, set=0, binding=1, type="StorageBuffer", access="Write|Discard"
// CHECK-NEXT: }
// CHECK-DAG: hal.executable.entry_point @simpleMath_rgn_dispatch_0 attributes {
// CHECK-SAME:  interface = @legacy_io,
// CHECK-SAME:  ordinal = 0 : i32,
// CHECK-SAME:  signature = (tensor<4xf32>) -> tensor<4xf32>,
// CHECK-SAME:  workgroup_size = dense<1> : vector<3xi32>
// CHECK-SAME:}
// CHECK-DAG: hal.executable.source {
// CHECK-NEXT: module {
// CHECK-NEXT: flow.executable @simpleMath_ex_dispatch_0
flow.executable @simpleMath_ex_dispatch_0 {
  flow.dispatch.entry @simpleMath_rgn_dispatch_0 attributes {
      workload = dense<[4, 1, 1]> : vector<3xi32>
  }
  // CHECK: module {
  module {
    // CHECK-NEXT: func @simpleMath_rgn_dispatch_0() {
    // CHECK-NEXT:   [[ZERO:%.+]] = constant 0
    // CHECK-NEXT:   [[ARG0:%.+]] = hal.interface.load.tensor @legacy_io::@arg0, offset = [[ZERO]] : tensor<4xf32>
    // CHECK-NEXT:   [[RET0:%.+]] = call @simpleMath_rgn_dispatch_0_impl([[ARG0]]) : (tensor<4xf32>) -> tensor<4xf32>
    // CHECK-NEXT:   hal.interface.store.tensor [[RET0]], @legacy_io::@ret0, offset = [[ZERO]] : tensor<4xf32>
    // CHECK-NEXT:   return
    // CHECK-NEXT: }
    // CHECK-NEXT: func @simpleMath_rgn_dispatch_0_impl
    func @simpleMath_rgn_dispatch_0(%arg0: tensor<4xf32>) -> tensor<4xf32> {
      %0 = xla_hlo.add %arg0, %arg0 : tensor<4xf32>
      return %0 : tensor<4xf32>
    }
    // CHECK: hal.interface @legacy_io attributes {sym_visibility = "private"}
  }
}

// -----

// CHECK-LABEL: hal.executable @reduction_ex_reduce_0_dim_0
// CHECK-DAG: hal.interface @legacy_io {
// CHECK-NEXT:  hal.interface.binding @arg0, set=0, binding=0, type="StorageBuffer", access="Read"
// CHECK-NEXT:  hal.interface.binding @arg1, set=0, binding=1, type="StorageBuffer", access="Read"
// CHECK-NEXT:  hal.interface.binding @ret0, set=0, binding=2, type="StorageBuffer", access="Write|Discard"
// CHECK-NEXT: }
// CHECK-DAG: hal.executable.entry_point @reduction_rgn_reduce_0_dim_0_entry attributes {
// CHECK-SAME:  interface = @legacy_io,
// CHECK-SAME:  ordinal = 0 : i32,
// CHECK-SAME:  signature = (tensor<4x8xf32>, tensor<f32>) -> tensor<4xf32>,
// CHECK-SAME:  workgroup_size = dense<1> : vector<3xi32>
// CHECK-SAME: }
// CHECK-DAG: hal.executable.source {
// CHECK-NEXT: module {
// CHECK-NEXT: flow.executable @reduction_ex_reduce_0_dim_0
flow.executable @reduction_ex_reduce_0_dim_0 {
  flow.reduction.entry @reduction_rgn_reduce_0_dim_0_entry apply(@reduction_rgn_reduce_0_dim_0) attributes {
    dimension = 1 : i32,
    workgroup_size = dense<[32, 1, 1]> : vector<3xi32>,
    workload = dense<[4, 1, 1]> : vector<3xi32>
  }
  // CHECK: module {
  module {
    // CHECK-NEXT: func @reduction_rgn_reduce_0_dim_0_entry() {
    // CHECK-NEXT:   [[ZERO:%.+]] = constant 0
    // CHECK-NEXT:   [[ARG0:%.+]] = hal.interface.load.tensor @legacy_io::@arg0, offset = [[ZERO]] : tensor<4x8xf32>
    // CHECK-NEXT:   [[ARG1:%.+]] = hal.interface.load.tensor @legacy_io::@arg1, offset = [[ZERO]] : tensor<f32>
    // CHECK-NEXT:   [[RET0:%.+]] = call @reduction_rgn_reduce_0_dim_0_entry_impl([[ARG0]], [[ARG1]]) : (tensor<4x8xf32>, tensor<f32>) -> tensor<4xf32>
    // CHECK-NEXT:   hal.interface.store.tensor [[RET0]], @legacy_io::@ret0, offset = [[ZERO]] : tensor<4xf32>
    // CHECK-NEXT:   return
    // CHECK-NEXT: }
    // CHECK-NEXT: func @reduction_rgn_reduce_0_dim_0_entry_impl
    func @reduction_rgn_reduce_0_dim_0_entry(tensor<4x8xf32>, tensor<f32>) -> tensor<4xf32>
    func @reduction_rgn_reduce_0_dim_0(%arg0: tensor<f32>, %arg1: tensor<f32>) -> tensor<f32> {
      %0 = xla_hlo.add %arg0, %arg1 : tensor<f32>
      return %0 : tensor<f32>
    }
    // CHECK: hal.interface @legacy_io attributes {sym_visibility = "private"}
  }
}