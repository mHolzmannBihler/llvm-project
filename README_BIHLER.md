# LLVM clang-tidy Build & Custom Check Integration

Komplette Anleitung zum Aufsetzen von clang-tidy auf einem neuen Windows-PC und zum Erstellen eigener Custom Checks.

---

## Voraussetzungen

### Erforderliche Software

| Software | Version | Download/Installation |
|----------|---------|----------------------|
| **Visual Studio 2022** | Professional/Community | [Download](https://visualstudio.microsoft.com/downloads/) |
| **CMake** | ≥ 3.26 | [Download](https://cmake.org/download/) |
| **Python** | 3.8 - 3.12 | [Download](https://www.python.org/downloads/) |
| **Git** | Latest | [Download](https://git-scm.com/download/win) |

### Visual Studio 2022 Workloads
Bei der Installation folgende Komponenten auswählen:
- ✅ Desktop development with C++
- ✅ C++ CMake tools for Windows
- ✅ Windows 10/11 SDK
- ✅ MSVC v143 - VS 2022 C++ x64/x86 build tools

### Systemanforderungen
- **Festplattenspeicher:** ~25 GB frei (LLVM Repo + Build)
- **RAM:** Mindestens 8 GB, empfohlen 16 GB
- **Build-Zeit:** 1-3 Stunden (abhängig von CPU)

---

## 1. LLVM Projekt klonen

```powershell
# Arbeitsverzeichnis erstellen
mkdir C:\TFS\Spielwiese\clangTidy
cd C:\TFS\Spielwiese\clangTidy

# LLVM Repository klonen
git clone https://github.com/llvm/llvm-project.git
```

**Hinweis:** Das Repository ist ca. 2-3 GB groß, der Download kann einige Minuten dauern.

---

## 2. Build-Umgebung konfigurieren

### CMake-Konfiguration

```powershell
# Build-Verzeichnis erstellen
mkdir llvm-build
cd llvm-build

# CMake konfigurieren mit Visual Studio 2022 Generator
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ^
  -DLLVM_TARGETS_TO_BUILD=X86 ^
  -DLLVM_ENABLE_ASSERTIONS=ON ^
  ..\llvm-project\llvm
```

**Wichtige CMake-Optionen:**
- Visual Studio 2022 Generator für optimale Windows-Kompatibilität
- Nur `clang` und `clang-tools-extra` (clang-tidy) werden gebaut
- Nur X86 Target (spart Build-Zeit)
- Assertions aktiviert für Debugging

---

## 3. clang-tidy bauen

### Manueller Build

```powershell
# Visual Studio Environment aktivieren
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

# CMake zum PATH hinzufügen
set PATH=C:\Program Files\CMake\bin;%PATH%

# clang-tidy bauen
cd C:\TFS\Spielwiese\ClangTidyBihler\llvm-build
cmake --build . --target clang-tidy --config Release
```

### Automatisiertes Build-Skript

Erstellen Sie `build_clang_tidy.bat`:

```batch
@echo off
echo Starting clang-tidy build...

REM Visual Studio Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64

REM CMake zum PATH hinzufügen
set "PATH=C:\Program Files\CMake\bin;%PATH%"

REM Build-Verzeichnis
cd /d "C:\TFS\Spielwiese\ClangTidyBihler\llvm-build"

REM Build ausführen
cmake --build . --target clang-tidy --config Release

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    Release\bin\clang-tidy.exe --version
) else (
    echo Build failed!
    exit /b 1
)
```

**Build-Dauer:** Erstes vollständiges Build: 1-3 Stunden (3010 Dateien)

**Hinweis:** Die fertige Binary liegt bei Verwendung des Visual Studio Generators unter `Release\bin\clang-tidy.exe`

---

## 4. Eigenen Custom Check erstellen

### Verzeichnisstruktur

```
llvm-project/
└── clang-tools-extra/
    └── clang-tidy/
        └── bihler/              # Ihr Custom Module
            ├── BihlerTidyModule.cpp
            ├── BihlerUnsafeAllocationCheck.h
            ├── BihlerUnsafeAllocationCheck.cpp
            └── CMakeLists.txt
```

### Dateien erstellen

#### 1. CMakeLists.txt

```cmake
set(LLVM_LINK_COMPONENTS Support)

add_clang_library(clangTidyBihlerModule
  BihlerTidyModule.cpp
  BihlerUnsafeAllocationCheck.cpp

  LINK_LIBS
  clangTidy
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  )
```

#### 2. BihlerTidyModule.cpp

```cpp
#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "BihlerUnsafeAllocationCheck.h"

namespace clang {
namespace tidy {
namespace bihler {

class BihlerModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<BihlerUnsafeAllocationCheck>(
        "bihler-unsafe-allocation");
  }
};

static ClangTidyModuleRegistry::Add<BihlerModule>
    X("bihler-module", "Adds Bihler custom lint checks.");

} // namespace bihler

volatile int BihlerModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
```

#### 3. BihlerUnsafeAllocationCheck.h

```cpp
#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BIHLER_UNSAFEALLOCATIONCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BIHLER_UNSAFEALLOCATIONCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace bihler {

class BihlerUnsafeAllocationCheck : public ClangTidyCheck {
public:
  BihlerUnsafeAllocationCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  bool isNothrowNew(const CXXNewExpr *NewExpr);
};

} // namespace bihler
} // namespace tidy
} // namespace clang

#endif
```

#### 4. BihlerUnsafeAllocationCheck.cpp

Vollständige Implementation siehe Git Repository.

**Wichtige Features:**
- ✅ Erkennt `new` Ausdrücke ohne try-catch
- ✅ Ignoriert `new(std::nothrow)` Ausdrücke  
- ✅ Erkennt 20+ STL-Container-Methoden (push_back, insert, emplace, etc.)
- ✅ Erkennt map/unordered_map `operator[]`
- ✅ Erkennt Smart Pointer Factories (make_unique, make_shared)
- ✅ Respektiert try-catch Blöcke

---

## 5. Build-System Integration

### Main CMakeLists.txt aktualisieren

**Datei:** `clang-tools-extra/clang-tidy/CMakeLists.txt`

```cmake
# Bei den add_subdirectory Einträgen:
add_subdirectory(zircon)
add_subdirectory(bihler)  # <-- Hinzufügen

# Bei der ALL_CLANG_TIDY_CHECKS Liste:
  clangTidyZirconModule
  clangTidyBihlerModule  # <-- Hinzufügen
)
```

### ClangTidyForceLinker.h aktualisieren

**Datei:** `clang-tools-extra/clang-tidy/ClangTidyForceLinker.h`

```cpp
// Am Ende der Datei vor dem namespace-Ende:
// This anchor is used to force the linker to link the BihlerModule.
extern volatile int BihlerModuleAnchorSource;
[[maybe_unused]] static int BihlerModuleAnchorDestination =
    BihlerModuleAnchorSource;

} // namespace clang::tidy
```

---

## 6. Custom Check bauen und testen

### Rebuild nach Änderungen

```powershell
cd C:\TFS\Spielwiese\clangTidy\llvm-build

# CMake neu konfigurieren (erkennt neue Dateien)
cmake --build . --target clean

# clang-tidy neu bauen
ninja clang-tidy
```

**Inkrementeller Build:** Nur 2-4 Dateien (~10 Sekunden)

### Check verfügbar prüfen

```powershell
.\bin\clang-tidy.exe --list-checks --checks=* | findstr bihler
```

**Erwartete Ausgabe:**
```
    bihler-unsafe-allocation
```

### Testdatei erstellen

**test_allocation.cpp:**
```cpp
#include <vector>
#include <memory>
#include <new>

void test() {
    // Warnung: nicht geschützt
    int* ptr1 = new int(42);
    std::vector<int> vec;
    vec.push_back(1);
    
    // Keine Warnung: nothrow
    int* ptr2 = new(std::nothrow) int(99);
    
    // Keine Warnung: in try-catch
    try {
        int* ptr3 = new int(77);
        vec.reserve(1000);
    } catch (const std::bad_alloc&) {}
}
```

### Check ausführen

```powershell
.\Release\bin\clang-tidy.exe --checks="-*,bihler-unsafe-allocation" test_allocation.cpp
```

**Erwartete Ausgabe:**
```
warning: new expression is not protected by try-catch block...
warning: STL method 'push_back' is not protected by try-catch block...
```

---

## 7. Häufige Probleme & Lösungen

### Problem: "cmake: command not found"

**Lösung:**
```powershell
# CMake zum PATH hinzufügen
set PATH=C:\Program Files\CMake\bin;%PATH%
```

### Problem: "MSVC not found"

**Lösung:**
```powershell
# Visual Studio Developer Command Prompt starten oder:
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
```

### Problem: Build-Fehler nach neuen Dateien

**Lösung:**
```powershell
# CMake Cache löschen und neu konfigurieren
cmake --build . --target clean
cmake ..
```

### Problem: Check wird nicht gefunden

**Lösung:**
1. Prüfen ob Modul in CMakeLists.txt eingetragen
2. Prüfen ob Anchor in ClangTidyForceLinker.h vorhanden
3. Vollständiges Rebuild durchführen

---

## 8. Schnellreferenz

### Build-Commands

```powershell
# Kompletter Rebuild
cmake --build . --target clang-tidy --config Release

# Nur spezifisches Target
cmake --build . --target clangTidyBihlerModule --config Release

# Parallel Build (schneller)
cmake --build . --target clang-tidy --config Release --parallel 8
```

### Check verwenden

```powershell
# Einzelner Check
Release\bin\clang-tidy.exe --checks="-*,bihler-unsafe-allocation" file.cpp

# Mit automatischen Fixes
Release\bin\clang-tidy.exe --checks="bihler-*" --fix file.cpp

# Alle Bihler-Checks
Release\bin\clang-tidy.exe --checks="bihler-*" file.cpp
```

### Verzeichnisstruktur

```
C:\TFS\Spielwiese\ClangTidyBihler\
├── llvm-project/              # Git Repository
│   └── clang-tools-extra/
│       └── clang-tidy/
│           ├── bihler/        # Custom Module
│           ├── CMakeLists.txt
│           └── ClangTidyForceLinker.h
├── llvm-build/                # Build-Verzeichnis
│   └── Release/
│       └── bin/
│           └── clang-tidy.exe # Fertige Binary
└── build_clang_tidy.bat       # Build-Skript
```

---

## Zusammenfassung

**Zeit-Aufwand:**
- Erstes Setup: ~4-5 Stunden (inkl. Downloads, erstem Build)
- Custom Check erstellen: ~30-60 Minuten
- Rebuild nach Änderungen: ~10 Sekunden

**Was Sie jetzt haben:**
- ✅ Funktionierendes clang-tidy Build-System
- ✅ Eigenes Custom Check Module (`bihler-unsafe-allocation`)
- ✅ Erkennung von 20+ Allokations-Patterns
- ✅ std::nothrow Unterstützung
- ✅ Try-catch Block Erkennung
- ✅ Vollständige STL-Container-Abdeckung

**Nächste Schritte:**
- Weitere Custom Checks hinzufügen
- Checks in CI/CD Pipeline integrieren
- .clang-tidy Konfigurationsdatei erstellen
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ^
  -DLLVM_TARGETS_TO_BUILD="X86" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLLVM_ENABLE_ASSERTIONS=ON
```
Notes:
- Restricting `LLVM_TARGETS_TO_BUILD` speeds up build (add more if needed later).
- Use `-DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;compiler-rt"` if later needed.

### Alternative (Generate VS Solution)
```
cmake -S llvm-project/llvm -B llvm-build -G "Visual Studio 17 2022" -A x64 ^
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_ASSERTIONS=ON
```
Then open `llvm-build/LLVM.sln` and build `clang-tidy` target.

---
## 4. Build
```
cmake --build llvm-build --target clang-tidy -- -j %NUMBER_OF_PROCESSORS%
```
Produces executable under `llvm-build/bin/clang-tidy.exe` (Ninja) or appropriate VS output dir.

---
## 5. Validate clang-tidy
Create a sample file `test.cpp`:
```
#include <vector>
int main() { std::vector<int> v; return 0; }
```
Run:
```
llvm-build/bin/clang-tidy.exe test.cpp -checks="*" -- -std=c++17
```
Version check:
```
llvm-build\bin\clang-tidy.exe --version
```
If it runs and prints diagnostics, the baseline build is good.

---
## 6. Adding a Custom Check (Next Step After Validation)
High-level steps (do only after baseline success):
1. Create a module directory: `llvm-project/clang-tools-extra/clang-tidy/MyCompany`.
2. Add your check impl: `MySampleCheck.h/.cpp` deriving from `ClangTidyCheck`.
3. Register in a new `MyCompanyTidyModule.cpp` implementing `ClangTidyModule` and adding to `ClangTidyModuleRegistry`.
4. Update `CMakeLists.txt` in `clang-tidy` folder to include new sources.
5. Reconfigure & rebuild target `clang-tidy`.
6. Use `-checks=mysample-*` to test.
Detailed implementation will follow once validation of the unmodified build is confirmed.

---
## 7. Troubleshooting
| Symptom | Fix |
|---------|-----|
| Missing `cl.exe` | Use VS Native Tools prompt / ensure C++ workload installed |
| CMake cannot find Python | Ensure Python added to PATH; retry prompt | 
| Link errors (OOM) | Add `-DCMAKE_BUILD_TYPE=RelWithDebInfo` or reduce targets | 
| Slow build | Use Ninja + assertions OFF for speed (`-DLLVM_ENABLE_ASSERTIONS=OFF`) | 

---
## 8. Disk Space Strategy
- Delete `llvm-build/CMakeCache.txt` & reconfigure if switching generators.
- Use `-DLLVM_BUILD_TOOLS=ON` (default) but avoid adding large extra projects unless needed.

---
## 9. Next Actions
1. Run `./prereq_check.ps1` and install missing items.
2. Clone + configure + build.
3. Confirm `clang-tidy --version` works.
4. Proceed to custom check implementation (future step).

---
## 10. License & Upstream
Stick to upstream `llvm-project` structure to simplify rebasing. Keep custom checks isolated under your own module directory.

---
## 11. Upgrade Process For New LLVM/clang-tidy Releases
When a new upstream release (for example `llvmorg-22.1.1`) is available, use this process:

1. Ensure the current branch is clean.
2. Commit all local Bihler changes first.
3. Fetch latest upstream tags.
4. Create a new Bihler release branch from the new upstream tag.
5. Cherry-pick the Bihler patch series onto the new branch.
6. Resolve conflicts (typically in `clang-tidy/CMakeLists.txt` and `ClangTidyForceLinker.h`).
7. Build and validate `clang-tidy`.
8. Run a small smoke test and then the full project analysis.

Recommended commands:
```bash
git status
git fetch upstream --tags

# Create new branch from upstream tag
git checkout -b release/llvmorg-22.1.1-bihler llvmorg-22.1.1

# Replay Bihler patches (example range)
git cherry-pick 5f3460814b8e^..bihler-patches

# Build + validate
cmake --build llvm-build --target clang-tidy --config Release
llvm-build/bin/clang-tidy.exe --list-checks --checks="*" | findstr bihler
```

Validation checklist after upgrade:
- `clang-tidy --version` shows expected new version.
- `bihler-unsafe-allocation` appears in `--list-checks`.
- Local sample test for BihlHeap warnings passes.
- Full project report can be generated.

---
If you need the custom check scaffolding next, let me know after validation.
