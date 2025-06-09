
#include "mlir/Pass/Pass.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/InitAllDialects.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/IR/PatternMatch.h"

struct RemoveTestDialectOps : public mlir::RewritePattern {
  RemoveTestDialectOps(mlir::MLIRContext *context)
      : RewritePattern(MatchAnyOpTypeTag(), /*benefit=*/1, context) {}

  llvm::LogicalResult matchAndRewrite(mlir::Operation *op,
                                      mlir::PatternRewriter &rewriter) const override {
    rewriter.eraseOp(op);
    return llvm::success();
  }
};


struct TestUnknownRootOpDriver
    : public mlir::PassWrapper<TestUnknownRootOpDriver, mlir::OperationPass<>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TestUnknownRootOpDriver)

  llvm::StringRef getArgument() const final {
    return "test-legalize-unknown-root-patterns";
  }

  llvm::StringRef getDescription() const final {
    return "Test public remapped value mechanism in ConversionPatternRewriter";
  }
  void runOnOperation() override {
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<RemoveTestDialectOps>(&getContext());

    mlir::ConversionTarget target(getContext());
    if (failed(applyPartialConversion(getOperation(), target,
                                      std::move(patterns))))
      signalPassFailure();
  }
};


int main(int argc , char** argv){
    mlir::DialectRegistry registry;
    mlir::registerAllDialects(registry);
    mlir::PassRegistration<TestUnknownRootOpDriver>();
}
