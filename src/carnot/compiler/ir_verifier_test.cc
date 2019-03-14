#include <gtest/gtest.h>

#include <pypa/ast/tree_walker.hh>
#include <pypa/parser/parser.hh>

#include "src/carnot/compiler/ir_verifier.h"
#include "src/carnot/compiler/test_utils.h"
#include "src/common/common.h"

namespace pl {
namespace carnot {
namespace compiler {
// Checks whether we can actually compile into a graph.
void CheckStatusVector(const std::vector<Status>& status_vec, bool should_fail) {
  if (should_fail) {
    EXPECT_NE(status_vec.size(), 0);
  } else {
    EXPECT_EQ(status_vec.size(), 0);
  }
  for (const Status& s : status_vec) {
    VLOG(1) << s.msg();
  }
}

/**
 * @brief Verifies that the graph has the corrected connected components. Expects each query to be
 * properly compiled into the IRGraph, otherwise the entire test fails.
 *
 * @param query
 * @param should_fail
 */
void GraphVerify(const std::string& query, bool should_fail) {
  auto ir_graph_status = ParseQuery(query);
  auto verifier = IRVerifier();
  VLOG(2) << ir_graph_status.ToString();
  EXPECT_OK(ir_graph_status);
  // this only should run if the ir_graph is completed. Basically, ParseQuery should run
  // successfullly for it to actually verify properly.
  if (ir_graph_status.ok()) {
    auto ir_graph = ir_graph_status.ValueOrDie();
    CheckStatusVector(verifier.VerifyGraphConnections(*ir_graph), should_fail);
    // Line Col should be set no matter what - this is independent of whether the query is written
    // incorrectly or not.
    CheckStatusVector(verifier.VerifyLineColGraph(*ir_graph), false);
  }
}

TEST(ASTVisitor, compilation_test) {
  std::string from_expr = "From(table='cpu', select=['cpu0', 'cpu1']).Result(name='cpu2')";
  GraphVerify(from_expr, false /*should_fail*/);
  // check the connection of ig
  std::string from_range_expr =
      "From(table='cpu', select=['cpu0']).Range(start=0,stop=10).Result(name='cpu2')";
  GraphVerify(from_range_expr, false /*should_fail*/);
}

TEST(ASTVisitor, assign_functionality) {
  std::string simple_assign =
      "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Result(name='cpu2')";
  GraphVerify(simple_assign, false /*should_fail*/);
  std::string assign_and_use =
      absl::StrJoin({"queryDF = From(table = 'cpu', select = [ 'cpu0', 'cpu1' ])",
                     "queryDF.Range(start=0, stop=10).Result(name='cpu2')"},
                    "\n");
  GraphVerify(assign_and_use, false /*should_fail*/);
}

// Range can only be after From, not after any other ops.
TEST(RangeTest, order_test) {
  std::string range_order_fail_map =
      absl::StrJoin({"queryDF = From(table='cpu', select=['cpu0', 'cpu1'])",
                     "mapDF = queryDF.Map(fn=lambda r : {'sum' : r.cpu0 + r.cpu1})",
                     "rangeDF = mapDF.Range(start=0,stop=10).Result(name='cpu2')"},
                    "\n");
  GraphVerify(range_order_fail_map, true /*should_fail*/);
  std::string range_order_fail_agg = absl::StrJoin(
      {"queryDF = From(table='cpu', select=['cpu0', 'cpu1'])",
       "mapDF = queryDF.Agg(fn=lambda r : {'sum' : pl.mean(r.cpu0)}, by=lambda r: r.cpu0)",
       "rangeDF = mapDF.Range(start=0,stop=10).Result(name='cpu2')"},
      "\n");
  GraphVerify(range_order_fail_agg, true /*should_fail*/);
}

// Map Tests
TEST(MapTest, single_col_map) {
  std::string single_col_map_sum = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sum' : r.cpu0 + r.cpu1}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(single_col_map_sum, false /*should_fail*/);
  std::string single_col_div_map_query = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sum' : "
          "pl.div(r.cpu0,r.cpu1)}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(single_col_div_map_query, false /*should_fail*/);
}

TEST(MapTest, multi_col_map) {
  std::string multi_col = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sum' : r.cpu0 + r.cpu1, 'copy' : "
          "r.cpu2}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(multi_col, false /*should_fail*/);
}

TEST(MapTest, bin_op_test) {
  std::string single_col_map_sum = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sum' : r.cpu0 + r.cpu1}).Result(name='cpu2')",
      },
      "\n");
  std::string single_col_map_sub = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sub' : r.cpu0 - r.cpu1}).Result(name='cpu2')",
      },
      "\n");
  std::string single_col_map_product = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'product' : r.cpu0 * r.cpu1}).Result(name='cpu2')",
      },
      "\n");
  std::string single_col_map_quotient = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'quotient' : r.cpu0 / r.cpu1}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(single_col_map_sum, false /*should_fail*/);
  GraphVerify(single_col_map_sub, false /*should_fail*/);
  GraphVerify(single_col_map_product, false /*should_fail*/);
  GraphVerify(single_col_map_quotient, false /*should_fail*/);
}

