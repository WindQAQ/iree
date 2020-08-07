// Copyright 2019 Google LLC
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

#include "iree/base/internal/status_builder.h"

#include <cstdio>

#include "iree/base/internal/status_errors.h"

namespace iree {

StatusBuilder::Rep::Rep(const Rep& r)
    : stream_message(r.stream_message), stream(&stream_message) {}

StatusBuilder::StatusBuilder(const Status& original_status,
                             SourceLocation location)
    : status_(original_status), loc_(location) {}

StatusBuilder::StatusBuilder(Status&& original_status, SourceLocation location)
    : status_(original_status), loc_(location) {}

StatusBuilder::StatusBuilder(StatusCode code, SourceLocation location)
    : status_(code, ""), loc_(location) {}

StatusBuilder::operator Status() const& {
  if (rep_ == nullptr) return status_;
  return StatusBuilder(*this).CreateStatus();
}
StatusBuilder::operator Status() && {
  if (rep_ == nullptr) return status_;
  return std::move(*this).CreateStatus();
}

bool StatusBuilder::ok() const { return status_.ok(); }

StatusCode StatusBuilder::code() const { return status_.code(); }

SourceLocation StatusBuilder::source_location() const { return loc_; }

Status StatusBuilder::CreateStatus() && {
  Status result = JoinMessageToStatus(status_, rep_->stream_message);

  // Reset the status after consuming it.
  status_ = Status(StatusCode::kUnknown, "");
  rep_ = nullptr;
  return result;
}

Status StatusBuilder::JoinMessageToStatus(Status s, absl::string_view msg) {
  if (msg.empty()) return s;
  return Annotate(s, msg);
}

std::ostream& operator<<(std::ostream& os, const StatusBuilder& builder) {
  return os << static_cast<Status>(builder);
}

std::ostream& operator<<(std::ostream& os, StatusBuilder&& builder) {
  return os << static_cast<Status>(std::move(builder));
}

StatusBuilder AbortedErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kAborted, location);
}

StatusBuilder AlreadyExistsErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kAlreadyExists, location);
}

StatusBuilder CancelledErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kCancelled, location);
}

StatusBuilder DataLossErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kDataLoss, location);
}

StatusBuilder DeadlineExceededErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kDeadlineExceeded, location);
}

StatusBuilder FailedPreconditionErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kFailedPrecondition, location);
}

StatusBuilder InternalErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kInternal, location);
}

StatusBuilder InvalidArgumentErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kInvalidArgument, location);
}

StatusBuilder NotFoundErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kNotFound, location);
}

StatusBuilder OutOfRangeErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kOutOfRange, location);
}

StatusBuilder PermissionDeniedErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kPermissionDenied, location);
}

StatusBuilder UnauthenticatedErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kUnauthenticated, location);
}

StatusBuilder ResourceExhaustedErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kResourceExhausted, location);
}

StatusBuilder UnavailableErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kUnavailable, location);
}

StatusBuilder UnimplementedErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kUnimplemented, location);
}

StatusBuilder UnknownErrorBuilder(SourceLocation location) {
  return StatusBuilder(StatusCode::kUnknown, location);
}

}  // namespace iree
