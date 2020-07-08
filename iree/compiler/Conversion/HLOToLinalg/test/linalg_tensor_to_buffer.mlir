// RUN: iree-opt -split-input-file -iree-codegen-hlo-to-linalg-on-buffers %s | IreeFileCheck %s

#map0 = affine_map<(d0, d1) -> (d0, d1)>

module {
  func @element_wise() {
    %c0 = constant 0 : index
    %0 = hal.interface.load.tensor @legacy_io::@arg0, offset = %c0 : tensor<2x2xf32>
    %1 = hal.interface.load.tensor @legacy_io::@arg1, offset = %c0 : tensor<2x2xf32>
    %2 = linalg.generic {args_in = 2 : i64, args_out = 1 : i64, indexing_maps = [#map0, #map0, #map0], iterator_types = ["parallel", "parallel"]} %0, %1 {
    ^bb0(%arg3: f32, %arg4: f32):       // no predecessors
      %3 = addf %arg3, %arg4 : f32
      linalg.yield %3 : f32
    }: tensor<2x2xf32>, tensor<2x2xf32> -> tensor<2x2xf32>
    hal.interface.store.tensor %2, @legacy_io::@ret0, offset = %c0 : tensor<2x2xf32>
    return
  }
  hal.interface @legacy_io attributes {sym_visibility = "private"} {
    hal.interface.binding @arg0, set=0, binding=0, type="StorageBuffer", access="Read"
    hal.interface.binding @arg1, set=0, binding=1, type="StorageBuffer", access="Read"
    hal.interface.binding @ret0, set=0, binding=2, type="StorageBuffer", access="Write"
  }
}
// CHECK-LABEL: func @element_wise
//   CHECK-DAG: %[[ARG2:.*]] = iree.placeholder
//  CHECK-SAME:   {binding = @legacy_io::@ret0}
//   CHECK-DAG: %[[ARG0:.*]] = iree.placeholder
//  CHECK-SAME:   {binding = @legacy_io::@arg0}
//   CHECK-DAG: %[[ARG1:.*]] = iree.placeholder
//  CHECK-SAME:   {binding = @legacy_io::@arg1}
//   CHECK-NOT: hal.interface.load.tensor
//       CHECK: linalg.generic
//  CHECK-SAME:   %[[ARG0]], %[[ARG1]], %[[ARG2]]
//       CHECK:   ^{{[a-zA-Z0-9$._-]+}}
//  CHECK-SAME:     %[[ARG3:[a-zA-Z0-9$._-]+]]: f32
//  CHECK-SAME:     %[[ARG4:[a-zA-Z0-9$._-]+]]: f32
//  CHECK-SAME:     %[[ARG5:[a-zA-Z0-9$._-]+]]: f32
//       CHECK:     addf %[[ARG3]], %[[ARG4]]
//   CHECK-NOT: hal.interface.store.tensor

// -----

#map0 = affine_map<(d0, d1) -> (d0, d1)>

module {
  func @indexed_generic() {
    %c0 = constant 0 : index
    %0 = hal.interface.load.tensor @legacy_io::@arg0, offset = %c0 : tensor<2x2xi32>
    %1 = linalg.indexed_generic {args_in = 1 : i64, args_out = 1 : i64, indexing_maps = [#map0, #map0], iterator_types = ["parallel", "parallel"]} %0 {
    ^bb0(%arg2: index, %arg3: index, %arg4: i32):       // no predecessors
      %2 = index_cast %arg2 : index to i32
      %3 = index_cast %arg3 : index to i32
      %4 = addi %arg4, %2 : i32
      %5 = addi %4, %3 : i32
      linalg.yield %5 : i32
    }: tensor<2x2xi32> -> tensor<2x2xi32>
    hal.interface.store.tensor %1, @legacy_io::@ret0, offset = %c0 : tensor<2x2xi32>
    return
  }
  hal.interface @legacy_io attributes {sym_visibility = "private"} {
    hal.interface.binding @arg0, set=0, binding=0, type="StorageBuffer", access="Read"
    hal.interface.binding @ret0, set=0, binding=1, type="StorageBuffer", access="Write"
  }
}
//      CHECK: func @indexed_generic
//  CHECK-DAG: %[[RET0:.*]] = iree.placeholder
// CHECK-SAME:   {binding = @legacy_io::@ret0}
//  CHECK-DAG: %[[ARG0:.*]] = iree.placeholder
// CHECK-SAME:   {binding = @legacy_io::@arg0}
//  CHECK-NOT: hal.interface.load.tensor
//      CHECK: linalg.indexed_generic
// CHECK-SAME:   %[[ARG0]], %[[RET0]]
//  CHECK-NOT: hal.interface.store.tensor
//      CHECK:   ^{{[a-zA-Z0-9$._-]+}}
// CHECK-SAME:       %[[ARG2:[a-zA-Z0-9$._-]+]]: index
// CHECK-SAME:       %[[ARG3:[a-zA-Z0-9$._-]+]]: index
// CHECK-SAME:       %[[ARG4:[a-zA-Z0-9$._-]+]]: i32
//      CHECK:     %[[A:.+]] = index_cast %[[ARG2]] : index to i32
//      CHECK:     %[[B:.+]] = index_cast %[[ARG3]] : index to i32
//      CHECK:     %[[C:.+]] = addi %[[ARG4]], %[[A]] : i32
//      CHECK:     %[[D:.+]] = addi %[[C]], %[[B]] : i32
//      CHECK:     linalg.yield %[[D]] : i32

// -----

#map0 = affine_map<(d0, d1) -> (d0, d1)>
#map1 = affine_map<(d0, d1) -> (d0, 0)>
#map2 = affine_map<(d0, d1) -> (0, d1)>

module {
  func @reshape_arg_result() {
    %c0 = constant 0 : index
    %0 = hal.interface.load.tensor @legacy_io::@arg0,
                                   offset = %c0 : tensor<5xf32>
    %1 = hal.interface.load.tensor @legacy_io::@arg1,
                                   offset = %c0 : tensor<5xf32>
    %2 = linalg.tensor_reshape %0 [#map0] : tensor<5xf32> into tensor<5x1xf32>
    %3 = linalg.tensor_reshape %1 [#map0] : tensor<5xf32> into tensor<1x5xf32>
    %4 = linalg.generic
           {args_in = 2 : i64, args_out = 1 : i64,
            indexing_maps = [#map1, #map2, #map0],
            iterator_types = ["parallel", "parallel"]} %2, %3 {
         ^bb0(%arg3: f32, %arg4: f32):       // no predecessors
           %5 = addf %arg3, %arg4 : f32
           linalg.yield %5 : f32
         }: tensor<5x1xf32>, tensor<1x5xf32> -> tensor<5x5xf32>
    %6 = linalg.tensor_reshape %4 [#map0] : tensor<5x5xf32> into tensor<25xf32>
    hal.interface.store.tensor %6, @legacy_io::@ret0,
                                   offset = %c0 : tensor<25xf32>
    return
  }
  hal.interface @legacy_io attributes {sym_visibility = "private"} {
    hal.interface.binding @arg0, set=0, binding=0,
                                 type="StorageBuffer", access="Read"
    hal.interface.binding @arg1, set=0, binding=1,
                                 type="StorageBuffer", access="Read"
    hal.interface.binding @ret0, set=0, binding=2,
                                 type="StorageBuffer", access="Write"
  }
}
//   CHECK-DAG: #[[MAP0:.*]] = affine_map<(d0, d1) -> (d0, d1)>
//   CHECK-DAG: #[[MAP1:.*]] = affine_map<(d0, d1) -> (d0, 0)>
//   CHECK-DAG: #[[MAP2:.*]] = affine_map<(d0, d1) -> (0, d1)>
//       CHECK: func @reshape_arg_result
//   CHECK-DAG:   %[[RET0:.*]] = iree.placeholder
//  CHECK-SAME:     binding = @legacy_io::@ret0
//   CHECK-DAG:   %[[RESULT:.*]] = linalg.reshape %[[RET0]] [#[[MAP0]]]
//   CHECK-DAG:   %[[ARG0:.*]] = iree.placeholder
//  CHECK-SAME:     binding = @legacy_io::@arg0
//   CHECK-DAG:   %[[ARG1:.*]] = iree.placeholder
//  CHECK-SAME:     binding = @legacy_io::@arg1
//   CHECK-DAG:   %[[LHS:.*]] = linalg.reshape %[[ARG0]] [#[[MAP0]]]
//   CHECK-DAG:   %[[RHS:.*]] = linalg.reshape %[[ARG1]] [#[[MAP0]]]
//       CHECK:   linalg.generic
//  CHECK-SAME:     indexing_maps = [#[[MAP1]], #[[MAP2]], #[[MAP0]]]
//  CHECK-SAME:     %[[LHS]], %[[RHS]], %[[RESULT]]

// -----

#map0 = affine_map<(d0, d1) -> (d0, d1)>
module {
  func @reshape_only() {
    %c0 = constant 0 : index
    %0 = hal.interface.load.tensor @legacy_io::@arg0,
                                   offset = %c0 : tensor<5x5xf32>
    %1 = linalg.tensor_reshape %0 [#map0] : tensor<5x5xf32> into tensor<25xf32>
    hal.interface.store.tensor %1, @legacy_io::@ret0,
                                   offset = %c0 : tensor<25xf32>
    return
  }
  hal.interface @legacy_io attributes {sym_visibility = "private"} {
    hal.interface.binding @arg0, set=0, binding=0,
                                 type="StorageBuffer", access="Read"
    hal.interface.binding @ret0, set=0, binding=1,
                                 type="StorageBuffer", access="Write"
  }
}
//       CHECK: #[[MAP0:.*]] = affine_map<(d0, d1) -> (d0, d1)>
//       CHECK: func @reshape_only
//   CHECK-DAG:   %[[RET0:.*]] = iree.placeholder
//  CHECK-SAME:     binding = @legacy_io::@ret0
//   CHECK-DAG:   %[[RESULT:.*]] = linalg.reshape %[[RET0]] [#[[MAP0]]]
//   CHECK-DAG:   %[[ARG0:.*]] = iree.placeholder
//  CHECK-SAME:     binding = @legacy_io::@arg0
//       CHECK:   linalg.copy(%[[ARG0]], %[[RESULT]])