// Singleton Definition

class ModRunner {
public:
  static ModRunner& Instance();
  bool Start();
  bool ModButton();
private:
  ModRunner() = default;
  ~ModRunner() = default;
  ModRunner(const ModRunner&) = delete;
  ModRunner& operator=(const ModRunner&) = delete;
};
