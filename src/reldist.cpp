#include "valr.h"

void reldist_grouped(intervalVector& vx, intervalVector& vy,
                     std::vector<int>& indices_x,
                     std::vector<float>& rel_distances) {

  // first build sorted vector of y interval midpoints

  std::vector<int> ref_midpoints ;
  for (auto vy_it : vy) {
    int midpoint = (vy_it.start + vy_it.stop) / 2 ;
    ref_midpoints.push_back(midpoint) ;
  }

  std::sort(ref_midpoints.begin(), ref_midpoints.end()) ;

  std::size_t low_idx, upper_idx;

  // iterate through x intervals and calculate reldist using a binary search
  for (auto vx_it : vx) {
    int midpoint = (vx_it.start + vx_it.stop) / 2 ;
    auto low_it = std::lower_bound(ref_midpoints.begin(),
                                   ref_midpoints.end(), midpoint) ;

    low_idx = low_it - ref_midpoints.begin() ;

    // drop intervals at start and end which have no reldist
    if (low_idx == 0 ||
        low_idx == ref_midpoints.size()) {
      continue ;
    }

    // get index below and above
    low_idx = low_idx - 1;
    upper_idx = low_idx + 1 ;

    int left = ref_midpoints[low_idx] ;
    int right = ref_midpoints[upper_idx] ;

    int dist_l = abs(midpoint - left) ;
    int dist_r = abs(midpoint - right) ;

    //calc relative distance
    auto reldist = (float) std::min(dist_l, dist_r) / float(right - left) ;

    rel_distances.push_back(reldist) ;
    indices_x.push_back(vx_it.value) ;

  }
}

//[[Rcpp::export]]
DataFrame reldist_impl(GroupedDataFrame x, GroupedDataFrame y) {

  std::vector<float> rel_distances ;
  std::vector<int> indices_x ;

  DataFrame df_x = x.data() ;
  PairedGroupApply(x, y, reldist_grouped, std::ref(indices_x), std::ref(rel_distances));

  DataFrame subset_x = DataFrameSubsetVisitors(df_x, names(df_x)).subset(indices_x, "data.frame");

  auto ncol_x = subset_x.size() ;

  CharacterVector names(ncol_x + 1) ;
  CharacterVector names_x = subset_x.attr("names") ;

  List out(ncol_x + 1) ;

  // x names, data
  for (int i=0; i<ncol_x; i++) {
    names[i] = names_x[i] ;
    out[i] = subset_x[i] ;
  }
  out[ncol_x] = rel_distances ;
  names[ncol_x] = "reldist" ;

  out.attr("names") = names ;
  out.attr("class") = classes_not_grouped() ;
  auto nrows = subset_x.nrows() ;
  set_rownames(out, nrows) ;

  return out ;

}

/***R
library(dplyr)
  x <- tibble::tribble(
      ~chrom, ~start, ~end,
      "chr1", 5,    15,
      "chr1", 50, 150,
      "chr2", 1000,   2000,
      "chr3", 3000, 4000
  ) %>% group_by(chrom)

  y <- tibble::tribble(
      ~chrom, ~start, ~end,
      "chr1", 25,    125,
      "chr1", 150,    250,
      "chr1", 550,    580,
      "chr2", 1,   1000,
      "chr2", 2000, 3000
  ) %>% group_by(chrom)



reldist_impl(x, y)

  */
