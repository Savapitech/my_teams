#pragma once

#include <dlfcn.h>
#include <memory>
#include <string>

template <typename T> class DLLoader {
public:
  DLLoader(const DLLoader &) = delete;
  DLLoader &operator=(const DLLoader &) = delete;

  DLLoader(const std::string &path) : _handle(nullptr), _path(path) {
    _handle = dlopen(_path.c_str(), RTLD_LAZY);

    if (_handle == nullptr)
      throw std::runtime_error(dlerror());
  }

  ~DLLoader() {
    if (_handle != nullptr) {
      dlclose(_handle);
      _handle = nullptr;
    }
  }

  std::unique_ptr<T> getInstance(const std::string &symbol) {
    dlerror();

    using create_t = T *(*)();

    create_t entryPoint =
        reinterpret_cast<create_t>(dlsym(_handle, symbol.c_str()));

    const char *err = dlerror();
    if (err)
      throw std::runtime_error(err);

    T *t = entryPoint();
    return std::unique_ptr<T>(t);
  }

private:
  void *_handle;
  std::string _path;
};
