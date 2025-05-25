#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/IRBuilder.h"
#include <iostream>
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/IR/Verifier.h"
class TestJIT {

private:
    static llvm::Expected<llvm::orc::ThreadSafeModule>
  optimizeModule(llvm::orc::ThreadSafeModule TSM, const llvm::orc::MaterializationResponsibility &R) {
        TSM.withModuleDo([](llvm::Module &M) {
          // Create a function pass manager.
          auto FPM = std::make_unique<llvm::legacy::FunctionPassManager>(&M);

          // Add some optimizations.
          FPM->add(llvm::createInstructionCombiningPass());
          FPM->add(llvm::createReassociatePass());
          FPM->add(llvm::createGVNPass());
          FPM->add(llvm::createCFGSimplificationPass());
          FPM->doInitialization();

          // Run the optimizations over all functions in the module being added to
          // the JIT.
          for (auto &F : M)
              FPM->run(F);
          });

        return std::move(TSM);
    }

private:
    std::unique_ptr<llvm::orc::ExecutionSession> ES;
    llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
    llvm::orc::IRCompileLayer CompileLayer;

    llvm::orc::IRTransformLayer TransformLayer;

    llvm::DataLayout DL;
    llvm::orc::MangleAndInterner Mangle;

    llvm::orc::JITDylib &Dylib;

public:
    TestJIT() = delete;

    TestJIT(std::unique_ptr<llvm::orc::ExecutionSession> ES,
            llvm::orc::JITTargetMachineBuilder JTMB, llvm::DataLayout DL)
        : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
          ObjectLayer(*this->ES,
                      []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
          CompileLayer(*this->ES, ObjectLayer,
                       std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(JTMB))),
          TransformLayer(*this->ES, this->CompileLayer, optimizeModule),
          Dylib(this->ES->createBareJITDylib("<main>")) {
        Dylib.addGenerator(
            llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                DL.getGlobalPrefix())));
        if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
            ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }

    ~TestJIT() {
        if (auto err = ES->endSession()) {
            ES->reportError(std::move(err));
        }
    }

    static llvm::Expected<std::unique_ptr<TestJIT> > create() {
        auto EPC = llvm::orc::SelfExecutorProcessControl::Create();
        if (!EPC)
            return EPC.takeError();

        auto ES = std::make_unique<llvm::orc::ExecutionSession>(std::move(*EPC));

        llvm::orc::JITTargetMachineBuilder JTMB(
            ES->getExecutorProcessControl().getTargetTriple());

        auto DL = JTMB.getDefaultDataLayoutForTarget();
        if (!DL)
            return DL.takeError();

        return std::make_unique<TestJIT>(std::move(ES), std::move(JTMB),
                                         std::move(*DL));
    }

    auto &getDataLayout() {
        return this->DL;
    }

    auto& getDylib() const {
        return this->Dylib;
    }

    llvm::Error addModule(llvm::orc::ThreadSafeModule &&TSM, llvm::orc::ResourceTrackerSP RT = nullptr) {
        if (!RT) { RT = Dylib.getDefaultResourceTracker(); }
        auto module = TSM.getModuleUnlocked();
        module->setDataLayout(DL);
        return CompileLayer.add(RT, std::move(TSM));
    }

    llvm::Expected<llvm::orc::ExecutorSymbolDef> lookup(const llvm::StringRef Name) {
        llvm::outs() << "Lookup " << Name << "\n";
        Dylib.dump(llvm::outs());
        return ES->lookup({&Dylib}, Mangle(Name));
    }
};


void addExposedGlobal(llvm::Module *M, llvm::LLVMContext &Ctx) {
    // 创建全局整数变量，初始值为42
    llvm::ConstantInt *Init = llvm::ConstantInt::get(llvm::IntegerType::get(Ctx, 32), 42);
    new llvm::GlobalVariable(
        *M, // 所属Module（传递引用）
        llvm::IntegerType::get(Ctx, 32), // 类型
        false, // 是否为常量
        llvm::GlobalValue::ExternalLinkage, // 关键：外部链接
        Init, // 初始值
        "_exposed_global" // 符号名
    );  
}

int main(int, char **) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    auto jit = TestJIT::create();

    if (!jit) {
        std::cerr << "Failed to create JIT layer." << std::endl;
        return 1;
    }
    auto llvmContext = std::make_unique<llvm::LLVMContext>();
    auto modulePtr = std::make_unique<llvm::Module>("bar", *llvmContext);


    auto Builder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);
    llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::Type::getInt32Ty(*llvmContext), std::nullopt, false);
    llvm::Function *F =
            llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "testFun", modulePtr.get());
    auto functionBasicBlock = llvm::BasicBlock::Create(*llvmContext, "entry", F);
    Builder->SetInsertPoint(functionBasicBlock);
    Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvmContext), 42));
    llvm::verifyFunction(*F, &llvm::outs());

    llvm::orc::ThreadSafeModule threadSafeModule {
        std::move(modulePtr),
        std::move(llvmContext),
    };
    if (auto err = (*jit)->addModule(std::move(threadSafeModule), (*jit)->getDylib().createResourceTracker())) {
        std::cerr << "Failed to add module." << std::endl;
        return 1;
    }
    (*jit)->getDylib().dump(llvm::outs());

    auto result = (*jit)->lookup("testFun");
    if (!result) {
        std::cerr << "Failed to lookup testFun." << std::endl;
    } else {
        std::cout << "Execution pointer: 0x" << std::hex << result.get().getAddress().getValue() << std::endl;
    }
    auto fun = (int(*)()) result.get().getAddress().getValue();
    (*jit)->getDylib().dump(llvm::outs());

    std::cout << "jit fun result = " << std::dec << fun() << std::endl;
    std::cout << "Hello, from orc-jit-1!\n";
    return 0;
}
