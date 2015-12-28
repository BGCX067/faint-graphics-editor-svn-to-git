// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <numeric>
#include <vector>
#include "util/gauss.hh"
#include "util/math-constants.hh"

static double gauss_1d(double x, double sigma){
  using faint::math::e;
  using faint::math::pi;
  return (1.0 / sqrt(2 * pi * sigma * sigma)) *
    pow(e, -((x*x) / (2 * sigma * sigma)));
}

// Normalizes the kernel so the pixel sum is 1 to avoid darkening the
// image
static std::vector<double> normalize(const std::vector<double>& k ){
  double sum = std::accumulate(begin(k), end(k), 0.0);

  std::vector<double> k2;
  std::transform(begin(k), end(k), std::back_inserter(k2),
    [&](const double& v){ return v / sum; });
  return k2;
}

static std::vector<double> gauss_kernel_1d(double sigma){
  std::vector<double> k;

  // Pixels further than 3*sigma have very little effect, limit kernel
  // size.
  int w = static_cast<int>(ceil(6 * sigma)) | 1;
  for ( int x = 0; x != w; x++ ){
    k.push_back(gauss_1d(double(x - w / 2), sigma));
  }
  return k;
}

std::vector<double> normalized_gauss_kernel_1d(double sigma){
  assert(sigma >= 0);
  return normalize(gauss_kernel_1d(sigma));
}
