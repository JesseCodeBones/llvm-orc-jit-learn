
#include "mlir/Pass/Pass.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/InitAllDialects.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/IR/PatternMatch.h"
// struct SimplifyRedundantTranspose : public mlir::OpRewritePattern<mlir::vector::TransposeOp> {
//   /// We register this pattern to match every toy.transpose in the IR.
//   /// The "benefit" is used by the framework to order the patterns and process
//   /// them in order of profitability.
//   SimplifyRedundantTranspose(mlir::MLIRContext *context)
//       : OpRewritePattern<mlir::vector::TransposeOp>(context, /*benefit=*/1) {}
//
//   /// This method attempts to match a pattern and rewrite it. The rewriter
//   /// argument is the orchestrator of the sequence of rewrites. The pattern is
//   /// expected to interact with it to perform any changes to the IR from here.
//   llvm::LogicalResult
//   matchAndRewrite(mlir::vector::TransposeOp op,
//                   mlir::PatternRewriter &rewriter) const override {
//     // Look through the input of the current transpose.
//     mlir::Value transposeInput = op.getOperand();
//     mlir::vector::TransposeOp transposeInputOp = transposeInput.getDefiningOp<mlir::vector::TransposeOp>();
//
//     // Input defined by another transpose? If not, no match.
//     if (!transposeInputOp)
//       return llvm::failure();
//
//     // Otherwise, we have a redundant transpose. Use the rewriter.
//     rewriter.replaceOp(op, {transposeInputOp.getOperand()});
//     return llvm::success();
//   }
// };
#include <iostream>
int main(int argc , char** argv){
    std::cout << "hellow world\n";
}
