// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Defines StabilizedLogCalculator.

#include <cmath>
#include <memory>
#include <string>

#include "mediapipe/calculators/audio/stabilized_log_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/matrix.h"
#include "mediapipe/framework/formats/time_series_header.pb.h"
#include "mediapipe/framework/port/proto_ns.h"
#include "mediapipe/util/time_series_util.h"

namespace mediapipe {

// Example config:
// node {
//   calculator: "StabilizedLogCalculator"
//   input_stream: "input_time_series"
//   output_stream: "stabilized_log_time_series"
//   options {
//     [mediapipe.StabilizedLogCalculatorOptions.ext] {
//       stabilizer: .00001
//       check_nonnegativity: true
//     }
//   }
// }
class StabilizedLogCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    cc->Inputs().Index(0).Set<Matrix>(
        // Input stream with TimeSeriesHeader.
    );
    cc->Outputs().Index(0).Set<Matrix>(
        // Output stabilized log stream with TimeSeriesHeader.
    );
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) override {
    StabilizedLogCalculatorOptions stabilized_log_calculator_options =
        cc->Options<StabilizedLogCalculatorOptions>();

    stabilizer_ = stabilized_log_calculator_options.stabilizer();
    output_scale_ = stabilized_log_calculator_options.output_scale();
    check_nonnegativity_ =
        stabilized_log_calculator_options.check_nonnegativity();
    CHECK_GE(stabilizer_, 0.0)
        << "stabilizer must be >= 0.0, received a value of " << stabilizer_;

    // If the input packets have a header, propagate the header to the output.
    if (!cc->Inputs().Index(0).Header().IsEmpty()) {
      TimeSeriesHeader input_header;
      RETURN_IF_ERROR(time_series_util::FillTimeSeriesHeaderIfValid(
          cc->Inputs().Index(0).Header(), &input_header));
      cc->Outputs().Index(0).SetHeader(
          Adopt(new TimeSeriesHeader(input_header)));
    }
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) override {
    auto input_matrix = cc->Inputs().Index(0).Get<Matrix>();
    if (check_nonnegativity_) {
      CHECK_GE(input_matrix.minCoeff(), 0);
    }
    std::unique_ptr<Matrix> output_frame(new Matrix(
        output_scale_ * (input_matrix.array() + stabilizer_).log().matrix()));
    cc->Outputs().Index(0).Add(output_frame.release(), cc->InputTimestamp());
    return ::mediapipe::OkStatus();
  }

 private:
  float stabilizer_;
  bool check_nonnegativity_;
  double output_scale_;
};
REGISTER_CALCULATOR(StabilizedLogCalculator);

}  // namespace mediapipe