TEST(MapTest, nested_expr_map) {
  std::string nested_expr = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sum' : r.cpu0 + r.cpu1 + "
          "r.cpu2}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(nested_expr, false /*should_fail*/);
  std::string nested_fn = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'sum' : pl.div(r.cpu0 + r.cpu1, "
          "r.cpu2)}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(nested_fn, false /*should_fail*/);
}

TEST(AggTest, single_col_agg) {
  std::string single_col_agg = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Agg(by=lambda r : r.cpu0, fn=lambda r : {'cpu_count' : "
          "pl.count(r.cpu1)}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(single_col_agg, false /*should_fail*/);
  std::string multi_output_col_agg =
      absl::StrJoin({"queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
                     "rangeDF = queryDF.Agg(by=lambda r : r.cpu0, fn=lambda r : {'cpu_count' : "
                     "pl.count(r.cpu1), 'cpu_mean' : pl.mean(r.cpu1)}).Result(name='cpu2')"},
                    "\n");
  GraphVerify(multi_output_col_agg, false /*should_fail*/);
  std::string multi_input_col_agg = absl::StrJoin(
      {"queryDF = From(table='cpu', select=['cpu0', 'cpu1', 'cpu2']).Range(start=0,stop=10)",
       "rangeDF = queryDF.Agg(by=lambda r : r.cpu0, fn=lambda r : {'cpu_sum' : pl.sum(r.cpu1), "
       "'cpu2_mean' : pl.mean(r.cpu2)}).Result(name='cpu2')"},
      "\n");
  GraphVerify(multi_input_col_agg, false /*should_fail*/);
}
TEST(AggTest, not_allowed_by) {
  std::string single_col_bad_by_fn_expr = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Agg(by=lambda r : 1+2, fn=lambda r: {'cpu_count' : "
          "pl.count(r.cpu0)}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(single_col_bad_by_fn_expr, true /*should_fail*/);

  std::string single_col_dict_by_fn = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Agg(by=lambda r : {'cpu' : r.cpu0}, fn=lambda r : {'cpu_count' : "
          "pl.count(r.cpu0)}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(single_col_dict_by_fn, true /*should_fail*/);
}

TEST(AggTest, nested_agg_expression_should_fail) {
  std::string nested_agg_fn = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0, stop=10)",
          "rangeDF = queryDF.Agg(by=lambda r : r.cpu0, fn=lambda r : {'cpu_count' : "
          "pl.sum(pl.mean(r.cpu0))}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(nested_agg_fn, true /*should_fail*/);

  std::string add_combination = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0, stop=10)",
          "rangeDF = queryDF.Agg(by=lambda r : r.cpu0, fn=lambda r : {'cpu_count' : "
          "pl.mean(r.cpu0)+2}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(add_combination, true /*should_fail*/);
}

TEST(TimeTest, basic) {
  std::string add_test = absl::StrJoin(
      {
          "queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=10)",
          "rangeDF = queryDF.Map(fn=lambda r : {'time' : r.cpu + pl.second}).Result(name='cpu2')",
      },
      "\n");
  GraphVerify(add_test, false /*should_fail*/);
}

TEST(RangeValueTests, now_stop) {
  std::string plc_now_test = absl::StrJoin(
      {"queryDF = From(table='cpu', select=['cpu0', 'cpu1']).Range(start=0,stop=plc.now())",
       "rangeDF = queryDF.Map(fn=lambda r : {'plc_now' : r.cpu0 + pl.second})",
       "result = rangeDF.Result(name='mapped')"},
      "\n");
  GraphVerify(plc_now_test, false /*should_fail*/);
}
}  // namespace compiler
}  // namespace carnot
}  // namespace pl
