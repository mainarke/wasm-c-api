#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>

#include "wasm.hh"

// Print a Wasm value
void val_print(const wasm::val& val) {
  switch (val.kind()) {
    case wasm::I32: {
      std::cout << val.i32();
    } break;
    case wasm::I64: {
      std::cout << val.i64();
    } break;
    case wasm::F32: {
      std::cout << val.f32();
    } break;
    case wasm::F64: {
      std::cout << val.f64();
    } break;
    case wasm::ANYREF:
    case wasm::FUNCREF: {
      if (val.ref() == nullptr) {
        std::cout << "null";
      } else {
        std::cout << "ref(" << val.ref() << ")";
      }
    } break;
  }
}

// A function to be called from Wasm code.
auto print_wasm(const wasm::vec<wasm::val>& args) -> wasm::vec<wasm::val> {
  std::cout << "Calling back..." << std::endl << ">";
  for (size_t i = 0; i < args.size(); ++i) {
    std::cout << " ";
    val_print(args[i]);
  }
  std::cout << std::endl;

  int32_t n = args.size();
  return wasm::vec<wasm::val>::make(wasm::val(n));
}


void run(int argc, const char* argv[]) {
  // Initialize.
  std::cout << "Initializing..." << std::endl;
  auto engine = wasm::engine::make(argc, argv);
  auto store = wasm::store::make(engine);

  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  std::ifstream file("hello.wasm");
  file.seekg(0, std::ios_base::end);
  auto file_size = file.tellg();
  file.seekg(0);
  auto binary = wasm::vec<byte_t>::make_uninitialized(file_size);
  file.read(binary.get(), file_size);
  file.close();
  if (file.fail()) {
    std::cout << "> Error loading module!" << std::endl;
    return;
  }

  // Compile.
  std::cout << "Compiling module..." << std::endl;
  auto module = wasm::module::make(store, binary);
  if (!module) {
    std::cout << "> Error compiling module!" << std::endl;
    return;
  }

  // Create external print functions.
  std::cout << "Creating callbacks..." << std::endl;
  auto print_type1 = wasm::functype::make(
    wasm::vec<wasm::valtype*>::make(wasm::valtype::make(wasm::I32)),
    wasm::vec<wasm::valtype*>::make(wasm::valtype::make(wasm::I32))
  );
  auto print_func1 = wasm::func::make(store, print_type1, print_wasm);

  auto print_type2 = wasm::functype::make(
    wasm::vec<wasm::valtype*>::make(wasm::valtype::make(wasm::I32), wasm::valtype::make(wasm::I32)),
    wasm::vec<wasm::valtype*>::make(wasm::valtype::make(wasm::I32))
  );
  auto print_func2 = wasm::func::make(store, print_type2, print_wasm);

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  auto imports = wasm::vec<wasm::external*>::make(print_func1, print_func2);
  auto instance = wasm::instance::make(store, module, imports);
  if (!instance) {
    std::cout << "> Error instantiating module!" << std::endl;
    return;
  }

  // Extract export.
  std::cout << "Extracting exports..." << std::endl;
  auto exports = instance->exports();
  if (exports.size() == 0 || exports[0]->kind() != wasm::EXTERN_FUNC || !exports[0]->func()) {
    std::cout << "> Error accessing export!" << std::endl;
    return;
  }
  auto run_func = exports[0]->func();

  // Call.
  std::cout << "Calling exports..." << std::endl;
  auto results = run_func->call(wasm::val(3), wasm::val(4));

  // Print result.
  std::cout << "Printing result..." << std::endl;
  std::cout << "> " << results[0].i32() << std::endl;

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}


int main(int argc, const char* argv[]) {
  run(argc, argv);
  std::cout << "Done." << std::endl;
  return 0;
}
